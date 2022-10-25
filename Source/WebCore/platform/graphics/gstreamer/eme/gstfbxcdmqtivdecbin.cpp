#include "config.h"
#include "gstfbxcdmqtivdecbin.h"

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/allocators/gstdmabuf.h>
#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>

GST_DEBUG_CATEGORY_STATIC(gst_fbxcdm_qtivdec_bin_debug);
#define GST_CAT_DEFAULT gst_fbxcdm_qtivdec_bin_debug

#define gst_fbxcdm_qtivdec_bin_parent_class parent_class
G_DEFINE_TYPE(GstFbxcdmQtivdecBin, gst_fbxcdm_qtivdec_bin, GST_TYPE_BIN);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "application/x-cenc, "
        "  original-media-type = (string) video/x-h264, "
        "  width = (int) [ 32, 4096 ], "
        "  height = (int) [ 32, 4096 ], "
        "  stream-format = (string) byte-stream, "
        "  alignment = (string) au, "
        "  profile = (string) { baseline, constrained-baseline, main, high, constrained-high, progressive-high, scalable-baseline, scalable-constrained-baseline, scalable-high, scalable-constrained-high, scalable-high-intra }; "
        "application/x-cenc, "
        "  original-media-type = (string) video/x-h265, "
        "  width = (int) [ 32, 4096 ], "
        "  height = (int) [ 32, 4096 ], "
        "  stream-format = (string) byte-stream, "
        "  alignment = (string) au, "
        "  profile = (string) { main, main-intra, main-still-picture, main-10, main-10-intra }; "));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        GST_VIDEO_CAPS_MAKE("NV12") ";" GST_VIDEO_CAPS_MAKE_WITH_FEATURES(GST_CAPS_FEATURE_MEMORY_DMABUF, "{ NV12, ENCODED }")));

static void
gst_fbxcdm_qtivdec_bin_class_init(GstFbxcdmQtivdecBinClass* klass)
{
    GST_DEBUG_CATEGORY_INIT(gst_fbxcdm_qtivdec_bin_debug, "fbxcdmqtivdecbin", 0, "Fbxcdm and Qtivdec in a Bin");
    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "FbxcdmQtivdecBin", "Decoder", "Fbxcdm and Qtivdec in a Bin", "Tanguy ROZIER <trozier@freebox.fr>");
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass), gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass), gst_static_pad_template_get(&sink_factory));
}

static void
gst_fbxcdm_qtivdec_bin_init(GstFbxcdmQtivdecBin* fbxcdmQtivdecBin)
{
    GstElement* decryptor = NULL;
    GstElement* decoder = NULL;
    GstPad* sinkpad = NULL;
    GstPad* srcpad = NULL;
    GstPad* pad = NULL;
    gboolean b;

    GST_LOG_OBJECT(fbxcdmQtivdecBin, "gst_fbxcdm_qtivdec_bin_init");

    decryptor = gst_element_factory_make("fbxcdm", NULL);
    if (!decryptor)
        return;

    b = gst_bin_add(GST_BIN_CAST(fbxcdmQtivdecBin), decryptor);
    if (!b)
        return;

    decoder = gst_element_factory_make("qtivdec", NULL);
    if (!decoder)
        return;

    g_object_set(decoder, "secure", TRUE, NULL);

    b = gst_bin_add(GST_BIN_CAST(fbxcdmQtivdecBin), decoder);
    if (!b)
        return;

    b = gst_element_link(decryptor, decoder);
    if (!b)
        return;

    pad = gst_element_get_static_pad(decryptor, "sink");
    if (!pad)
        return;

    sinkpad = gst_ghost_pad_new("sink", pad);
    gst_object_unref(pad);
    pad = NULL;
    if (!sinkpad)
        return;

    b = gst_element_add_pad(GST_ELEMENT(fbxcdmQtivdecBin), sinkpad);
    if (!b)
        return;

    pad = gst_element_get_static_pad(decoder, "src");
    if (!pad)
        return;

    srcpad = gst_ghost_pad_new("src", pad);
    gst_object_unref(pad);
    pad = NULL;
    if (!srcpad)
        return;

    b = gst_element_add_pad(GST_ELEMENT(fbxcdmQtivdecBin), srcpad);
    if (!b)
        return;
}

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
