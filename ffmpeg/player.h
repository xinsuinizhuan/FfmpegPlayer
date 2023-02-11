#ifndef PLAYER_H
#define PLAYER_H

#include "ffmepg_global.h"

#include <QThread>

namespace Ffmpeg {

class AVError;
class AVContextInfo;
class VideoRender;

class FFMPEG_EXPORT Player : public QThread
{
    Q_OBJECT
public:
    enum MediaState { StoppedState, PlayingState, PausedState };

    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    QString &filePath() const;

    bool isOpen();

    void setVolume(qreal volume);

    void setSpeed(double speed);
    double speed();

    void pause(bool status = true);

    void setUseGpuDecode(bool on);
    bool isGpuDecode();

    MediaState mediaState();

    qint64 duration() const; // ms
    qint64 position() const; // ms
    qint64 fames() const;
    QSize resolutionRatio() const;
    double fps() const;

    int audioIndex() const;
    int videoIndex() const;
    int subtitleIndex() const;

    void setVideoRenders(QVector<VideoRender *> videoRenders);
    QVector<VideoRender *> videoRenders();

public slots:
    void onSetFilePath(const QString &filepath);
    void onPlay();
    void onStop();
    void onSeek(int timestamp); // s
    void onSetAudioTracks(const QString &text);
    void onSetSubtitleStream(const QString &text);

signals:
    void durationChanged(qint64 duration); // ms
    void positionChanged(qint64 position); // ms
    void stateChanged(Ffmpeg::Player::MediaState);
    void audioTracksChanged(const QStringList &tracks);
    void audioTrackChanged(const QString &track);
    void subTracksChanged(const QStringList &streams);
    void subTrackChanged(const QString &stream);
    void error(const Ffmpeg::AVError &avError);

    void playStarted();
    void seekFinished();

protected:
    void run() override;

private:
    void buildConnect(bool state = true);
    void buildErrorConnect();
    bool initAvCodec();
    void playVideo();
    void checkSeek();
    void setMediaState(MediaState mediaState);
    bool setMediaIndex(AVContextInfo *contextInfo, int index);

    class PlayerPrivate;
    QScopedPointer<PlayerPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PLAYER_H
