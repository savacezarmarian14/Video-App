#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>


struct Size {
    int width;
    int height;
};

int main(int argc, char **argv)
{
    struct Size scaleSize = { 320, 240 };
    char *flip = "none";
    gboolean gRet = FALSE;
    GOptionEntry entries[] = {
        { "width",  'w', 0, G_OPTION_ARG_INT,    &scaleSize.width,  "Scale Width",  "N" },
        { "height", 'h', 0, G_OPTION_ARG_INT,    &scaleSize.height, "Scale Height", "N" },
        { "flip",   'f', 0, G_OPTION_ARG_STRING, &flip,             "Flip",         "STR" },
        { NULL }
    };
    GError *err = NULL;
    GOptionContext *ctx = g_option_context_new("Real-time Video processing pipeline");
   
    gst_init(&argc, &argv);
    printf("[DEBUG] Gstreamer initialised\n");


    g_option_context_add_main_entries(ctx, entries, NULL);
    g_option_context_add_group(ctx, gst_init_get_option_group());

    if (!g_option_context_parse(ctx, &argc, &argv, &err)) {
        g_printerr("%s\n", err->message);
        g_clear_error(&err);
        g_option_context_free(ctx);
        return -1;
    }
    g_option_context_free(ctx);
    

    GstElement *pipeline = gst_pipeline_new("pipeline");
    if (NULL == pipeline) {
        printf("[ERROR] Couldn't create pipeline\n");
        return -1;
    }

    GstElement *src = gst_element_factory_make("v4l2src", "video source");
    if (NULL == src) {
        printf("[ERROR] Couldn't create source pipeline element\n");
        return -1;
    }

    g_object_set(src, "device", "/dev/video0", NULL);
    printf("[DEBUG] Set video device on source element\n");

    GstElement *convert = gst_element_factory_make("videoconvert", "video converter");
    if (NULL == convert) {
        printf("[ERROR] Couldn't create video converter element\n");
        return -1;
    }

    GstElement *capsfilter = gst_element_factory_make("capsfilter", "video source filter");
    if (NULL == capsfilter) {
        printf("[ERROR] Couldn't create caps filter\n");
        return -1;
    }

    GstCaps *capsfilterValues = gst_caps_new_simple("video/x-raw",
        "format",    G_TYPE_STRING,     "YUY2",
        "width",     G_TYPE_INT,        640,
        "height",    G_TYPE_INT,        480,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        NULL);

    if (NULL == capsfilterValues) {
        printf("[ERROR] Couldn't create caps filter values\n");
        return -1;
    }

    g_object_set(capsfilter, "caps", capsfilterValues, NULL);
    gst_caps_unref(capsfilterValues);


    GstElement *scaler = gst_element_factory_make("videoscale", "video scaler");
    if (NULL == scaler) {
        printf("[ERROR] Couldn't create video scaler\n");
        return -1;
    }

    GstElement *capsScaler = gst_element_factory_make("capsfilter", "scaler caps");
    if (NULL == capsScaler) {
        printf("[ERROR] Couldn't create caps scaler\n");
        return -1;
    }
    GstCaps *capsScalerValues = gst_caps_new_simple("video/x-raw",
        "width",     G_TYPE_INT,        scaleSize.width,
        "height",    G_TYPE_INT,        scaleSize.height,
        NULL);
    g_object_set(capsScaler, "caps", capsScalerValues, NULL);
    gst_caps_unref(capsScalerValues);

    GstElement *invertor = gst_element_factory_make("frei0r-filter-invert0r", "video invertor");
    if (NULL == invertor) {
        printf("[ERROR] Couldn't create video invertor. Possible plugin is not instaleld\n");
        return -1;
    }

    GstElement *invertorConverter = gst_element_factory_make("videoconvert", "Invertor converter to RGBA");
    if (NULL == invertorConverter) {
        printf("[ERROR] Couldn't create invertor converter\n");
        return -1;
    }

    GstElement *videoflip = gst_element_factory_make("videoflip", "Video Flipper");
    if (NULL == videoflip) {
        printf("[ERROR] Couldn't create video flipper\n");
        return -1;
    }
    gst_util_set_object_arg(G_OBJECT(videoflip), "method", flip);

    GstElement *videoflipConverter = gst_element_factory_make("videoconvert", "Video flip converter\n");
    if (NULL == videoflipConverter) {
        printf("[ERROR] Couldn't create video flip converter\n");
        return -1;
    }

    GstElement *screen = gst_element_factory_make("autovideosink", "video destination");
    if (NULL == screen) {
        printf("[ERROR] Couldn't create output element\n");
        return -1;
    }

    g_object_set(screen, "sync", FALSE, NULL);

    gst_bin_add_many(GST_BIN(pipeline), src, capsfilter, convert, scaler, capsScaler, invertorConverter, invertor, videoflip, videoflipConverter, screen, NULL);

    gRet = gst_element_link_many(src, capsfilter, convert, scaler, capsScaler, invertorConverter, invertor, videoflip, videoflipConverter, screen, NULL);
    if (FALSE == gRet) {
        printf("[ERROR] Couldn't link elements in pipeline\n");
        gst_object_unref(pipeline);
        return -1;
    }

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING)
            == GST_STATE_CHANGE_FAILURE) {
        g_printerr("[ERROR] Couldn't start the pipeline\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_print("[INFO] Running. Press Ctrl+C to stop.\n");

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(
        bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *e = NULL;
            gchar *dbg = NULL;
            gst_message_parse_error(msg, &e, &dbg);
            g_printerr("[ERROR] From %s: %s\n",
                       GST_OBJECT_NAME(msg->src), e->message);
            g_printerr("[DEBUG] %s\n", dbg ? dbg : "(no details)");
            g_clear_error(&e);
            g_free(dbg);
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}