#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "decoderaudioframe.h"

#include <QDebug>

namespace Ffmpeg {

class AudioDecoder::AudioDecoderPrivate
{
public:
    AudioDecoderPrivate(QObject *parent)
        : owner(parent)
    {
        decoderAudioFrame = new DecoderAudioFrame(owner);
    }
    QObject *owner;
    DecoderAudioFrame *decoderAudioFrame;
};

AudioDecoder::AudioDecoder(QObject *parent)
    : Decoder<Packet *>(parent)
    , d_ptr(new AudioDecoderPrivate(this))
{
    connect(d_ptr->decoderAudioFrame,
            &DecoderAudioFrame::positionChanged,
            this,
            &AudioDecoder::positionChanged);
}

AudioDecoder::~AudioDecoder() {}

void AudioDecoder::pause(bool state)
{
    d_ptr->decoderAudioFrame->pause(state);
}

void AudioDecoder::setVolume(qreal volume)
{
    d_ptr->decoderAudioFrame->setVolume(volume);
}

void AudioDecoder::setSpeed(double speed)
{
    Decoder<Packet *>::setSpeed(speed);
    d_ptr->decoderAudioFrame->setSpeed(speed);
}

void AudioDecoder::setIsLocalFile(bool isLocalFile)
{
    d_ptr->decoderAudioFrame->setIsLocalFile(isLocalFile);
}

void AudioDecoder::runDecoder()
{
    d_ptr->decoderAudioFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        if (m_seek) {
            clear();
            d_ptr->decoderAudioFrame->seek(m_seekTime, m_latchPtr.lock());
            seekFinish();
        }

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());
        if (packetPtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        if (!m_contextInfo->sendPacket(packetPtr.data())) {
            continue;
        }

        std::unique_ptr<Frame> framePtr(new Frame);
        while (m_contextInfo->receiveFrame(framePtr.get())) { // 一个packet 一个或多个音频帧
            Ffmpeg::calculateTime(framePtr.get(), m_contextInfo, m_formatContext);
            d_ptr->decoderAudioFrame->append(framePtr.release());
            framePtr.reset(new Frame);
        }

        while (m_runing && d_ptr->decoderAudioFrame->size() > Max_Frame_Size && !m_seek) {
            msleep(Sleep_Queue_Full_Milliseconds);
        }
    }
    while (m_runing && d_ptr->decoderAudioFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }

    d_ptr->decoderAudioFrame->stopDecoder();
}

} // namespace Ffmpeg
