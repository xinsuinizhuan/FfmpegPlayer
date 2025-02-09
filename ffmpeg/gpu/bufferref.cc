#include "bufferref.hpp"

#include <ffmpeg/averrormanager.hpp>

#include <QDebug>

extern "C" {
#include <libavutil/buffer.h>
}

namespace Ffmpeg {

class BufferRef::BufferRefPrivate
{
public:
    BufferRefPrivate(BufferRef *q)
        : q_ptr(q)
    {}

    ~BufferRefPrivate() { av_buffer_unref(&bufferRef); }

    BufferRef *q_ptr;

    AVBufferRef *bufferRef = nullptr;
};

BufferRef::BufferRef(QObject *parent)
    : QObject{parent}
    , d_ptr(new BufferRefPrivate(this))
{}

BufferRef::~BufferRef() {}

bool BufferRef::hwdeviceCtxCreate(AVHWDeviceType hwDeviceType)
{
    auto ret = av_hwdevice_ctx_create(&d_ptr->bufferRef, hwDeviceType, nullptr, nullptr, 0);
    if (ret < 0) {
        qWarning() << "Failed to create Gpu device context: " << hwDeviceType;
        SET_ERROR_CODE(ret);
        return false;
    }
    return true;
}

BufferRef *BufferRef::hwframeCtxAlloc()
{
    auto hw_frames_ref = av_hwframe_ctx_alloc(d_ptr->bufferRef);
    if (!hw_frames_ref) {
        qWarning() << "Failed to create Gpu frame context.";
        return nullptr;
    }
    auto bufferRef = new BufferRef;
    bufferRef->d_ptr->bufferRef = hw_frames_ref;
    return bufferRef;
}

bool BufferRef::hwframeCtxInit()
{
    auto ret = av_hwframe_ctx_init(d_ptr->bufferRef);
    ERROR_RETURN(ret)
}

AVBufferRef *BufferRef::ref()
{
    return av_buffer_ref(d_ptr->bufferRef);
}

AVBufferRef *BufferRef::avBufferRef()
{
    return d_ptr->bufferRef;
}

} // namespace Ffmpeg
