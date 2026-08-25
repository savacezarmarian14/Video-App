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
    gchar *flipOpt = NULL;
    gchar *deviceOpt = NULL;
    gboolean gRet = FALSE;

    GOptionEntry entries[] = {
        { "width",  'w', 0, G_OPTION_ARG_INT,    &scaleSize.width,  "Scale width",  "N" },
        { "height",   0, 0, G_OPTION_ARG_INT,    &scaleSize.height, "Scale height", "N" },
        { "flip",   'f', 0, G_OPTION_ARG_STRING, &flipOpt,          "Flip method",  "STR" },
        { "device", 'd', 0, G_OPTION_ARG_STRING, &deviceOpt,        "Video device", "PATH" },
        { NULL }
    };

    GError *err = NULL;
    GOptionContext *ctx = g_option_context_new("- real-time video processing pipeline");

    g_option_context_add_main_entries(ctx, entries, NULL);
    g_option_context_add_group(ctx, gst_init_get_option_group());

    if (!g_option_context_parse(ctx, &argc, &argv, &err)) {
        printf("[ERROR] %s\n", err->message);
        g_clear_error(&err);
        g_option_context_free(ctx);
        return -1;
    }
    g_option_context_free(ctx);

    const char *flip = flipOpt ? flipOpt : "none";
    const char *device = deviceOpt ? deviceOpt : "/dev/video0";

    if (scaleSize.width <= 0 || scaleSize.height <= 0) {
        printf("[ERROR] Invalid scale size %dx%d\n", scaleSize.width, scaleSize.height);
        g_free(flipOpt);
        g_free(deviceOpt);
        return -1;
    }

    printf("[DEBUG] GStreamer initialised\n");

    GstElement *pipeline = gst_pipeline_new("pipeline");
    if (NULL == pipeline) {
        printf("[ERROR] Couldn't create pipeline\n");
        return -1;
    }

    GstElement *src = gst_element_factory_make("v4l2src", "source");
    if (NULL == src) {
        printf("[ERROR] Couldn't create source element\n");
        return -1;
    }
    g_object_set(src, "device", device, NULL);
    printf("[DEBUG] Using video device %s\n", device);

    GstElement *capsfilter = gst_element_factory_make("capsfilter", "caps-source");
    if (NULL == capsfilter) {
        printf("[ERROR] Couldn't create source caps filter\n");
        return -1;
    }

    GstCaps *capsfilterValues = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "YUY2", "width", G_TYPE_INT, 640,
        "height", G_TYPE_INT, 480, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
    if (NULL == capsfilterValues) {
        printf("[ERROR] Couldn't create source caps\n");
        return -1;
    }
    g_object_set(capsfilter, "caps", capsfilterValues, NULL);
    gst_caps_unref(capsfilterValues);

    GstElement *convert = gst_element_factory_make("videoconvert", "conv-scale");
    if (NULL == convert) {
        printf("[ERROR] Couldn't create video converter\n");
        return -1;
    }

    GstElement *scaler = gst_element_factory_make("videoscale", "scaler");
    if (NULL == scaler) {
        printf("[ERROR] Couldn't create video scaler\n");
        return -1;
    }

    GstElement *capsScaler = gst_element_factory_make("capsfilter", "caps-scale");
    if (NULL == capsScaler) {
        printf("[ERROR] Couldn't create scaler caps filter\n");
        return -1;
    }

    GstCaps *capsScalerValues = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, scaleSize.width,
        "height", G_TYPE_INT, scaleSize.height, NULL);
    if (NULL == capsScalerValues) {
        printf("[ERROR] Couldn't create scaler caps\n");
        return -1;
    }
    g_object_set(capsScaler, "caps", capsScalerValues, NULL);
    gst_caps_unref(capsScalerValues);

    GstElement *invertorConverter = gst_element_factory_make("videoconvert", "conv-invert");
    if (NULL == invertorConverter) {
        printf("[ERROR] Couldn't create invert converter\n");
        return -1;
    }

    GstElement *invertor = gst_element_factory_make("frei0r-filter-invert0r", "invert");
    if (NULL == invertor) {
        printf("[ERROR] Couldn't create invert element. Is frei0r installed?\n");
        return -1;
    }

    GstElement *videoflip = gst_element_factory_make("videoflip", "flip");
    if (NULL == videoflip) {
        printf("[ERROR] Couldn't create video flip element\n");
        return -1;
    }
    gst_util_set_object_arg(G_OBJECT(videoflip), "method", flip);

    GstElement *encoderConverter = gst_element_factory_make("videoconvert", "conv-encode");
    if (NULL == encoderConverter) {
        printf("[ERROR] Couldn't create encoder converter\n");
        return -1;
    }

    GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
    if (NULL == encoder) {
        printf("[ERROR] Couldn't create H264 encoder\n");
        return -1;
    }
    gst_util_set_object_arg(G_OBJECT(encoder), "tune", "zerolatency");

    GstElement *capsH264 = gst_element_factory_make("capsfilter", "caps-h264");
    if (NULL == capsH264) {
        printf("[ERROR] Couldn't create H264 caps filter\n");
        return -1;
    }

    GstCaps *capsH264Values = gst_caps_new_empty_simple("video/x-h264");
    if (NULL == capsH264Values) {
        printf("[ERROR] Couldn't create H264 caps\n");
        return -1;
    }
    g_object_set(capsH264, "caps", capsH264Values, NULL);
    gst_caps_unref(capsH264Values);

    GstElement *parser = gst_element_factory_make("h264parse", "parser");
    if (NULL == parser) {
        printf("[ERROR] Couldn't create H264 parser\n");
        return -1;
    }

    GstElement *decoder = gst_element_factory_make("avdec_h264", "decoder");
    if (NULL == decoder) {
        printf("[ERROR] Couldn't create H264 decoder\n");
        return -1;
    }

    GstElement *decoderConverter = gst_element_factory_make("videoconvert", "conv-sink");
    if (NULL == decoderConverter) {
        printf("[ERROR] Couldn't create sink converter\n");
        return -1;
    }

    GstElement *screen = gst_element_factory_make("autovideosink", "sink");
    if (NULL == screen) {
        printf("[ERROR] Couldn't create output element\n");
        return -1;
    }
    g_object_set(screen, "sync", FALSE, NULL);

    gst_bin_add_many(GST_BIN(pipeline), src, capsfilter, convert, scaler, capsScaler,
        invertorConverter, invertor, videoflip, encoderConverter, encoder, 
        capsH264, parser, decoder, decoderConverter, screen, NULL);

    gRet = gst_element_link_many(src, capsfilter, convert, scaler, capsScaler,
        invertorConverter, invertor, videoflip, encoderConverter, encoder, 
        capsH264, parser, decoder, decoderConverter, screen, NULL);
    if (FALSE == gRet) {
        printf("[ERROR] Couldn't link elements in pipeline\n");
        gst_object_unref(pipeline);
        return -1;
    }

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        printf("[ERROR] Couldn't start the pipeline\n");
        gst_object_unref(pipeline);
        return -1;
    }

    printf("[INFO] Running at %dx%d, flip=%s. Press Ctrl+C to stop.\n",
        scaleSize.width, scaleSize.height, flip);

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_EOS));

    if (msg) {
        GError *e = NULL;
        gchar *dbg = NULL;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &e, &dbg);
            printf("[ERROR] From %s: %s\n", GST_OBJECT_NAME(msg->src), e->message);
            printf("[DEBUG] %s\n", dbg ? dbg : "(no details)");
            break;
        case GST_MESSAGE_WARNING:
            gst_message_parse_warning(msg, &e, &dbg);
            printf("[WARN] From %s: %s\n", GST_OBJECT_NAME(msg->src), e->message);
            printf("[DEBUG] %s\n", dbg ? dbg : "(no details)");
            break;
        case GST_MESSAGE_EOS:
            printf("[INFO] End of stream\n");
            break;
        default:
            break;
        }

        g_clear_error(&e);
        g_free(dbg);
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    g_free(flipOpt);
    g_free(deviceOpt);

    return 0;
}