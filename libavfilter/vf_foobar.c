/*
 * Copyright (c) 2012-2014 Clément Bœsch <u pkh me>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Foobar filter
 *
 * @see https://en.wikipedia.org/wiki/Canny_edge_detector
 */

#include "libavutil/avassert.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

#define PLANE_R 0x4
#define PLANE_G 0x1
#define PLANE_B 0x2
#define PLANE_Y 0x1
#define PLANE_U 0x2
#define PLANE_V 0x4
#define PLANE_A 0x8

enum FilterMode {
    MODE_WIRES,
    MODE_COLORMIX,
    MODE_CANNY,
    NB_MODE
};

struct plane_info {
    uint8_t  *tmpbuf;
    uint16_t *gradients;
    char     *directions;
    int      width, height;
};

typedef struct FoobarContext {
    const AVClass *class;
    struct plane_info planes[3];
    int filter_planes;
    int nb_planes;
    double   low, high;
    uint8_t  low_u8, high_u8;
    int mode;
} FoobarContext;

#define OFFSET(x) offsetof(FoobarContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM
static const AVOption foobar_options[] = {
    { "high", "set high threshold", OFFSET(high), AV_OPT_TYPE_DOUBLE, {.dbl=50/255.}, 0, 1, FLAGS },
    { "low",  "set low threshold",  OFFSET(low),  AV_OPT_TYPE_DOUBLE, {.dbl=20/255.}, 0, 1, FLAGS },
    { "mode", "set mode", OFFSET(mode), AV_OPT_TYPE_INT, {.i64=MODE_WIRES}, 0, NB_MODE-1, FLAGS, "mode" },
        { "wires",    "white/gray wires on black",  0, AV_OPT_TYPE_CONST, {.i64=MODE_WIRES},    INT_MIN, INT_MAX, FLAGS, "mode" },
        { "colormix", "mix colors",                 0, AV_OPT_TYPE_CONST, {.i64=MODE_COLORMIX}, INT_MIN, INT_MAX, FLAGS, "mode" },
        { "canny",    "detect edges on planes",     0, AV_OPT_TYPE_CONST, {.i64=MODE_CANNY},    INT_MIN, INT_MAX, FLAGS, "mode" },
    { "planes", "set planes to filter",  OFFSET(filter_planes), AV_OPT_TYPE_FLAGS, {.i64=7}, 1, 0x7, FLAGS, "flags" },
        { "y", "filter luma plane",  0, AV_OPT_TYPE_CONST, {.i64=PLANE_Y}, 0, 0, FLAGS, "flags" },
        { "u", "filter u plane",     0, AV_OPT_TYPE_CONST, {.i64=PLANE_U}, 0, 0, FLAGS, "flags" },
        { "v", "filter v plane",     0, AV_OPT_TYPE_CONST, {.i64=PLANE_V}, 0, 0, FLAGS, "flags" },
        { "r", "filter red plane",   0, AV_OPT_TYPE_CONST, {.i64=PLANE_R}, 0, 0, FLAGS, "flags" },
        { "g", "filter green plane", 0, AV_OPT_TYPE_CONST, {.i64=PLANE_G}, 0, 0, FLAGS, "flags" },
        { "b", "filter blue plane",  0, AV_OPT_TYPE_CONST, {.i64=PLANE_B}, 0, 0, FLAGS, "flags" },
    { NULL }
};

AVFILTER_DEFINE_CLASS(foobar);

static av_cold int init(AVFilterContext *ctx)
{
	av_log(NULL, AV_LOG_WARNING, "init foobar\n");
    FoobarContext *foobar = ctx->priv;

    foobar->low_u8  = foobar->low  * 255. + .5;
    foobar->high_u8 = foobar->high * 255. + .5;
    return 0;
}

static int query_formats(AVFilterContext *ctx)
{
	av_log(NULL, AV_LOG_WARNING, "query_format foobar\n");
    const FoobarContext *foobar = ctx->priv;
    static const enum AVPixelFormat wires_pix_fmts[] = {AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE};
    static const enum AVPixelFormat canny_pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV444P, AV_PIX_FMT_GBRP, AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE};
    static const enum AVPixelFormat colormix_pix_fmts[] = {AV_PIX_FMT_GBRP, AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE};
    AVFilterFormats *fmts_list;
	static const enum AVPixelFormat dst_pix_fmts[] = {AV_PIX_FMT_GBRP, AV_PIX_FMT_NONE};
    const enum AVPixelFormat *pix_fmts = NULL;

	av_log(NULL, AV_LOG_WARNING, "foobar->mode: %d, foobar->nb_planes: %d\n", foobar->mode, foobar->nb_planes);

	pix_fmts = dst_pix_fmts;

    fmts_list = ff_make_format_list(pix_fmts);
    if (!fmts_list)
        return AVERROR(ENOMEM);
    return ff_set_common_formats(ctx, fmts_list);
}

static int config_props(AVFilterLink *inlink)
{
	av_log(NULL, AV_LOG_WARNING, "config_props foobar\n");
    int p;
    AVFilterContext *ctx = inlink->dst;
    FoobarContext *foobar = ctx->priv;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);

    foobar->nb_planes = inlink->format == AV_PIX_FMT_GRAY8 ? 1 : 3;
    for (p = 0; p < foobar->nb_planes; p++) {
        struct plane_info *plane = &foobar->planes[p];
        int vsub = p ? desc->log2_chroma_h : 0;
        int hsub = p ? desc->log2_chroma_w : 0;

        plane->width      = AV_CEIL_RSHIFT(inlink->w, hsub);
        plane->height     = AV_CEIL_RSHIFT(inlink->h, vsub);
        plane->tmpbuf     = av_malloc(plane->width * plane->height);
        plane->gradients  = av_calloc(plane->width * plane->height, sizeof(*plane->gradients));
        plane->directions = av_malloc(plane->width * plane->height);
        if (!plane->tmpbuf || !plane->gradients || !plane->directions)
            return AVERROR(ENOMEM);
    }
    return 0;
}

static void process_image(int w, int h,
                            uint8_t *dst, int dst_linesize,
                      const uint8_t *src, int src_linesize)
{
    int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++)
            dst[i] = src[i];
        dst += dst_linesize;
        src += src_linesize;
    }
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
	av_log(NULL, AV_LOG_WARNING, "filter_frame foobar\n");
    AVFilterContext *ctx = inlink->dst;
    FoobarContext *foobar = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    int p, direct = 0;
    AVFrame *out;

    if (foobar->mode != MODE_COLORMIX && av_frame_is_writable(in)) {
        direct = 1;
        out = in;
    } else {
        out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
        if (!out) {
            av_frame_free(&in);
            return AVERROR(ENOMEM);
        }
        av_frame_copy_props(out, in);
    }

	av_log(NULL, AV_LOG_WARNING, "direct = %d, foobar->nb_planes: %d\n", direct, foobar->nb_planes);
    for (p = 0; p < foobar->nb_planes; p++) {
        struct plane_info *plane = &foobar->planes[p];
        uint8_t  *tmpbuf     = plane->tmpbuf;
        uint16_t *gradients  = plane->gradients;
        int8_t   *directions = plane->directions;
        const int width      = plane->width;
        const int height     = plane->height;

        if (!((1 << p) & foobar->filter_planes)) {
            if (!direct)
                av_image_copy_plane(out->data[p], out->linesize[p],
                                    in->data[p], in->linesize[p],
                                    width, height);
            continue;
        }

		//Image Processing
		process_image(width, height,
				  out->data[p], out->linesize[p],
				  in->data[p], in->linesize[p]);
    }

    if (!direct)
        av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}

static av_cold void uninit(AVFilterContext *ctx)
{
	printf("\e[35m begin \e[0m\n");
	av_log(NULL, AV_LOG_WARNING, "uninit foobar04\n");
    int p;
    FoobarContext *foobar = ctx->priv;

    for (p = 0; p < foobar->nb_planes; p++) {
        struct plane_info *plane = &foobar->planes[p];
        av_freep(&plane->tmpbuf);
        av_freep(&plane->gradients);
        av_freep(&plane->directions);
    }
}

static const AVFilterPad foobar_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_props,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad foobar_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_foobar = {
    .name          = "foobar",
    .description   = NULL_IF_CONFIG_SMALL("Foobar filter as a filter template."),
    .priv_size     = sizeof(FoobarContext),
    .init          = init,
    .uninit        = uninit,
    .query_formats = query_formats,
    .inputs        = foobar_inputs,
    .outputs       = foobar_outputs,
    .priv_class    = &foobar_class,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
};
