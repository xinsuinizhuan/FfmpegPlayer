#include "frame.hpp"
#include "averrormanager.hpp"
#include "videoformat.hpp"

#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

namespace Ffmpeg {

class Frame::FramePrivate
{
public:
    explicit FramePrivate(Frame *q)
        : q_ptr(q)
    {}
    ~FramePrivate() { destroyFrame(); }

    void destroyFrame()
    {
        if (frame) {
            freeImageAlloc();
            av_frame_unref(frame);
        }
    }

    void freeImageAlloc()
    {
        if (imageAlloc) {
            av_freep(&frame->data[0]);
            imageAlloc = false;
        }
    }

    Frame *q_ptr;

    AVFrame *frame = nullptr;
    bool imageAlloc = false;
};

Frame::Frame()
    : d_ptr(new FramePrivate(this))
{
    d_ptr->frame = av_frame_alloc();
}

Frame::Frame(const Frame &other)
    : d_ptr(new FramePrivate(this))
{
    d_ptr->frame = av_frame_alloc();
    av_frame_ref(d_ptr->frame, other.d_ptr->frame);
}

Frame::Frame(Frame &&other) noexcept
    : d_ptr(new FramePrivate(this))
{
    d_ptr->frame = other.d_ptr->frame;
    other.d_ptr->frame = nullptr;
}

Frame::~Frame() = default;

auto Frame::operator=(const Frame &other) -> Frame &
{
    if (this != &other) {
        d_ptr->freeImageAlloc();
        av_frame_unref(d_ptr->frame);
        av_frame_ref(d_ptr->frame, other.d_ptr->frame);
    }

    return *this;
}

auto Frame::operator=(Frame &&other) noexcept -> Frame &
{
    if (this != &other) {
        d_ptr->freeImageAlloc();
        d_ptr->frame = other.d_ptr->frame;
        other.d_ptr->frame = nullptr;
    }

    return *this;
}

void Frame::copyPropsFrom(Frame *src)
{
    Q_ASSERT(src != nullptr);
    auto *srcFrame = src->avFrame();
    Q_ASSERT(srcFrame != nullptr);
    Q_ASSERT(d_ptr->frame != nullptr);

    av_frame_copy_props(d_ptr->frame, srcFrame);
    d_ptr->frame->pts = srcFrame->pts;
    d_ptr->frame->pkt_duration = srcFrame->pkt_duration;
    d_ptr->frame->pict_type = srcFrame->pict_type;
    d_ptr->frame->key_frame = srcFrame->key_frame;
    d_ptr->frame->width = srcFrame->width;
    d_ptr->frame->height = srcFrame->height;
}

auto Frame::imageAlloc(const QSize &size, AVPixelFormat pix_fmt, int align) -> bool
{
    Q_ASSERT(d_ptr->frame != nullptr);
    Q_ASSERT(size.width() > 0);
    Q_ASSERT(size.height() > 0);
    Q_ASSERT(pix_fmt != AV_PIX_FMT_NONE);

    auto ret = av_image_alloc(d_ptr->frame->data,
                              d_ptr->frame->linesize,
                              size.width(),
                              size.height(),
                              pix_fmt,
                              align);
    if (ret < 0) {
        SET_ERROR_CODE(ret);
        return false;
    }

    d_ptr->imageAlloc = true;
    return true;
}

void Frame::freeImageAlloc()
{
    d_ptr->freeImageAlloc();
}

void Frame::setPictType(AVPictureType type)
{
    d_ptr->frame->pict_type = type;
}

void Frame::unref()
{
    av_frame_unref(d_ptr->frame);
}

void Frame::setPts(qint64 pts)
{
    d_ptr->frame->pts = pts;
}

auto Frame::pts() -> qint64
{
    return d_ptr->frame->pts;
}

void Frame::setDuration(qint64 duration)
{
    d_ptr->frame->pkt_duration = duration;
}

auto Frame::duration() -> qint64
{
    return d_ptr->frame->pkt_duration;
}

void Frame::destroyFrame()
{
    d_ptr->destroyFrame();
}

auto Frame::toImage() -> QImage
{
    Q_ASSERT(d_ptr->frame != nullptr);
    Q_ASSERT(d_ptr->frame->data[0] != nullptr);
    Q_ASSERT(d_ptr->frame->width > 0);
    Q_ASSERT(d_ptr->frame->height > 0);
    Q_ASSERT(d_ptr->frame->format != AV_PIX_FMT_NONE);

    auto format = VideoFormat::qFormatMaps.value(static_cast<AVPixelFormat>(d_ptr->frame->format),
                                                 QImage::Format_Invalid);
    if (format == QImage::Format_Invalid) {
        return {};
    }

    QImage image(d_ptr->frame->data[0],
                 d_ptr->frame->width,
                 d_ptr->frame->height,
                 d_ptr->frame->linesize[0],
                 format);
    return image;
}

auto Frame::getBuffer() -> bool
{
    auto ret = av_frame_get_buffer(d_ptr->frame, 0);
    ERROR_RETURN(ret)
}

auto Frame::isKey() -> bool
{
    return d_ptr->frame->key_frame == 1;
}

auto Frame::avFrame() -> AVFrame *
{
    return d_ptr->frame;
}

auto Frame::fromQImage(const QImage &image) -> Frame *
{
    Q_ASSERT(!image.isNull());
    Q_ASSERT(image.format() != QImage::Format_Invalid);

    auto img = image;
    img.convertTo(QImage::Format_RGBA8888);

    auto framePtr = std::make_unique<Frame>();
    auto avFrame = framePtr->avFrame();
    avFrame->width = img.width();
    avFrame->height = img.height();
    avFrame->format = AV_PIX_FMT_RGBA;
    framePtr->getBuffer();

    memcpy(avFrame->data[0], img.bits(), avFrame->width * avFrame->height * 4);

    return framePtr.release();
}

} // namespace Ffmpeg
