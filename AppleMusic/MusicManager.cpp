#include "MusicManager.h"

#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioDevice>
#include <QAudioSink>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QMediaDevices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSet>
#include <QStringConverter>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
const char *kDatabaseConnectionPrefix = "music_library_";
constexpr float kPi = 3.14159265358979323846f;

QString normalizeSeparators(QString path)
{
    return path.replace('\\', '/');
}

QString sqlValue(const QSqlQuery &query, const char *name)
{
    const int index = query.record().indexOf(QString::fromLatin1(name));
    return index >= 0 ? query.value(index).toString() : QString();
}

int sqlIntValue(const QSqlQuery &query, const char *name)
{
    const int index = query.record().indexOf(QString::fromLatin1(name));
    return index >= 0 ? query.value(index).toInt() : 0;
}

float clampAudio(float value)
{
    return std::clamp(value, -1.0f, 0.999999f);
}

QString formatDuration(int seconds)
{
    if (seconds <= 0) {
        return QStringLiteral("--:--");
    }
    return QStringLiteral("%1:%2")
        .arg(seconds / 60)
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

float readSample(const char *data, qsizetype index, QAudioFormat::SampleFormat format)
{
    switch (format) {
    case QAudioFormat::UInt8: {
        quint8 value = 0;
        std::memcpy(&value, data + index, sizeof(value));
        return (static_cast<float>(value) - 128.0f) / 128.0f;
    }
    case QAudioFormat::Int16: {
        qint16 value = 0;
        std::memcpy(&value, data + index * static_cast<qsizetype>(sizeof(value)), sizeof(value));
        return static_cast<float>(value) / 32768.0f;
    }
    case QAudioFormat::Int32: {
        qint32 value = 0;
        std::memcpy(&value, data + index * static_cast<qsizetype>(sizeof(value)), sizeof(value));
        return static_cast<float>(static_cast<double>(value) / 2147483648.0);
    }
    case QAudioFormat::Float: {
        float value = 0.0f;
        std::memcpy(&value, data + index * static_cast<qsizetype>(sizeof(value)), sizeof(value));
        return value;
    }
    default:
        return 0.0f;
    }
}

void writeSample(char *data, qsizetype index, QAudioFormat::SampleFormat format, float sample)
{
    const float value = clampAudio(sample);
    switch (format) {
    case QAudioFormat::UInt8: {
        const quint8 encoded = static_cast<quint8>(std::lround((value + 1.0f) * 127.5f));
        std::memcpy(data + index, &encoded, sizeof(encoded));
        break;
    }
    case QAudioFormat::Int16: {
        const qint16 encoded = static_cast<qint16>(std::lround(value * 32767.0f));
        std::memcpy(data + index * static_cast<qsizetype>(sizeof(encoded)), &encoded, sizeof(encoded));
        break;
    }
    case QAudioFormat::Int32: {
        const qint32 encoded = static_cast<qint32>(std::llround(static_cast<double>(value) * 2147483647.0));
        std::memcpy(data + index * static_cast<qsizetype>(sizeof(encoded)), &encoded, sizeof(encoded));
        break;
    }
    case QAudioFormat::Float:
        std::memcpy(data + index * static_cast<qsizetype>(sizeof(value)), &value, sizeof(value));
        break;
    default:
        break;
    }
}
}

MusicManager::MusicManager(QObject *parent) : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_network = new QNetworkAccessManager(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(1.0);

    const QAudioFormat effectFormat = QMediaDevices::defaultAudioOutput().preferredFormat();
    m_audioBufferOutput = new QAudioBufferOutput(effectFormat, this);
    m_player->setAudioBufferOutput(m_audioBufferOutput);
    connect(m_audioBufferOutput, &QAudioBufferOutput::audioBufferReceived,
            this, &MusicManager::onAudioBufferReceived);

    m_effectDrainTimer = new QTimer(this);
    m_effectDrainTimer->setInterval(10);
    connect(m_effectDrainTimer, &QTimer::timeout, this, &MusicManager::flushEffectAudio);

    connect(m_player, &QMediaPlayer::positionChanged, this, &MusicManager::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MusicManager::durationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MusicManager::playbackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MusicManager::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
                if (m_audioEffect == 0) {
                    return;
                }
                if (m_effectSink) {
                    if (state == QMediaPlayer::PlayingState &&
                        m_effectSink->state() == QtAudio::SuspendedState) {
                        m_effectSink->resume();
                    } else if (state == QMediaPlayer::PausedState &&
                               m_effectSink->state() == QtAudio::ActiveState) {
                        m_effectSink->suspend();
                    } else if (state == QMediaPlayer::StoppedState) {
                        resetEffectPipeline();
                    }
                }

                if (state == QMediaPlayer::PlayingState && !m_effectReceivedBuffer) {
                    QTimer::singleShot(1600, this, &MusicManager::verifyEffectPipeline);
                }
            });

    if (openDatabase() && ensureSchema()) {
        ensureLibraryIsCurrent();
    }

    refreshLocalMusic();
    m_playlist = m_localMusic;
}

MusicManager::~MusicManager()
{
    const QString connectionName = m_connectionName;
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
    }
    if (!connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool MusicManager::openDatabase()
{
    const QString dbPath = databasePath();
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    m_connectionName = QString::fromLatin1(kDatabaseConnectionPrefix) + QString::number(reinterpret_cast<quintptr>(this));
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open music database:" << dbPath << m_db.lastError().text();
        return false;
    }
    return true;
}

bool MusicManager::ensureSchema()
{
    QSqlQuery query(m_db);
    const bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS songs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL UNIQUE,"
        "artist TEXT,"
        "album TEXT,"
        "duration TEXT,"
        "audio_path TEXT NOT NULL,"
        "cover_path TEXT,"
        "lyric_path TEXT,"
        "mv_path TEXT,"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ")");

    if (!ok) {
        qWarning() << "Failed to create songs table:" << query.lastError().text();
    }
    return ok;
}

void MusicManager::ensureLibraryIsCurrent()
{
    QSqlQuery countQuery(m_db);
    if (!countQuery.exec("SELECT COUNT(*) FROM songs") || !countQuery.next() || countQuery.value(0).toInt() == 0) {
        rebuildDatabaseFromMedia();
        return;
    }
    repairMissingPaths();
}

void MusicManager::rebuildDatabaseFromMedia()
{
    QDir mediaDir(mediaRootPath());
    if (!mediaDir.exists()) {
        QDir().mkpath(mediaDir.absolutePath());
        return;
    }

    QSqlQuery clearQuery(m_db);
    clearQuery.exec("DELETE FROM songs");

    const QFileInfoList files = mediaDir.entryInfoList(QStringList() << "*.mp3", QDir::Files, QDir::Name);
    for (const QFileInfo &fileInfo : files) {
        const QString title = fileInfo.completeBaseName();
        const QString audioPath = relativePathForFile(fileInfo.absoluteFilePath());
        const QString coverPath = relativePathForFile(findMediaFile(title, QStringList() << "png" << "jpg" << "jpeg"));
        const QString lyricPath = relativePathForFile(findMediaFile(title, QStringList() << "lrc" << "txt"));
        const QString mvPath = relativePathForFile(findMediaFile(title, QStringList() << "mp4" << "mov" << "mkv"));
        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        QSqlQuery insert(m_db);
        insert.prepare(
            "INSERT INTO songs (title, artist, album, duration, audio_path, cover_path, lyric_path, mv_path, created_at, updated_at) "
            "VALUES (:title, :artist, :album, :duration, :audio_path, :cover_path, :lyric_path, :mv_path, :created_at, :updated_at)");
        insert.bindValue(":title", title);
        insert.bindValue(":artist", guessArtist(title));
        insert.bindValue(":album", guessAlbum(title));
        insert.bindValue(":duration", guessDuration(title));
        insert.bindValue(":audio_path", audioPath);
        insert.bindValue(":cover_path", coverPath);
        insert.bindValue(":lyric_path", lyricPath);
        insert.bindValue(":mv_path", mvPath);
        insert.bindValue(":created_at", now);
        insert.bindValue(":updated_at", now);

        if (!insert.exec()) {
            qWarning() << "Failed to insert song:" << title << insert.lastError().text();
        }
    }
}

void MusicManager::repairMissingPaths()
{
    QSqlQuery select(m_db);
    if (!select.exec("SELECT id, title, artist, album, audio_path, cover_path, lyric_path, mv_path FROM songs")) {
        qWarning() << "Failed to validate songs:" << select.lastError().text();
        return;
    }

    QSet<QString> knownTitles;
    while (select.next()) {
        const int id = sqlIntValue(select, "id");
        const QString title = sqlValue(select, "title");
        knownTitles.insert(title.toCaseFolded());
        QString artist = sqlValue(select, "artist");
        QString album = sqlValue(select, "album");
        QString audioPath = sqlValue(select, "audio_path");
        QString coverPath = sqlValue(select, "cover_path");
        QString lyricPath = sqlValue(select, "lyric_path");
        QString mvPath = sqlValue(select, "mv_path");
        bool changed = false;

        if (resolveRelativePath(audioPath).isEmpty()) {
            audioPath = relativePathForFile(findMediaFile(title, QStringList() << "mp3"));
            changed = true;
        }
        if (!coverPath.isEmpty() && resolveRelativePath(coverPath).isEmpty()) {
            coverPath.clear();
            changed = true;
        }
        if (coverPath.isEmpty()) {
            coverPath = relativePathForFile(findMediaFile(title, QStringList() << "png" << "jpg" << "jpeg"));
            changed = true;
        }
        if (!lyricPath.isEmpty() && resolveRelativePath(lyricPath).isEmpty()) {
            lyricPath.clear();
            changed = true;
        }
        if (lyricPath.isEmpty()) {
            lyricPath = relativePathForFile(findMediaFile(title, QStringList() << "lrc" << "txt"));
            changed = true;
        }
        if (!mvPath.isEmpty() && resolveRelativePath(mvPath).isEmpty()) {
            mvPath.clear();
            changed = true;
        }
        if (mvPath.isEmpty()) {
            mvPath = relativePathForFile(findMediaFile(title, QStringList() << "mp4" << "mov" << "mkv"));
            changed = true;
        }
        const QString guessedArtist = guessArtist(title);
        if (!guessedArtist.isEmpty() &&
            guessedArtist != QStringLiteral("Unknown Artist") &&
            (artist.trimmed().isEmpty() ||
             artist == QStringLiteral("????") ||
             artist == QStringLiteral("???") ||
             artist.compare(QStringLiteral("Unknown Artist"), Qt::CaseInsensitive) == 0)) {
            artist = guessedArtist;
            changed = true;
        }
        const QString guessedAlbum = guessAlbum(title);
        if (!guessedAlbum.isEmpty() &&
            (album.trimmed().isEmpty() ||
             album == QStringLiteral("????") ||
             album == QStringLiteral("???") ||
             album.compare(QStringLiteral("Local Music"), Qt::CaseInsensitive) == 0)) {
            album = guessedAlbum;
            changed = true;
        }

        if (!changed) {
            continue;
        }

        QSqlQuery update(m_db);
        update.prepare(
            "UPDATE songs SET artist = :artist, album = :album, audio_path = :audio_path, cover_path = :cover_path, lyric_path = :lyric_path, "
            "mv_path = :mv_path, updated_at = :updated_at WHERE id = :id");
        update.bindValue(":artist", artist);
        update.bindValue(":album", album);
        update.bindValue(":audio_path", audioPath);
        update.bindValue(":cover_path", coverPath);
        update.bindValue(":lyric_path", lyricPath);
        update.bindValue(":mv_path", mvPath);
        update.bindValue(":updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        update.bindValue(":id", id);
        if (!update.exec()) {
            qWarning() << "Failed to repair song paths:" << title << update.lastError().text();
        }
    }

    QDir mediaDir(mediaRootPath());
    const QFileInfoList files = mediaDir.entryInfoList(QStringList() << "*.mp3", QDir::Files, QDir::Name);
    for (const QFileInfo &fileInfo : files) {
        const QString title = fileInfo.completeBaseName();
        if (knownTitles.contains(title.toCaseFolded())) {
            continue;
        }

        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        QSqlQuery insert(m_db);
        insert.prepare(
            "INSERT INTO songs (title, artist, album, duration, audio_path, cover_path, lyric_path, mv_path, created_at, updated_at) "
            "VALUES (:title, :artist, :album, :duration, :audio_path, :cover_path, :lyric_path, :mv_path, :created_at, :updated_at)");
        insert.bindValue(":title", title);
        insert.bindValue(":artist", guessArtist(title));
        insert.bindValue(":album", guessAlbum(title));
        insert.bindValue(":duration", guessDuration(title));
        insert.bindValue(":audio_path", relativePathForFile(fileInfo.absoluteFilePath()));
        insert.bindValue(":cover_path", relativePathForFile(findMediaFile(title, QStringList() << "png" << "jpg" << "jpeg")));
        insert.bindValue(":lyric_path", relativePathForFile(findMediaFile(title, QStringList() << "lrc" << "txt")));
        insert.bindValue(":mv_path", relativePathForFile(findMediaFile(title, QStringList() << "mp4" << "mov" << "mkv")));
        insert.bindValue(":created_at", now);
        insert.bindValue(":updated_at", now);
        if (!insert.exec()) {
            qWarning() << "Failed to add new media file:" << title << insert.lastError().text();
        }
    }
}

void MusicManager::refreshLocalMusic()
{
    m_localMusic.clear();

    QSqlQuery query(m_db);
    if (!query.exec("SELECT title, artist, album, duration, audio_path, cover_path, lyric_path, mv_path FROM songs ORDER BY title COLLATE NOCASE")) {
        qWarning() << "Failed to load local music:" << query.lastError().text();
        emit localMusicChanged();
        return;
    }

    while (query.next()) {
        QVariantMap record;
        record["title"] = sqlValue(query, "title");
        record["artist"] = sqlValue(query, "artist");
        record["album"] = sqlValue(query, "album");
        record["duration"] = sqlValue(query, "duration");
        record["audio_path"] = sqlValue(query, "audio_path");
        record["cover_path"] = sqlValue(query, "cover_path");
        record["lyric_path"] = sqlValue(query, "lyric_path");
        record["mv_path"] = sqlValue(query, "mv_path");
        m_localMusic.append(songMapFromRecord(record));
    }

    emit localMusicChanged();
}

void MusicManager::search(const QString &queryText)
{
    m_results.clear();

    QSqlQuery query(m_db);
    if (queryText.isEmpty() || queryText == "all") {
        query.prepare("SELECT title, artist, album, duration, audio_path, cover_path, lyric_path, mv_path FROM songs ORDER BY title COLLATE NOCASE");
    } else {
        query.prepare(
            "SELECT title, artist, album, duration, audio_path, cover_path, lyric_path, mv_path FROM songs "
            "WHERE title LIKE :term OR artist LIKE :term OR album LIKE :term ORDER BY title COLLATE NOCASE");
        query.bindValue(":term", "%" + queryText + "%");
    }

    if (!query.exec()) {
        qWarning() << "Failed to search songs:" << query.lastError().text();
        emit searchResultsChanged();
        return;
    }

    while (query.next()) {
        QVariantMap record;
        record["title"] = sqlValue(query, "title");
        record["artist"] = sqlValue(query, "artist");
        record["album"] = sqlValue(query, "album");
        record["duration"] = sqlValue(query, "duration");
        record["audio_path"] = sqlValue(query, "audio_path");
        record["cover_path"] = sqlValue(query, "cover_path");
        record["lyric_path"] = sqlValue(query, "lyric_path");
        record["mv_path"] = sqlValue(query, "mv_path");
        QVariantMap item = songMapFromRecord(record);
        item["type"] = "song";
        item["subtitle"] = item["artist"];
        m_results.append(item);
    }

    emit searchResultsChanged();
}

QVariantList MusicManager::localArtists() const
{
    QMap<QString, QVariantMap> artists;

    for (const QVariant &item : m_localMusic) {
        const QVariantMap song = item.toMap();
        QString artist = song.value("artist").toString().trimmed();
        if (artist.isEmpty()) {
            artist = QStringLiteral("Unknown Artist");
        }

        QVariantMap artistItem = artists.value(artist);
        if (artistItem.isEmpty()) {
            artistItem["name"] = artist;
            artistItem["songCount"] = 0;
            artistItem["cover"] = song.value("cover").toString();
        }
        artistItem["songCount"] = artistItem.value("songCount").toInt() + 1;
        artists[artist] = artistItem;
    }

    QVariantList result;
    for (const QVariantMap &artist : artists) {
        result.append(artist);
    }
    return result;
}

void MusicManager::fetchArtistInfo(const QString &artist)
{
    const QString artistName = artist.trimmed();
    ++m_artistRequestToken;
    const int token = m_artistRequestToken;

    m_artistPendingRequests = 0;
    m_artistProfile.clear();
    m_artistTracks = localSongsForArtist(artistName);
    setArtistError(QString());

    if (artistName.isEmpty()) {
        setArtistLoading(false);
        emit artistProfileChanged();
        emit artistTracksChanged();
        return;
    }

    QString cover = defaultCoverUrl();
    if (!m_artistTracks.isEmpty()) {
        cover = m_artistTracks.first().toMap().value("cover").toString();
    }

    m_artistProfile = QVariantMap {
        {"name", artistName},
        {"title", artistName},
        {"subtitle", QStringLiteral("Online artist profile")},
        {"description", QStringLiteral("Loading artist biography, background image, covers, and songs from the network.")},
        {"avatar", cover},
        {"background", cover},
        {"source", QStringLiteral("Local + Wikipedia + Deezer")},
        {"localCount", m_artistTracks.size()},
        {"trackCount", m_artistTracks.size()}
    };
    emit artistProfileChanged();
    emit artistTracksChanged();

    setArtistLoading(true);
    m_artistPendingRequests = 2;

    const QString wikiTitle = QString::fromLatin1(QUrl::toPercentEncoding(artistName));
    QNetworkReply *wikiReply = getJson(QUrl(QStringLiteral("https://zh.wikipedia.org/api/rest_v1/page/summary/%1").arg(wikiTitle)));
    connect(wikiReply, &QNetworkReply::finished, this, [this, wikiReply, token]() {
        handleWikipediaSummary(wikiReply, token);
    });

    QUrl deezerUrl(QStringLiteral("https://api.deezer.com/search/artist"));
    QUrlQuery deezerQuery;
    deezerQuery.addQueryItem(QStringLiteral("q"), artistName);
    deezerQuery.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
    deezerUrl.setQuery(deezerQuery);
    QNetworkReply *deezerReply = getJson(deezerUrl);
    connect(deezerReply, &QNetworkReply::finished, this, [this, deezerReply, token, artistName]() {
        handleDeezerArtistSearch(deezerReply, token, artistName);
    });
}


QVariantMap MusicManager::songByTitle(const QString &title) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT title, artist, album, duration, audio_path, cover_path, lyric_path, mv_path FROM songs WHERE title = :title LIMIT 1");
    query.bindValue(":title", title);

    if (!query.exec() || !query.next()) {
        return {};
    }

    QVariantMap record;
    record["title"] = sqlValue(query, "title");
    record["artist"] = sqlValue(query, "artist");
    record["album"] = sqlValue(query, "album");
    record["duration"] = sqlValue(query, "duration");
    record["audio_path"] = sqlValue(query, "audio_path");
    record["cover_path"] = sqlValue(query, "cover_path");
    record["lyric_path"] = sqlValue(query, "lyric_path");
    record["mv_path"] = sqlValue(query, "mv_path");
    return songMapFromRecord(record);
}

QVariantMap MusicManager::songMapFromRecord(const QVariantMap &record) const
{
    const QString coverUrl = fileUrlForRelativePath(record.value("cover_path").toString());
    const QString mvPath = record.value("mv_path").toString();
    const QString lyricPath = record.value("lyric_path").toString();

    QVariantMap item;
    item["title"] = record.value("title").toString();
    item["artist"] = record.value("artist").toString();
    item["album"] = record.value("album").toString();
    item["duration"] = record.value("duration").toString();
    item["cover"] = coverUrl.isEmpty() ? defaultCoverUrl() : coverUrl;
    item["image"] = item["cover"];
    item["url"] = record.value("audio_path").toString();
    item["audio_path"] = record.value("audio_path").toString();
    item["lyric_path"] = lyricPath;
    item["mv_path"] = mvPath;
    item["mv"] = fileUrlForRelativePath(mvPath);
    return item;
}

void MusicManager::playTrack(const QString &title, const QString &artist, const QString &cover, const QString &url)
{
    QVariantMap song = songByTitle(title);
    const QString audioRelativePath = url.isEmpty() ? song.value("audio_path").toString() : url;
    const QString lyricRelativePath = song.value("lyric_path").toString();
    const QUrl remoteUrl(audioRelativePath);
    const bool isRemoteAudio = remoteUrl.isValid() &&
        (remoteUrl.scheme() == QStringLiteral("http") || remoteUrl.scheme() == QStringLiteral("https"));
    const QString audioPath = resolveRelativePath(audioRelativePath);

    m_title = title;
    m_artist = artist.isEmpty() ? song.value("artist").toString() : artist;
    m_cover = cover.isEmpty() ? song.value("cover").toString() : cover;
    if (m_cover.isEmpty()) {
        m_cover = defaultCoverUrl();
    }
    m_lyrics.clear();
    m_lyricsIndex = -1;

    emit lyricsChanged();
    emit currentLyricsIndexChanged();
    emit isCurrentLikedChanged();

    bool found = false;
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist[i].toMap().value("title").toString() == title) {
            m_currentIndex = i;
            found = true;
            break;
        }
    }
    if (!found) {
        m_playlist = m_localMusic;
        emit playlistChanged();
        for (int i = 0; i < m_playlist.size(); ++i) {
            if (m_playlist[i].toMap().value("title").toString() == title) {
                m_currentIndex = i;
                break;
            }
        }
    }

    if (isRemoteAudio || !audioPath.isEmpty()) {
        if (m_audioEffect != 0) {
            m_effectReceivedBuffer = false;
            setAudioEffectStatus(QStringLiteral("正在应用%1音效").arg(audioEffectName()));
        }
        resetEffectPipeline();
        m_player->setSource(isRemoteAudio ? remoteUrl : QUrl::fromLocalFile(audioPath));
        m_player->play();
        if (!isRemoteAudio && !lyricRelativePath.isEmpty()) {
            loadLyrics(lyricRelativePath);
        }
    } else {
        qWarning() << "Audio file is missing for" << title << "relative path:" << audioRelativePath;
        m_player->stop();
    }

    emit trackChanged();
}

void MusicManager::loadLyrics(const QString &relativePath)
{
    const QString path = resolveRelativePath(relativePath);
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QRegularExpression regex("\\[(\\d{2}):(\\d{2})\\.(\\d{2,3})\\](.*)");
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            int ms = match.captured(3).toInt();
            if (match.captured(3).length() == 2) {
                ms *= 10;
            }
            const qint64 t = match.captured(1).toInt() * 60000 + match.captured(2).toInt() * 1000 + ms;
            const QString text = match.captured(4).trimmed();
            if (!text.isEmpty()) {
                m_lyrics.append(QVariantMap{{"time", t}, {"text", text}});
            }
        } else if (!line.trimmed().isEmpty()) {
            m_lyrics.append(QVariantMap{{"time", qint64(0)}, {"text", line.trimmed()}});
        }
    }

    emit lyricsChanged();
}

void MusicManager::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        next();
    }
}

void MusicManager::next()
{
    if (m_playlist.isEmpty()) {
        return;
    }
    if (m_playbackMode == 1) {
        m_player->setPosition(0);
        m_player->play();
    } else if (m_playbackMode == 2) {
        playIndex(QRandomGenerator::global()->bounded(m_playlist.size()));
    } else {
        int nextIndex = m_currentIndex + 1;
        if (nextIndex >= m_playlist.size()) {
            nextIndex = 0;
        }
        playIndex(nextIndex);
    }
}

void MusicManager::previous()
{
    if (m_playlist.isEmpty()) {
        return;
    }
    int previousIndex = m_currentIndex - 1;
    if (previousIndex < 0) {
        previousIndex = m_playlist.size() - 1;
    }
    playIndex(previousIndex);
}

void MusicManager::playIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) {
        return;
    }
    const QVariantMap track = m_playlist[index].toMap();
    playTrack(
        track.value("title").toString(),
        track.value("artist").toString(),
        track.value("cover").toString(),
        track.value("url").toString());
}

void MusicManager::onPositionChanged(qint64 position)
{
    emit positionChanged();
    if (m_lyrics.isEmpty()) {
        return;
    }
    if (m_lyrics.last().toMap().value("time").toLongLong() == 0) {
        return;
    }

    int newIndex = -1;
    for (int i = 0; i < m_lyrics.size(); ++i) {
        if (position < m_lyrics[i].toMap().value("time").toLongLong()) {
            newIndex = i - 1;
            break;
        }
    }
    if (newIndex == -1 && position >= m_lyrics.last().toMap().value("time").toLongLong()) {
        newIndex = m_lyrics.size() - 1;
    }
    if (newIndex < 0) {
        newIndex = 0;
    }
    if (m_lyricsIndex != newIndex) {
        m_lyricsIndex = newIndex;
        emit currentLyricsIndexChanged();
    }
}

void MusicManager::togglePlayPause()
{
    m_player->playbackState() == QMediaPlayer::PlayingState ? m_player->pause() : m_player->play();
}

void MusicManager::stopPlayback()
{
    if (m_player->playbackState() == QMediaPlayer::StoppedState) {
        return;
    }

    m_player->stop();
    resetEffectPipeline();
    emit playbackStateChanged();
}

void MusicManager::seek(qint64 position)
{
    resetEffectPipeline();
    m_player->setPosition(position);
}

void MusicManager::setVolume(float volume)
{
    const float safeVolume = std::clamp(volume, 0.0f, 1.0f);
    m_audioOutput->setVolume(safeVolume);
    if (m_effectSink) {
        m_effectSink->setVolume(safeVolume);
    }
    emit volumeChanged();
}

QString MusicManager::audioEffectName() const
{
    switch (m_audioEffect) {
    case 1:
        return QStringLiteral("演唱会");
    case 2:
        return QStringLiteral("纯净人声");
    case 3:
        return QStringLiteral("纯伴奏");
    default:
        return QStringLiteral("标准");
    }
}

void MusicManager::setAudioEffect(int effect)
{
    const int safeEffect = std::clamp(effect, 0, 3);
    if (m_audioEffect == safeEffect) {
        return;
    }

    m_audioEffect = safeEffect;
    m_effectReceivedBuffer = false;
    resetEffectPipeline();
    m_audioOutput->setMuted(m_audioEffect != 0);

    if (m_audioEffect == 0) {
        m_effectDrainTimer->stop();
        setAudioEffectStatus(QStringLiteral("标准音质"));
    } else {
        m_effectDrainTimer->start();
        setAudioEffectStatus(QStringLiteral("正在应用%1音效").arg(audioEffectName()));
        if (m_player->playbackState() == QMediaPlayer::PlayingState) {
            QTimer::singleShot(1600, this, &MusicManager::verifyEffectPipeline);
        }
    }

    emit audioEffectChanged();
}

void MusicManager::onAudioBufferReceived(const QAudioBuffer &buffer)
{
    if (m_audioEffect == 0 || !buffer.isValid() || buffer.byteCount() == 0) {
        return;
    }

    if (!m_effectReceivedBuffer) {
        m_effectReceivedBuffer = true;
        setAudioEffectStatus(QStringLiteral("%1音效已开启").arg(audioEffectName()));
    }

    ensureEffectSink(buffer.format());
    if (!m_effectDevice) {
        return;
    }

    m_pendingEffectAudio.append(processAudioBuffer(buffer));
    flushEffectAudio();
}

void MusicManager::ensureEffectSink(const QAudioFormat &format)
{
    if (!format.isValid()) {
        return;
    }

    if (m_effectSink && m_effectFormat == format) {
        return;
    }

    if (m_effectSink) {
        m_effectSink->stop();
        delete m_effectSink;
        m_effectSink = nullptr;
        m_effectDevice = nullptr;
    }

    m_effectFormat = format;
    m_effectSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
    m_effectSink->setVolume(m_audioOutput->volume());
    m_effectSink->setBufferFrameCount(std::max(format.sampleRate() / 12, 2048));
    m_effectDevice = m_effectSink->start();

    if (m_player->playbackState() == QMediaPlayer::PausedState) {
        m_effectSink->suspend();
    }
}

void MusicManager::flushEffectAudio()
{
    if (m_audioEffect == 0 || !m_effectSink || !m_effectDevice ||
        m_pendingEffectAudio.isEmpty()) {
        return;
    }

    qsizetype writable = std::min(m_effectSink->bytesFree(), m_pendingEffectAudio.size());
    const int bytesPerFrame = m_effectFormat.bytesPerFrame();
    if (bytesPerFrame > 0) {
        writable -= writable % bytesPerFrame;
    }
    if (writable <= 0) {
        return;
    }

    const qint64 written = m_effectDevice->write(m_pendingEffectAudio.constData(), writable);
    if (written > 0) {
        m_pendingEffectAudio.remove(0, written);
    }
}

void MusicManager::resetEffectPipeline()
{
    m_pendingEffectAudio.clear();
    resetDspState();

    if (!m_effectSink) {
        return;
    }

    m_effectSink->reset();
    m_effectDevice = m_effectSink->start();
    if (m_player->playbackState() == QMediaPlayer::PausedState) {
        m_effectSink->suspend();
    }
}

void MusicManager::verifyEffectPipeline()
{
    if (m_audioEffect == 0 || m_effectReceivedBuffer ||
        m_player->playbackState() != QMediaPlayer::PlayingState) {
        return;
    }

    qWarning() << "Audio effect pipeline did not receive decoded buffers; falling back to normal playback.";
    m_audioEffect = 0;
    m_audioOutput->setMuted(false);
    m_effectDrainTimer->stop();
    resetEffectPipeline();
    setAudioEffectStatus(QStringLiteral("当前后端不支持实时音效，已切回标准"));
    emit audioEffectChanged();
}

void MusicManager::resetDspState()
{
    m_reverbLeft.clear();
    m_reverbRight.clear();
    m_reverbIndex = 0;
    m_dspSampleRate = 0;
    m_vocalPreviousInput = 0.0f;
    m_vocalPreviousOutput = 0.0f;
    m_vocalLowPass = 0.0f;
    m_lastConcertLeft = 0.0f;
    m_lastConcertRight = 0.0f;
}

void MusicManager::setAudioEffectStatus(const QString &status)
{
    if (m_audioEffectStatus == status) {
        return;
    }
    m_audioEffectStatus = status;
    emit audioEffectStatusChanged();
}

QByteArray MusicManager::processAudioBuffer(const QAudioBuffer &buffer)
{
    const QAudioFormat format = buffer.format();
    QByteArray output(buffer.constData<char>(), buffer.byteCount());
    const int channels = format.channelCount();
    if (channels <= 0 || format.bytesPerSample() <= 0) {
        return output;
    }

    if (m_dspSampleRate != format.sampleRate()) {
        resetDspState();
        m_dspSampleRate = format.sampleRate();
    }

    const char *input = buffer.constData<char>();
    char *processed = output.data();
    const qsizetype frames = buffer.frameCount();

    for (qsizetype frame = 0; frame < frames; ++frame) {
        const qsizetype base = frame * channels;
        const float left = readSample(input, base, format.sampleFormat());
        const float right = channels > 1
            ? readSample(input, base + 1, format.sampleFormat())
            : left;

        float outputLeft = left;
        float outputRight = right;

        if (m_audioEffect == 1) {
            outputLeft = processConcertSample(left, right, false);
            outputRight = processConcertSample(left, right, true);
            m_reverbIndex++;
            if (!m_reverbLeft.isEmpty() && m_reverbIndex >= m_reverbLeft.size()) {
                m_reverbIndex = 0;
            }
        } else if (m_audioEffect == 2) {
            const float vocal = processVocalSample(left, right);
            outputLeft = vocal;
            outputRight = vocal;
        } else if (m_audioEffect == 3) {
            const float side = (left - right) * 1.35f;
            outputLeft = side;
            outputRight = -side;
        }

        for (int channel = 0; channel < channels; ++channel) {
            const float value = channel % 2 == 0 ? outputLeft : outputRight;
            writeSample(processed, base + channel, format.sampleFormat(), value);
        }
    }

    return output;
}

float MusicManager::processConcertSample(float left, float right, bool rightChannel)
{
    if (m_reverbLeft.isEmpty()) {
        const int delayFrames = std::max(m_dspSampleRate / 3, 1);
        m_reverbLeft.fill(0.0f, delayFrames);
        m_reverbRight.fill(0.0f, delayFrames);
        m_reverbIndex = 0;
    }

    const int size = m_reverbLeft.size();
    const auto delayed = [this, size](const QVector<float> &line, float milliseconds) {
        const int offset = std::max(static_cast<int>(m_dspSampleRate * milliseconds / 1000.0f), 1);
        int index = m_reverbIndex - offset;
        while (index < 0) {
            index += size;
        }
        return line[index % size];
    };

    const float mid = (left + right) * 0.5f;
    const float side = (left - right) * 0.5f;
    const float wideLeft = mid + side * 1.2f;
    const float wideRight = mid - side * 1.2f;

    const float wetLeft = delayed(m_reverbLeft, 43.0f) * 0.30f
        + delayed(m_reverbRight, 79.0f) * 0.18f
        + delayed(m_reverbLeft, 137.0f) * 0.12f;
    const float wetRight = delayed(m_reverbRight, 47.0f) * 0.30f
        + delayed(m_reverbLeft, 83.0f) * 0.18f
        + delayed(m_reverbRight, 149.0f) * 0.12f;

    if (!rightChannel) {
        m_lastConcertLeft = clampAudio(wideLeft * 0.78f + wetLeft);
        m_reverbLeft[m_reverbIndex] = clampAudio(wideLeft + wetLeft * 0.32f);
        return m_lastConcertLeft;
    }

    m_lastConcertRight = clampAudio(wideRight * 0.78f + wetRight);
    m_reverbRight[m_reverbIndex] = clampAudio(wideRight + wetRight * 0.32f);
    return m_lastConcertRight;
}

float MusicManager::processVocalSample(float left, float right)
{
    const float centered = (left + right) * 0.5f;
    const float sampleRate = std::max(m_dspSampleRate, 1);
    const float dt = 1.0f / sampleRate;

    const float highPassRc = 1.0f / (2.0f * kPi * 110.0f);
    const float highPassAlpha = highPassRc / (highPassRc + dt);
    const float highPassed = highPassAlpha
        * (m_vocalPreviousOutput + centered - m_vocalPreviousInput);
    m_vocalPreviousInput = centered;
    m_vocalPreviousOutput = highPassed;

    const float lowPassRc = 1.0f / (2.0f * kPi * 12000.0f);
    const float lowPassAlpha = dt / (lowPassRc + dt);
    m_vocalLowPass += lowPassAlpha * (highPassed - m_vocalLowPass);

    return clampAudio(m_vocalLowPass * 1.28f);
}

void MusicManager::switchPlaybackMode()
{
    m_playbackMode++;
    if (m_playbackMode > 2) {
        m_playbackMode = 0;
    }
    emit playbackModeChanged();
}

void MusicManager::playFromQueue(int index)
{
    playIndex(index);
}

void MusicManager::removeFromQueue(int index)
{
    if (index < 0 || index >= m_playlist.size()) {
        return;
    }
    m_playlist.removeAt(index);
    if (index < m_currentIndex) {
        m_currentIndex--;
    }
    emit playlistChanged();
}

QString MusicManager::getMvPath(const QString &title)
{
    const QVariantMap song = songByTitle(title);
    return fileUrlForRelativePath(song.value("mv_path").toString());
}

void MusicManager::toggleLike(const QString &title, const QString &artist, const QString &cover, const QString &url)
{
    bool found = false;
    for (int i = 0; i < m_likedSongs.size(); ++i) {
        if (m_likedSongs[i].toMap().value("title").toString() == title) {
            m_likedSongs.removeAt(i);
            found = true;
            break;
        }
    }
    if (!found) {
        m_likedSongs.append(QVariantMap{{"title", title}, {"artist", artist}, {"cover", cover}, {"url", url}});
    }
    emit isCurrentLikedChanged();
}

bool MusicManager::isCurrentLiked() const
{
    for (const auto &item : m_likedSongs) {
        if (item.toMap().value("title").toString() == m_title) {
            return true;
        }
    }
    return false;
}

void MusicManager::createPlaylist(const QString &name)
{
    m_playlists.append(QVariantMap{{"name", name}});
}

void MusicManager::addToPlaylist(int, const QString &, const QString &, const QString &, const QString &)
{
}

void MusicManager::removePlaylist(int)
{
}

void MusicManager::setArtistLoading(bool loading)
{
    if (m_artistLoading == loading) {
        return;
    }
    m_artistLoading = loading;
    emit artistLoadingChanged();
}

void MusicManager::setArtistError(const QString &error)
{
    if (m_artistError == error) {
        return;
    }
    m_artistError = error;
    emit artistErrorChanged();
}

void MusicManager::finishArtistRequest(int token)
{
    if (token != m_artistRequestToken) {
        return;
    }
    m_artistPendingRequests = std::max(0, m_artistPendingRequests - 1);
    if (m_artistPendingRequests == 0) {
        setArtistLoading(false);
        if (m_artistTracks.isEmpty() && m_artistError.isEmpty()) {
            setArtistError(QStringLiteral("No online songs were found for this artist."));
        }
    }
}

QNetworkReply *MusicManager::getJson(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QTmusicplayer/1.0"));
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return m_network->get(request);
}

void MusicManager::fetchDeezerTopTracks(int token, const QString &artistName, int artistId)
{
    QUrl topUrl(QStringLiteral("https://api.deezer.com/artist/%1/top").arg(artistId));
    QUrlQuery topQuery;
    topQuery.addQueryItem(QStringLiteral("limit"), QStringLiteral("30"));
    topUrl.setQuery(topQuery);

    QNetworkReply *reply = getJson(topUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply, token, artistName]() {
        handleDeezerTopTracks(reply, token, artistName);
    });
}

void MusicManager::handleWikipediaSummary(QNetworkReply *reply, int token)
{
    const QByteArray payload = reply->readAll();
    const bool networkOk = reply->error() == QNetworkReply::NoError;
    reply->deleteLater();

    if (token != m_artistRequestToken) {
        return;
    }
    if (!networkOk) {
        finishArtistRequest(token);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        finishArtistRequest(token);
        return;
    }

    const QJsonObject root = document.object();
    const QString extract = root.value(QStringLiteral("extract")).toString().trimmed();
    const QString description = root.value(QStringLiteral("description")).toString().trimmed();
    const QJsonObject originalImage = root.value(QStringLiteral("originalimage")).toObject();
    const QJsonObject thumbnail = root.value(QStringLiteral("thumbnail")).toObject();
    const QJsonObject contentUrls = root.value(QStringLiteral("content_urls")).toObject();
    const QJsonObject desktopUrls = contentUrls.value(QStringLiteral("desktop")).toObject();

    if (!extract.isEmpty()) {
        m_artistProfile["description"] = extract;
    }
    if (!description.isEmpty()) {
        m_artistProfile["subtitle"] = description;
    }
    const QString image = originalImage.value(QStringLiteral("source")).toString(
        thumbnail.value(QStringLiteral("source")).toString());
    if (!image.isEmpty()) {
        m_artistProfile["background"] = image;
        m_artistProfile["avatar"] = image;
    }
    const QString page = desktopUrls.value(QStringLiteral("page")).toString();
    if (!page.isEmpty()) {
        m_artistProfile["wikiUrl"] = page;
    }

    emit artistProfileChanged();
    finishArtistRequest(token);
}

void MusicManager::handleDeezerArtistSearch(QNetworkReply *reply, int token, const QString &artistName)
{
    const QByteArray payload = reply->readAll();
    const bool networkOk = reply->error() == QNetworkReply::NoError;
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (token != m_artistRequestToken) {
        return;
    }
    if (!networkOk) {
        setArtistError(QStringLiteral("Song list request failed: %1").arg(errorText));
        finishArtistRequest(token);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    const QJsonArray data = document.object().value(QStringLiteral("data")).toArray();
    if (parseError.error != QJsonParseError::NoError || data.isEmpty()) {
        setArtistError(QStringLiteral("No online songs were found for this artist."));
        finishArtistRequest(token);
        return;
    }

    const QJsonObject artist = data.first().toObject();
    const int artistId = artist.value(QStringLiteral("id")).toInt();
    const QString foundName = artist.value(QStringLiteral("name")).toString(artistName);
    const QString picture = artist.value(QStringLiteral("picture_xl")).toString(
        artist.value(QStringLiteral("picture_big")).toString());

    if (!foundName.isEmpty()) {
        m_artistProfile["name"] = foundName;
        m_artistProfile["title"] = foundName;
    }
    if (!picture.isEmpty()) {
        m_artistProfile["avatar"] = picture;
        if (m_artistProfile.value("background").toString().isEmpty() ||
            m_artistProfile.value("background").toString() == defaultCoverUrl()) {
            m_artistProfile["background"] = picture;
        }
    }
    m_artistProfile["deezerUrl"] = artist.value(QStringLiteral("link")).toString();
    m_artistProfile["albumCount"] = artist.value(QStringLiteral("nb_album")).toInt();
    m_artistProfile["fanCount"] = artist.value(QStringLiteral("nb_fan")).toInt();
    emit artistProfileChanged();

    if (artistId <= 0) {
        finishArtistRequest(token);
        return;
    }
    fetchDeezerTopTracks(token, foundName, artistId);
}

void MusicManager::handleDeezerTopTracks(QNetworkReply *reply, int token, const QString &artistName)
{
    const QByteArray payload = reply->readAll();
    const bool networkOk = reply->error() == QNetworkReply::NoError;
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (token != m_artistRequestToken) {
        return;
    }
    if (!networkOk) {
        setArtistError(QStringLiteral("Song list request failed: %1").arg(errorText));
        finishArtistRequest(token);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        finishArtistRequest(token);
        return;
    }

    QSet<QString> seen;
    QVariantList combined;
    for (const QVariant &item : m_artistTracks) {
        const QVariantMap song = item.toMap();
        seen.insert(song.value("title").toString().toCaseFolded());
        combined.append(song);
    }

    const QJsonArray tracks = document.object().value(QStringLiteral("data")).toArray();
    for (const QJsonValue &value : tracks) {
        const QJsonObject track = value.toObject();
        const QString title = track.value(QStringLiteral("title")).toString().trimmed();
        if (title.isEmpty() || seen.contains(title.toCaseFolded())) {
            continue;
        }

        const QJsonObject album = track.value(QStringLiteral("album")).toObject();
        const QJsonObject artist = track.value(QStringLiteral("artist")).toObject();
        QVariantMap song;
        song["title"] = title;
        song["artist"] = artist.value(QStringLiteral("name")).toString(artistName);
        song["album"] = album.value(QStringLiteral("title")).toString(QStringLiteral("Single"));
        song["duration"] = formatDuration(track.value(QStringLiteral("duration")).toInt());
        song["cover"] = album.value(QStringLiteral("cover_xl")).toString(
            album.value(QStringLiteral("cover_big")).toString(defaultCoverUrl()));
        song["image"] = song["cover"];
        song["url"] = track.value(QStringLiteral("preview")).toString();
        song["preview"] = song["url"];
        song["externalUrl"] = track.value(QStringLiteral("link")).toString();
        song["source"] = QStringLiteral("Deezer");
        song["playable"] = !song.value("url").toString().isEmpty();
        combined.append(song);
        seen.insert(title.toCaseFolded());
    }

    m_artistTracks = combined;
    m_artistProfile["trackCount"] = m_artistTracks.size();
    if (!m_artistTracks.isEmpty()) {
        const QString cover = m_artistTracks.first().toMap().value("cover").toString();
        if (!cover.isEmpty() && (m_artistProfile.value("background").toString().isEmpty() ||
                                m_artistProfile.value("background").toString() == defaultCoverUrl())) {
            m_artistProfile["background"] = cover;
        }
    }
    emit artistTracksChanged();
    emit artistProfileChanged();
    finishArtistRequest(token);
}

QVariantList MusicManager::localSongsForArtist(const QString &artist) const
{
    QVariantList songs;
    const QString normalized = artist.trimmed();
    if (normalized.isEmpty()) {
        return songs;
    }

    for (const QVariant &item : m_localMusic) {
        const QVariantMap song = item.toMap();
        if (song.value("artist").toString().compare(normalized, Qt::CaseInsensitive) == 0) {
            QVariantMap localSong = song;
            localSong["source"] = QStringLiteral("Local");
            localSong["playable"] = true;
            songs.append(localSong);
        }
    }
    return songs;
}


QString MusicManager::libraryRootPath() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();
    const QStringList candidates = {
        appDir,
        QDir(appDir).absoluteFilePath(".."),
        QDir(appDir).absoluteFilePath("../.."),
        QDir(appDir).absoluteFilePath("../../.."),
        currentDir,
        QDir(currentDir).absoluteFilePath("AppleMusic")
    };

    for (const QString &candidate : candidates) {
        const QString clean = QDir(candidate).absolutePath();
        QDir dir(clean);
        if (dir.exists("media") || dir.exists("data") || dir.exists("MusicManager.cpp")) {
            return clean;
        }
    }

    return QDir(appDir).absolutePath();
}

QString MusicManager::mediaRootPath() const
{
    return QDir(libraryRootPath()).absoluteFilePath("media");
}

QString MusicManager::databasePath() const
{
    return QDir(libraryRootPath()).absoluteFilePath("data/music_library.db");
}

QString MusicManager::resolveRelativePath(const QString &relativePath) const
{
    if (relativePath.trimmed().isEmpty() || QDir::isAbsolutePath(relativePath)) {
        return {};
    }

    const QString cleanedRelative = normalizeSeparators(QDir::cleanPath(relativePath));
    const QString absolutePath = QDir(libraryRootPath()).absoluteFilePath(cleanedRelative);
    const QFileInfo info(absolutePath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }
    return QDir::cleanPath(info.absoluteFilePath());
}

QString MusicManager::relativePathForFile(const QString &absolutePath) const
{
    if (absolutePath.isEmpty()) {
        return {};
    }
    const QFileInfo info(absolutePath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    const QString relative = QDir(libraryRootPath()).relativeFilePath(info.absoluteFilePath());
    if (relative.startsWith("..")) {
        return {};
    }
    return normalizeSeparators(QDir::cleanPath(relative));
}

QString MusicManager::fileUrlForRelativePath(const QString &relativePath) const
{
    const QString absolutePath = resolveRelativePath(relativePath);
    if (absolutePath.isEmpty()) {
        return {};
    }
    return QUrl::fromLocalFile(absolutePath).toString();
}

QString MusicManager::findMediaFile(const QString &title, const QStringList &extensions) const
{
    QDir mediaDir(mediaRootPath());
    if (!mediaDir.exists()) {
        return {};
    }

    for (const QString &extension : extensions) {
        const QString exact = mediaDir.absoluteFilePath(title + "." + extension);
        if (QFileInfo::exists(exact)) {
            return exact;
        }
    }

    const QFileInfoList files = mediaDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        if (fileInfo.completeBaseName().compare(title, Qt::CaseInsensitive) != 0) {
            continue;
        }
        for (const QString &extension : extensions) {
            if (fileInfo.suffix().compare(extension, Qt::CaseInsensitive) == 0) {
                return fileInfo.absoluteFilePath();
            }
        }
    }

    return {};
}

QString MusicManager::defaultCoverUrl() const
{
    return "qrc:/default_cover.svg";
}

QString MusicManager::guessArtist(const QString &title) const
{
    const QString lower = title.toLower();
    if (lower == "yellow") {
        return "Coldplay";
    }
    if (lower == "closer" || lower.contains("something")) {
        return "The Chainsmokers";
    }
    if (title.contains(QStringLiteral("我是如此相信")) ||
        title.contains(QStringLiteral("星球大战")) ||
        title.contains(QStringLiteral("威廉古堡"))) {
        return QStringLiteral("周杰伦");
    }
    if (lower.contains("last christmas") || lower.contains("willow") || lower.contains("ophelia") ||
        lower.contains("anti-hero") || lower.contains("lavender") || lower.contains("karma") ||
        lower.contains("love story")) {
        return "Taylor Swift";
    }
    return "Unknown Artist";
}

QString MusicManager::guessAlbum(const QString &title) const
{
    const QString lower = title.toLower();
    if (lower == "last christmas") {
        return "Holiday Collection";
    }
    if (lower == "yellow") {
        return "Parachutes";
    }
    if (lower == "closer") {
        return "Collage";
    }
    if (lower.contains("something just like this")) {
        return "Memories...Do Not Open";
    }
    if (title.contains(QStringLiteral("我是如此相信"))) {
        return QStringLiteral("我是如此相信");
    }
    if (lower == "willow") {
        return "evermore";
    }
    if (lower.contains("anti-hero") || lower.contains("lavender") || lower.contains("karma")) {
        return "Midnights";
    }
    if (lower.contains("love story")) {
        return "Fearless";
    }
    if (lower.contains("ophelia")) {
        return "Unreleased";
    }
    return "Local Music";
}

QString MusicManager::guessDuration(const QString &title) const
{
    const QString lower = title.toLower();
    if (lower == "last christmas") return "3:29";
    if (lower == "willow") return "3:34";
    if (lower.contains("ophelia")) return "3:40";
    if (lower == "anti-hero") return "3:20";
    if (lower.contains("lavender")) return "3:22";
    if (lower == "karma") return "3:24";
    if (lower.contains("love story")) return "3:55";
    return "3:30";
}
