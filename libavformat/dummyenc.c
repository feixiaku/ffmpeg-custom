/**
 *Author: feixiaku
 *mail: feixiaku@gmail.com
 * */

#include "dummy.h"

static int dummy_write_header(AVFormatContext *fmt)
{
    av_log(NULL, AV_LOG_WARNING, "dummy write header!");
    //write your self format header
    return 0;
}

static int dummy_write_packet(AVFormatContext *fmt, AVPacket *pkt)
{
    //use file protocol write file
    av_log(NULL, AV_LOG_WARNING, "dummy write packet %s, codec ID: %d",
            (fmt->streams[0]->codec->codec->name), fmt->video_codec_id);
    avio_write(fmt->pb, pkt->data, pkt->size);
    return 0;
}

static int dummy_write_trailer(AVFormatContext *fmt)
{
    av_log(NULL, AV_LOG_WARNING, "dummy write trailer!");
    //write your self format trailer
    return 0;
}

AVOutputFormat ff_dummy_muxer =
{
    .name           = "dummy",
    .long_name       = NULL_IF_CONFIG_SMALL("dummy custom demux"),
    .mime_type      = "dummy/x-dummyvideo",
    .extensions     = "dummy",

    .audio_codec    = AV_CODEC_ID_NONE,
    .video_codec    = AV_CODEC_ID_RAWVIDEO, //TODO: AV_CODEC_ID_RAWVIDEO --> AV_CODEC_ID_DUMMY
    .subtitle_codec = AV_CODEC_ID_NONE,

    .flags          = AVFMT_NOTIMESTAMPS,
    .priv_data_size = 0,

    .write_header   = dummy_write_header,
    .write_packet   = dummy_write_packet,
    .write_trailer  = dummy_write_trailer,
};
