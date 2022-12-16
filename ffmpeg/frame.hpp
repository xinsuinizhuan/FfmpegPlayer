#pragma once

#include "ffmepg_global.h"

#include <QImage>

extern "C" {
#include <libavutil/pixfmt.h>
}

struct AVFrame;

namespace Ffmpeg {

class FFMPEG_EXPORT Frame
{
public:
    explicit Frame();
    Frame(const QImage &image);
    Frame(const Frame &other);
    Frame &operator=(const Frame &other);
    ~Frame();

    bool isKey();

    bool imageAlloc(const QSize &size, AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA, int align = 1);
    void freeImageAlloc();

    void setPts(double pts);
    double pts();

    void setDuration(double duration);
    double duration();

    void setQImageFormat(QImage::Format format);
    QImage::Format format() const;
    QImage convertToImage() const; // maybe null

    void clear();

    AVFrame *avFrame();

private:
    struct FramePrivate;
    QScopedPointer<FramePrivate> d_ptr;
};

} // namespace Ffmpeg
