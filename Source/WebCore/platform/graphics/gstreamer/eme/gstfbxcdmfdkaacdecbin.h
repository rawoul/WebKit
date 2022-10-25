#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_FBXCDMFDKAACDECBIN (gst_fbxcdm_fdkaacdec_bin_get_type())
#define GST_FBXCDMFDKAACDECBIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_FBXCDMFDKAACDECBIN, GstFbxcdmFdkaacdecBin))
#define GST_FBXCDMFDKAACDECBIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_FBXCDMFDKAACDECBIN, GstFbxcdmFdkaacdecBinClass))
#define GST_IS_FBXCDMFDKAACDECBIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_FBXCDMFDKAACDECBIN))
#define GST_IS_FBXCDMFDKAACDECBIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_FBXCDMFDKAACDECBIN))

typedef struct _GstFbxcdmFdkaacdecBin GstFbxcdmFdkaacdecBin;
typedef struct _GstFbxcdmFdkaacdecBinClass GstFbxcdmFdkaacdecBinClass;

struct _GstFbxcdmFdkaacdecBin {
    GstBin bin;
};

struct _GstFbxcdmFdkaacdecBinClass {
    GstBinClass parent_class;
};

GType gst_fbxcdm_fdkaacdec_bin_get_type(void);

G_END_DECLS

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
