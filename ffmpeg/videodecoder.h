#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoder.h"
#include "packet.h"

struct AVFrame;

namespace Ffmpeg {

class VideoRender;

class VideoDecoder : public Decoder<Packet *>
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    void pause(bool state) override;

    void setVideoRenders(QVector<VideoRender *> videoRenders);

protected:
    void runDecoder() override;

private:
    class VideoDecoderPrivate;
    QScopedPointer<VideoDecoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEODECODER_H
