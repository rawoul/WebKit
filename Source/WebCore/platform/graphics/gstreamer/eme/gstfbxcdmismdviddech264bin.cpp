#include "config.h"
#include "gstfbxcdmismdviddech264bin.h"

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/allocators/gstdmabuf.h>
#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>

GST_DEBUG_CATEGORY_STATIC(gst_fbxcdm_ismdviddech264_bin_debug);
#define GST_CAT_DEFAULT gst_fbxcdm_ismdviddech264_bin_debug

#define gst_fbxcdm_ismdviddech264_bin_parent_class parent_class
G_DEFINE_TYPE(GstFbxcdmIsmdviddech264Bin, gst_fbxcdm_ismdviddech264_bin, GST_TYPE_BIN);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "application/x-cenc, "
        "  original-media-type = (string) video/x-h264, "
        "  stream-format = (string) byte-stream, "
        "  alignment = (string) au; "));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        GST_VIDEO_CAPS_MAKE("NV12")));

static void
gst_fbxcdm_ismdviddech264_bin_class_init(GstFbxcdmIsmdviddech264BinClass* klass)
{
    GST_DEBUG_CATEGORY_INIT(gst_fbxcdm_ismdviddech264_bin_debug, "fbxcdmismdviddech264bin", 0, "Fbxcdm and Ismdviddec_h264 in a Bin");
    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "FbxcdmIsmdviddech264Bin", "Decoder", "Fbxcdm and Ismdviddec_h264 in a Bin", "Tanguy ROZIER <trozier@freebox.fr>");
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass), gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass), gst_static_pad_template_get(&sink_factory));
}

static void
gst_fbxcdm_ismdviddech264_bin_init(GstFbxcdmIsmdviddech264Bin* fbxcdmIsmdviddech264Bin)
{
    GstElement* decryptor = NULL;
    GstElement* decoder = NULL;
    GstPad* sinkpad = NULL;
    GstPad* srcpad = NULL;
    GstPad* pad = NULL;
    gboolean b;

    GST_LOG_OBJECT(fbxcdmIsmdviddech264Bin, "gst_fbxcdm_ismdviddech264_bin_init");

    decryptor = gst_element_factory_make("fbxcdm", NULL);
    if (!decryptor)
        return;

    b = gst_bin_add(GST_BIN_CAST(fbxcdmIsmdviddech264Bin), decryptor);
    if (!b)
        return;

    decoder = gst_element_factory_make("ismdviddec_h264", NULL);
    if (!decoder)
        return;

    b = gst_bin_add(GST_BIN_CAST(fbxcdmIsmdviddech264Bin), decoder);
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

    b = gst_element_add_pad(GST_ELEMENT(fbxcdmIsmdviddech264Bin), sinkpad);
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

    b = gst_element_add_pad(GST_ELEMENT(fbxcdmIsmdviddech264Bin), srcpad);
    if (!b)
        return;
}

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
