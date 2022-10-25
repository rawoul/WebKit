#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_FBXCDMAVDECEAC3BIN (gst_fbxcdm_avdeceac3_bin_get_type())
#define GST_FBXCDMAVDECEAC3BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_FBXCDMAVDECEAC3BIN, GstFbxcdmAvdeceac3Bin))
#define GST_FBXCDMAVDECEAC3BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_FBXCDMAVDECEAC3BIN, GstFbxcdmAvdeceac3BinClass))
#define GST_IS_FBXCDMAVDECEAC3BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_FBXCDMAVDECEAC3BIN))
#define GST_IS_FBXCDMAVDECEAC3BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_FBXCDMAVDECEAC3BIN))

typedef struct _GstFbxcdmAvdeceac3Bin GstFbxcdmAvdeceac3Bin;
typedef struct _GstFbxcdmAvdeceac3BinClass GstFbxcdmAvdeceac3BinClass;

struct _GstFbxcdmAvdeceac3Bin {
    GstBin bin;
};

struct _GstFbxcdmAvdeceac3BinClass {
    GstBinClass parent_class;
};

GType gst_fbxcdm_avdeceac3_bin_get_type(void);

G_END_DECLS

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
