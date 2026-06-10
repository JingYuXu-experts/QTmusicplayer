#include "MusicManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSet>
#include <QStringConverter>
#include <QTextStream>
#include <QUrl>

namespace {
const char *kDatabaseConnectionPrefix = "music_library_";

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
}

MusicManager::MusicManager(QObject *parent) : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(1.0);

    connect(m_player, &QMediaPlayer::positionChanged, this, &MusicManager::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MusicManager::durationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MusicManager::playbackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MusicManager::onMediaStatusChanged);

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
    if (!select.exec("SELECT id, title, audio_path, cover_path, lyric_path, mv_path FROM songs")) {
        qWarning() << "Failed to validate songs:" << select.lastError().text();
        return;
    }

    QSet<QString> knownTitles;
    while (select.next()) {
        const int id = sqlIntValue(select, "id");
        const QString title = sqlValue(select, "title");
        knownTitles.insert(title.toCaseFolded());
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

        if (!changed) {
            continue;
        }

        QSqlQuery update(m_db);
        update.prepare(
            "UPDATE songs SET audio_path = :audio_path, cover_path = :cover_path, lyric_path = :lyric_path, "
            "mv_path = :mv_path, updated_at = :updated_at WHERE id = :id");
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

    if (!audioPath.isEmpty()) {
        m_player->setSource(QUrl::fromLocalFile(audioPath));
        m_player->play();
        if (!lyricRelativePath.isEmpty()) {
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

void MusicManager::seek(qint64 position)
{
    m_player->setPosition(position);
}

void MusicManager::setVolume(float volume)
{
    m_audioOutput->setVolume(volume);
    emit volumeChanged();
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
