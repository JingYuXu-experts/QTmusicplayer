#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QByteArray>
#include <QMediaPlayer>
#include <QObject>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class QAudioBuffer;
class QAudioBufferOutput;
class QAudioSink;
class QIODevice;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class MusicManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY trackChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY trackChanged)
    Q_PROPERTY(QString currentCover READ currentCover NOTIFY trackChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackStateChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int audioEffect READ audioEffect WRITE setAudioEffect NOTIFY audioEffectChanged)
    Q_PROPERTY(QString audioEffectName READ audioEffectName NOTIFY audioEffectChanged)
    Q_PROPERTY(QString audioEffectStatus READ audioEffectStatus NOTIFY audioEffectStatusChanged)

    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QVariantList lyricsModel READ lyricsModel NOTIFY lyricsChanged)
    Q_PROPERTY(int currentLyricsIndex READ currentLyricsIndex NOTIFY currentLyricsIndexChanged)

    Q_PROPERTY(QVariantList playlist READ playlist NOTIFY playlistChanged)
    Q_PROPERTY(int playbackMode READ playbackMode NOTIFY playbackModeChanged)

    Q_PROPERTY(QVariantList localMusic READ localMusic NOTIFY localMusicChanged)
    Q_PROPERTY(bool isCurrentLiked READ isCurrentLiked NOTIFY isCurrentLikedChanged)
    Q_PROPERTY(QVariantMap artistProfile READ artistProfile NOTIFY artistProfileChanged)
    Q_PROPERTY(QVariantList artistTracks READ artistTracks NOTIFY artistTracksChanged)
    Q_PROPERTY(bool artistLoading READ artistLoading NOTIFY artistLoadingChanged)
    Q_PROPERTY(QString artistError READ artistError NOTIFY artistErrorChanged)

public:
    explicit MusicManager(QObject *parent = nullptr);
    ~MusicManager() override;

    QString currentTitle() const { return m_title; }
    QString currentArtist() const { return m_artist; }
    QString currentCover() const { return m_cover; }
    bool isPlaying() const { return m_player->playbackState() == QMediaPlayer::PlayingState; }
    qint64 duration() const { return m_player->duration(); }
    qint64 position() const { return m_player->position(); }
    float volume() const { return m_audioOutput->volume(); }
    int audioEffect() const { return m_audioEffect; }
    QString audioEffectName() const;
    QString audioEffectStatus() const { return m_audioEffectStatus; }
    QVariantList searchResults() const { return m_results; }
    QVariantList lyricsModel() const { return m_lyrics; }
    int currentLyricsIndex() const { return m_lyricsIndex; }
    QVariantList playlist() const { return m_playlist; }
    int playbackMode() const { return m_playbackMode; }
    QVariantList localMusic() const { return m_localMusic; }
    bool isCurrentLiked() const;
    QVariantMap artistProfile() const { return m_artistProfile; }
    QVariantList artistTracks() const { return m_artistTracks; }
    bool artistLoading() const { return m_artistLoading; }
    QString artistError() const { return m_artistError; }

    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void fetchArtistInfo(const QString &artist);
    Q_INVOKABLE QVariantList localArtists() const;
    Q_INVOKABLE void playTrack(const QString &title, const QString &artist, const QString &cover, const QString &url);
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void seek(qint64 position);
    void setVolume(float volume);
    void setAudioEffect(int effect);

    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void switchPlaybackMode();
    Q_INVOKABLE void playFromQueue(int index);
    Q_INVOKABLE void removeFromQueue(int index);

    Q_INVOKABLE void refreshLocalMusic();
    Q_INVOKABLE QString getMvPath(const QString &title);

    Q_INVOKABLE void toggleLike(const QString &title, const QString &artist, const QString &cover, const QString &url);
    Q_INVOKABLE void createPlaylist(const QString &name);
    Q_INVOKABLE void addToPlaylist(int index, const QString &title, const QString &artist, const QString &cover, const QString &url);
    Q_INVOKABLE void removePlaylist(int index);

signals:
    void trackChanged();
    void playbackStateChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void searchResultsChanged();
    void lyricsChanged();
    void currentLyricsIndexChanged();
    void playlistChanged();
    void playbackModeChanged();
    void audioEffectChanged();
    void audioEffectStatusChanged();
    void localMusicChanged();
    void isCurrentLikedChanged();
    void artistProfileChanged();
    void artistTracksChanged();
    void artistLoadingChanged();
    void artistErrorChanged();

private slots:
    void onPositionChanged(qint64 position);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onAudioBufferReceived(const QAudioBuffer &buffer);
    void flushEffectAudio();
    void verifyEffectPipeline();

private:
    bool openDatabase();
    bool ensureSchema();
    void ensureLibraryIsCurrent();
    void rebuildDatabaseFromMedia();
    void repairMissingPaths();

    QVariantMap songByTitle(const QString &title) const;
    QVariantMap songMapFromRecord(const QVariantMap &record) const;
    void loadLyrics(const QString &relativePath);
    void playIndex(int index);

    QString libraryRootPath() const;
    QString mediaRootPath() const;
    QString databasePath() const;
    QString resolveRelativePath(const QString &relativePath) const;
    QString relativePathForFile(const QString &absolutePath) const;
    QString fileUrlForRelativePath(const QString &relativePath) const;
    QString findMediaFile(const QString &title, const QStringList &extensions) const;
    QString defaultCoverUrl() const;

    QString guessArtist(const QString &title) const;
    QString guessAlbum(const QString &title) const;
    QString guessDuration(const QString &title) const;

    void setArtistLoading(bool loading);
    void setArtistError(const QString &error);
    void finishArtistRequest(int token);
    QNetworkReply *getJson(const QUrl &url);
    void fetchDeezerTopTracks(int token, const QString &artistName, int artistId);
    void handleWikipediaSummary(QNetworkReply *reply, int token);
    void handleDeezerArtistSearch(QNetworkReply *reply, int token, const QString &artistName);
    void handleDeezerTopTracks(QNetworkReply *reply, int token, const QString &artistName);
    QVariantList localSongsForArtist(const QString &artist) const;

    void ensureEffectSink(const QAudioFormat &format);
    void resetEffectPipeline();
    void resetDspState();
    void setAudioEffectStatus(const QString &status);
    QByteArray processAudioBuffer(const QAudioBuffer &buffer);
    float processConcertSample(float left, float right, bool rightChannel);
    float processVocalSample(float left, float right);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QAudioBufferOutput *m_audioBufferOutput = nullptr;
    QAudioSink *m_effectSink = nullptr;
    QIODevice *m_effectDevice = nullptr;
    QTimer *m_effectDrainTimer = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QAudioFormat m_effectFormat;
    QByteArray m_pendingEffectAudio;
    QSqlDatabase m_db;
    QString m_connectionName;

    QString m_title = "Ready";
    QString m_artist = "Select a song";
    QString m_cover = "";

    QVariantList m_results;
    QVariantList m_lyrics;
    QVariantList m_localMusic;
    QVariantList m_playlist;
    QVariantList m_likedSongs;
    QVariantList m_playlists;
    QVariantMap m_artistProfile;
    QVariantList m_artistTracks;
    QString m_artistError;

    int m_currentIndex = -1;
    int m_playbackMode = 0;
    int m_lyricsIndex = -1;
    int m_audioEffect = 0;
    int m_artistRequestToken = 0;
    int m_artistPendingRequests = 0;
    bool m_artistLoading = false;
    bool m_effectReceivedBuffer = false;
    QString m_audioEffectStatus = QStringLiteral("标准音质");

    QVector<float> m_reverbLeft;
    QVector<float> m_reverbRight;
    int m_reverbIndex = 0;
    int m_dspSampleRate = 0;
    float m_vocalPreviousInput = 0.0f;
    float m_vocalPreviousOutput = 0.0f;
    float m_vocalLowPass = 0.0f;
    float m_lastConcertLeft = 0.0f;
    float m_lastConcertRight = 0.0f;
};

#endif // MUSICMANAGER_H
