
typedef struct TransformContex {

} TransformContex;


static const AVClass feixiaku_transform_class = {

}



static const AVFilterPad avfilter_feixiaku_transform_inputs = {
	{
		.name 			= "feixiaku_transform_inputpad"
		.type 			= AVMEDIA_TYPE_VIDEO, 
		.filter_frame	= filter_frame,
	},
	{ NULL }
};

static const AVFliterPad avfilter_feixiaku_transform_outputs = {
	{
		.name 			= "feixiaku_transform_outputpad"
		.type 			= AVMEDIA_TYPE_VIDEO,
		.config_props 	= config_output,
	}
	{ NULL}
};

AVFilter ff_feixiaku_transform = {
    .name          = "feixiaku_transform",
    .description   = NULL_IF_CONFIG_SMALL("feixiaku's custom transform"),
    .priv_size     = sizeof(TransformContex), 
    .init 		   = init,
    .unint 		   = uninit,
    .query_formats = query_formats,
    .inputs 	   = avfilter_feixiaku_transform_inputs,
    .outputs 	   = avfilter_feixiaku_transform_outputs,
    .priv_class    = &feixiaku_transform_class,
};
