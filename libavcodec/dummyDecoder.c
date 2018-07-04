/*
 *author: feixiaku
 *
 */

#include "avcodec.h"

static av_cold int dummy_init_decoder(AVCodecContext *avctx)
{
    av_log(NULL, AV_LOG_WARNING, "init dummy decoder!\n");
    return 0;
}

static int dummy_decode(AVCodecContext *avctx, void *data,
                        int *got_frame, AVPacket* avpkt)
{
    av_log(NULL, AV_LOG_WARNING, "processing dummy decoder!\n");

    AVFrame *frame = (AVFrame*)data;
    AVPicture *picture = (AVPicture*)data;

    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;

    int size = avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height);

    frame->pict_type = AV_PICTURE_TYPE_I;
    frame->key_frame = 1;
    frame->buf[0] = av_buffer_alloc(size);

    memcpy(frame->buf[0]->data, buf, buf_size);

    int res = 0;
    if(res = avpicture_fill(picture, frame->buf[0]->data, avctx->pix_fmt,
                    avctx->width, avctx->height) < 0)
    {
        av_buffer_unref(&frame->buf[0]);
        return res;
    }

    *got_frame = 1;

    return 0;
}

static av_cold int dummy_close_decoder(AVCodecContext *avctx)
{
    av_log(NULL, AV_LOG_WARNING, "close dummy decoder!\n");
    return 0;
}

AVCodec ff_dummy_decoder = {
    .name   = "dummy",
    .long_name  = "dummy custom decoder",
    .type   = AVMEDIA_TYPE_VIDEO,
    .id     = AV_CODEC_ID_DUMMY,
    .capabilities   = AV_CODEC_CAP_PARAM_CHANGE,
    .init   = dummy_init_decoder,
    .close  = dummy_close_decoder,
    .decode    = dummy_decode,
};
