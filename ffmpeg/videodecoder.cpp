#include "videodecoder.h"
#include "avcontextinfo.h"
#include "decodervideoframe.h"
#include "videoformat.hpp"

#include <QDebug>

namespace Ffmpeg {

class VideoDecoder::VideoDecoderPrivate
{
public:
    VideoDecoderPrivate(QObject *parent)
        : owner(parent)
    {
        decoderVideoFrame = new DecoderVideoFrame(owner);
    }

    QObject *owner;

    DecoderVideoFrame *decoderVideoFrame;
};

VideoDecoder::VideoDecoder(QObject *parent)
    : Decoder<Packet *>(parent)
    , d_ptr(new VideoDecoderPrivate(this))
{}

VideoDecoder::~VideoDecoder() {}

void VideoDecoder::pause(bool state)
{
    d_ptr->decoderVideoFrame->pause(state);
}

void VideoDecoder::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->decoderVideoFrame->setVideoRenders(videoRenders);
}

void VideoDecoder::runDecoder()
{
    d_ptr->decoderVideoFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        if (m_seek) {
            clear();
            d_ptr->decoderVideoFrame->seek(m_seekTime, m_latchPtr.lock());
            seekFinish();
        }

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());
        if (packetPtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        auto frames(m_contextInfo->decodeFrame(packetPtr.data()));
        for (auto frame : frames) {
            Ffmpeg::calculateTime(frame, m_contextInfo, m_formatContext);
            d_ptr->decoderVideoFrame->append(frame);
        }

        while (m_runing && d_ptr->decoderVideoFrame->size() > Max_Frame_Size && !m_seek) {
            msleep(Sleep_Queue_Full_Milliseconds);
        }
    }
    while (m_runing && d_ptr->decoderVideoFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }
    d_ptr->decoderVideoFrame->stopDecoder();
}

} // namespace Ffmpeg
