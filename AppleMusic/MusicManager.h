#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QVariantList>

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

    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QVariantList lyricsModel READ lyricsModel NOTIFY lyricsChanged)
    Q_PROPERTY(int currentLyricsIndex READ currentLyricsIndex NOTIFY currentLyricsIndexChanged)

    Q_PROPERTY(QVariantList playlist READ playlist NOTIFY playlistChanged)
    Q_PROPERTY(int playbackMode READ playbackMode NOTIFY playbackModeChanged)

    Q_PROPERTY(QVariantList localMusic READ localMusic NOTIFY localMusicChanged)
    Q_PROPERTY(bool isCurrentLiked READ isCurrentLiked NOTIFY isCurrentLikedChanged)

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
    QVariantList searchResults() const { return m_results; }
    QVariantList lyricsModel() const { return m_lyrics; }
    int currentLyricsIndex() const { return m_lyricsIndex; }
    QVariantList playlist() const { return m_playlist; }
    int playbackMode() const { return m_playbackMode; }
    QVariantList localMusic() const { return m_localMusic; }
    bool isCurrentLiked() const;

    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void playTrack(const QString &title, const QString &artist, const QString &cover, const QString &url);
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void seek(qint64 position);
    void setVolume(float volume);

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
    void localMusicChanged();
    void isCurrentLikedChanged();

private slots:
    void onPositionChanged(qint64 position);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

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

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
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

    int m_currentIndex = -1;
    int m_playbackMode = 0;
    int m_lyricsIndex = -1;
};

#endif // MUSICMANAGER_H
