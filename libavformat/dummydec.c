/**
 *Author: feixiaku
 *mail: feixiaku@gmail.com
 * */

#include "dummy.h"

typedef struct DummyDemuxerContext{
    const AVClass *pClass;
    int width, height;
    char *pixel_format;
    AVRational framerate;
} DummyDemuxerContext;

static int dummy_probe(AVProbeData *p)
{
    //compare key words
    av_log(NULL, AV_LOG_WARNING, "Dummy demuxer dummy_probe!\n");
    return AVPROBE_SCORE_MAX;
}

static int dummy_read_header(AVFormatContext *s)
{
    av_log(NULL, AV_LOG_WARNING, "Dummy demuxer read_header!\n");
    DummyDemuxerContext *dummyCtx = s->priv_data;
    enum AVPixelFormat pix_fmt;
    AVStream *st;

    st = avformat_new_stream(s, NULL);
    if(!st)
    {
        av_log(NULL, AV_LOG_ERROR, "dummy demuxer read header error\n");
        return AVERROR(ENOMEM);
    }

    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->codec_id = s->iformat->raw_codec_id;

    //hardcode pixel format
    pix_fmt = AV_PIX_FMT_YUV420P;

    st->time_base.num = dummyCtx->framerate.num;
    st->time_base.den = dummyCtx->framerate.den;
    st->pts_wrap_bits = 64;

    st->codec->width    = dummyCtx->width;
    st->codec->height   = dummyCtx->height;
    st->codec->pix_fmt  = pix_fmt;

    AVRational tmpRation;
    tmpRation.den = 1;
    tmpRation.num = 8;

    st->codec->bit_rate = av_rescale_q(
            avpicture_get_size(st->codec->pix_fmt, dummyCtx->width, dummyCtx->height),
            tmpRation,
            st->time_base);
    av_log(NULL, AV_LOG_WARNING, "Dummy demuxer bit rate: %d, width: %d, height: %d, codec ID: %d\n",
            st->codec->bit_rate, dummyCtx->width, dummyCtx->height, st->codec->codec_id);
    return 0;
}

static int dummy_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    av_log(NULL, AV_LOG_WARNING, "Dummy demuxer read_packet!\n");
    int packet_size, ret, width, height;
    AVStream *st = s->streams[0];

    st->codec->codec_id = s->iformat->raw_codec_id;
    width   = st->codec->width;
    height  = st->codec->height;

    av_log(NULL, AV_LOG_WARNING, "read packet width: %d, height: %d, get packet size %d, stream number: %d, Codec ID: %d\n",
            width, height, packet_size, s->nb_streams, st->codec->codec_id);
    packet_size = avpicture_get_size(st->codec->pix_fmt, width, height);
    if(packet_size < 0)
    {
        av_log(NULL, AV_LOG_WARNING, "Dummy demuxer get packet size %d\n", packet_size);
        return -1;
    }

    ret = av_get_packet(s->pb, pkt, packet_size);
    pkt->pts = pkt->dts = pkt->pos / packet_size;

    pkt->stream_index = 0;
    if(ret < 0)
        return ret;

    return 0;
}

static int dummy_read_close(AVFormatContext *s)
{
    av_log(NULL, AV_LOG_WARNING, "Dummy demuxer read_close!\n");
    //free some resource
    return 0;
}

#define OFFSET(x) offsetof(DummyDemuxerContext, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
static const AVOption dummy_options[] =
{
    { "video_size", "set frame size", OFFSET(width), AV_OPT_TYPE_IMAGE_SIZE, {.str = NULL}, 0, 0, DEC },
    { "pixel_format", "set pixel format", OFFSET(pixel_format), AV_OPT_TYPE_STRING, {.str = "yuv420p"}, 0, 0, DEC },
    { "framerate", "set frame rate", OFFSET(framerate), AV_OPT_TYPE_VIDEO_RATE, {.str = "25"}, 0, 0, DEC },
    { NULL },
};

static const AVClass dummy_demuxer_class = {
    .class_name = "dummy video demuxer",
    .item_name  = av_default_item_name,
    .option     = dummy_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_dummy_demuxer = {
    .name           = "dummy",
    .long_name      = NULL_IF_CONFIG_SMALL("dummy demuxer"),
    .flags          = AVFMT_GENERIC_INDEX,
    .extensions     = "dummy",

    .priv_class     = &dummy_demuxer_class,
    .priv_data_size = sizeof(DummyDemuxerContext),
    .raw_codec_id   = AV_CODEC_ID_RAWVIDEO,

    //.read_probe     = dummy_probe,
    .read_header    = dummy_read_header,
    .read_packet    = dummy_read_packet,
    //.read_close     = dummy_read_close,
};
