#include "formatcontext.h"
#include "averrormanager.hpp"
#include "packet.h"

#include <QDebug>
#include <QImage>
#include <QTime>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class FormatContext::FormatContextPrivate
{
public:
    FormatContextPrivate(FormatContext *q)
        : q_ptr(q)
    {
        avformat_network_init();
    }

    ~FormatContextPrivate() {}

    void findStreamIndex()
    {
        videoIndexs.clear();
        audioMap.clear();
        subtitleMap.clear();
        coverImage = QImage();

        //nb_streams视音频流的个数
        for (uint i = 0; i < formatCtx->nb_streams; i++) {
            switch (formatCtx->streams[i]->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO: videoIndexs.append(i); break;
            case AVMEDIA_TYPE_AUDIO: {
                AVDictionaryEntry *tag = nullptr;
                QString str = QString::number(audioMap.size() + 1) + ".";
                while (nullptr
                       != (tag = av_dict_get(formatCtx->streams[i]->metadata,
                                             "",
                                             tag,
                                             AV_DICT_IGNORE_SUFFIX))) {
                    QString key = tag->key;
                    if (key == "language" || key == "title" || key == "NUMBER_OF_BYTES") {
                        str = str + QString::fromUtf8(tag->value) + "-";
                    }
                    //qDebug() << key << ":" << QString::fromUtf8(tag->value);
                }
                str.chop(1);
                audioMap.insert(i, str);
            } break;
            case AVMEDIA_TYPE_SUBTITLE: {
                QString str = QString::number(subtitleMap.size() + 1) + ".";
                AVDictionaryEntry *tag = nullptr;
                while (nullptr
                       != (tag = av_dict_get(formatCtx->streams[i]->metadata,
                                             "",
                                             tag,
                                             AV_DICT_IGNORE_SUFFIX))) {
                    QString key = tag->key;
                    if (key == "language" || key == "title" || key == "NUMBER_OF_BYTES") {
                        str = str + QString::fromUtf8(tag->value) + "-";
                    }
                    //qDebug() << key << ":" << QString::fromUtf8(tag->value);
                }
                str.chop(1);
                subtitleMap.insert(i, str);
            } break;
            default: break;
            }

            if (formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket &pkt = formatCtx->streams[i]->attached_pic;
                coverImage = QImage::fromData((uchar *) pkt.data, pkt.size);
            }
        }
    }

    FormatContext *q_ptr;

    AVFormatContext *formatCtx = nullptr;
    QString filepath;
    FormatContext::OpenMode mode = FormatContext::ReadOnly;
    bool isOpen = false;

    QVector<int> videoIndexs;
    QMap<int, QString> audioMap;
    QMap<int, QString> subtitleMap;
    QImage coverImage;

    const qint64 seekOffset = 2 * AV_TIME_BASE;
};

FormatContext::FormatContext(QObject *parent)
    : QObject(parent)
    , d_ptr(new FormatContextPrivate(this))
{}

FormatContext::~FormatContext()
{
    close();
}

void FormatContext::copyChapterFrom(FormatContext *src)
{
    auto is = src->avFormatContext();
    auto os = d_ptr->formatCtx;
    AVChapter **tmp = (AVChapter **) av_realloc_f(os->chapters,
                                                  is->nb_chapters + os->nb_chapters,
                                                  sizeof(*os->chapters));
    if (!tmp) {
        qWarning() << tr("Memory request error");
        return;
    }
    os->chapters = tmp;

    for (uint i = 0; i < is->nb_chapters; i++) {
        AVChapter *in_ch = is->chapters[i], *out_ch;
        int64_t start_time = 0;
        int64_t ts_off = av_rescale_q(start_time, AVRational{1, AV_TIME_BASE}, in_ch->time_base);
        int64_t rt = INT64_MAX;

        if (in_ch->end < ts_off) {
            continue;
        }
        if (rt != INT64_MAX && in_ch->start > rt + ts_off) {
            break;
        }
        out_ch = (AVChapter *) av_mallocz(sizeof(AVChapter));
        if (!out_ch) {
            qWarning() << tr("Memory request error");
            return;
        }
        out_ch->id = in_ch->id;
        out_ch->time_base = in_ch->time_base;
        out_ch->start = FFMAX(0, in_ch->start - ts_off);
        out_ch->end = FFMIN(rt, in_ch->end - ts_off);

        av_dict_copy(&out_ch->metadata, in_ch->metadata, 0);

        os->chapters[os->nb_chapters++] = out_ch;
    }
}

bool FormatContext::isOpen()
{
    return d_ptr->isOpen;
}

bool FormatContext::openFilePath(const QString &filepath, OpenMode mode)
{
    d_ptr->mode = mode;
    close();
    d_ptr->filepath = filepath;

    QByteArray inpuUrl;
    if (QFile::exists(d_ptr->filepath)) {
        inpuUrl = d_ptr->filepath.toUtf8();
    } else {
        inpuUrl = QUrl(d_ptr->filepath).toEncoded();
    }
    //初始化pFormatCtx结构
    if (mode == ReadOnly) {
        auto ret = avformat_open_input(&d_ptr->formatCtx, inpuUrl.constData(), nullptr, nullptr);
        if (ret != 0) {
            SET_ERROR_CODE(ret);
            return false;
        }
        av_format_inject_global_side_data(d_ptr->formatCtx);
        d_ptr->isOpen = true;
    } else if (mode == WriteOnly) {
        avformat_free_context(d_ptr->formatCtx);
        auto ret = avformat_alloc_output_context2(&d_ptr->formatCtx,
                                                  nullptr,
                                                  nullptr,
                                                  inpuUrl.constData());
        if (ret < 0) {
            SET_ERROR_CODE(ret);
            return false;
        }
        d_ptr->isOpen = true;
    }
    return d_ptr->isOpen;
}

void FormatContext::close()
{
    if (!d_ptr->isOpen) {
        return;
    }
    switch (d_ptr->mode) {
    case ReadOnly:
        avformat_close_input(&d_ptr->formatCtx);
        d_ptr->formatCtx = nullptr;
        d_ptr->isOpen = false;
        break;
    case WriteOnly: avio_close(); break;
    default: break;
    }
}

bool FormatContext::avio_open()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    if (d_ptr->formatCtx->oformat->flags & AVFMT_NOFILE) {
        return false;
    }
    auto ret = ::avio_open(&d_ptr->formatCtx->pb,
                           d_ptr->filepath.toLocal8Bit().constData(),
                           AVIO_FLAG_WRITE);
    ERROR_RETURN(ret)
}

void FormatContext::avio_close()
{
    if (!d_ptr->isOpen && d_ptr->mode != WriteOnly) {
        return;
    }
    if (d_ptr->formatCtx && !(d_ptr->formatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&d_ptr->formatCtx->pb);
    }
    avformat_free_context(d_ptr->formatCtx);
    d_ptr->formatCtx = nullptr;
    d_ptr->isOpen = false;
}

bool FormatContext::writeHeader()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto ret = avformat_write_header(d_ptr->formatCtx, nullptr);
    ERROR_RETURN(ret)
}

bool FormatContext::writePacket(Packet *packet)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto ret = av_interleaved_write_frame(d_ptr->formatCtx, packet->avPacket());
    ERROR_RETURN(ret)
}

bool FormatContext::writeTrailer()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto ret = av_write_trailer(d_ptr->formatCtx);
    ERROR_RETURN(ret)
}

bool FormatContext::findStream()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    //获取音视频流数据信息
    int ret = avformat_find_stream_info(d_ptr->formatCtx, nullptr);
    if (ret < 0) {
        SET_ERROR_CODE(ret);
        return false;
    }
    if (d_ptr->formatCtx->pb) {
        // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
        d_ptr->formatCtx->pb->eof_reached = 0;
    }
    d_ptr->findStreamIndex();
    return true;
}

int FormatContext::streams() const
{
    return d_ptr->formatCtx->nb_streams;
}

QMap<int, QString> FormatContext::audioMap() const
{
    return d_ptr->audioMap;
}

QVector<int> FormatContext::videoIndexs() const
{
    return d_ptr->videoIndexs;
}

QMap<int, QString> FormatContext::subtitleMap() const
{
    return d_ptr->subtitleMap;
}

int FormatContext::findBestStreamIndex(AVMediaType type) const
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return av_find_best_stream(d_ptr->formatCtx, type, -1, -1, nullptr, 0);
}

void FormatContext::discardStreamExcluded(QVector<int> indexs)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    for (uint i = 0; i < d_ptr->formatCtx->nb_streams; i++) {
        if (indexs.contains(i)) {
            continue;
        }
        d_ptr->formatCtx->streams[i]->discard = AVDISCARD_ALL;
    }
}

AVStream *FormatContext::stream(int index)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx->streams[index];
}

AVStream *FormatContext::createStream()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto stream = avformat_new_stream(d_ptr->formatCtx, nullptr);
    if (!stream) {
        qWarning() << "Failed allocating output stream\n";
    }
    return stream;
}

bool FormatContext::readFrame(Packet *packet)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_read_frame(d_ptr->formatCtx, packet->avPacket());
    ERROR_RETURN(ret)
}

bool FormatContext::checkPktPlayRange(Packet *packet)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto avPacket = packet->avPacket();
    /* check if packet is in play range specified by user, then queue, otherwise discard */
    auto start_time = AV_NOPTS_VALUE;
    auto duration = AV_NOPTS_VALUE;
    auto stream_start_time = d_ptr->formatCtx->streams[avPacket->stream_index]->start_time;
    auto pkt_ts = avPacket->pts == AV_NOPTS_VALUE ? avPacket->dts : avPacket->pts;
    auto pkt_in_play_range
        = duration == AV_NOPTS_VALUE
          || (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0))
                         * av_q2d(d_ptr->formatCtx->streams[avPacket->stream_index]->time_base)
                     - (double) (start_time != AV_NOPTS_VALUE ? start_time : 0) / AV_TIME_BASE
                 <= ((double) duration / AV_TIME_BASE);
    return pkt_in_play_range;
}

AVRational FormatContext::guessFrameRate(int index) const
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return guessFrameRate(d_ptr->formatCtx->streams[index]);
}

AVRational FormatContext::guessFrameRate(AVStream *stream) const
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return av_guess_frame_rate(d_ptr->formatCtx, stream, nullptr);
}

bool FormatContext::seekFirstFrame()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int64_t timestamp = 0;
    if (d_ptr->formatCtx->start_time != AV_NOPTS_VALUE) {
        timestamp = d_ptr->formatCtx->start_time;
    }
    int ret = avformat_seek_file(d_ptr->formatCtx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
    ERROR_RETURN(ret)
}

bool FormatContext::seek(qint64 timestamp)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    Q_ASSERT(timestamp >= 0);
    auto seekMin = timestamp - d_ptr->seekOffset;
    if (seekMin < 0) {
        seekMin = 0;
    }
    auto seekMax = timestamp + d_ptr->seekOffset;
    if (seekMax > d_ptr->formatCtx->duration) {
        seekMax = d_ptr->formatCtx->duration;
    }
    // qDebug() << "seekMin:" << seekMin << "seekMax:" << seekMax << "timestamp:" << timestamp;
    auto ret = avformat_seek_file(d_ptr->formatCtx, -1, seekMin, timestamp, seekMax, 0);
    ERROR_RETURN(ret)
}

bool FormatContext::seek(int index, qint64 timestamp)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_seek_frame(d_ptr->formatCtx, index, timestamp, AVSEEK_FLAG_BACKWARD);
    ERROR_RETURN(ret)
}

void FormatContext::printFileInfo()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    qInfo() << "AV Format Name:" << d_ptr->formatCtx->iformat->name;
    QTime time(QTime::fromMSecsSinceStartOfDay(d_ptr->formatCtx->duration / 1000));
    qInfo() << "Duration:" << time.toString("hh:mm:ss.zzz");

    // Metadata
    AVDictionaryEntry *tag = nullptr;
    while (nullptr
           != (tag = av_dict_get(d_ptr->formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        qInfo() << tag->key << QString::fromUtf8(tag->value);
    }
}

void FormatContext::dumpFormat()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    av_dump_format(d_ptr->formatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), d_ptr->mode - 1);
}

AVFormatContext *FormatContext::avFormatContext()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx;
}

qint64 FormatContext::duration() const
{
    return d_ptr->isOpen ? d_ptr->formatCtx->duration : 0;
}

QImage &FormatContext::coverImage() const
{
    return d_ptr->coverImage;
}

} // namespace Ffmpeg
