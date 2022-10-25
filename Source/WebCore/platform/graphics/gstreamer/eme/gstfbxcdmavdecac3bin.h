#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_FBXCDMAVDECAC3BIN (gst_fbxcdm_avdecac3_bin_get_type())
#define GST_FBXCDMAVDECAC3BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_FBXCDMAVDECAC3BIN, GstFbxcdmAvdecac3Bin))
#define GST_FBXCDMAVDECAC3BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_FBXCDMAVDECAC3BIN, GstFbxcdmAvdecac3BinClass))
#define GST_IS_FBXCDMAVDECAC3BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_FBXCDMAVDECAC3BIN))
#define GST_IS_FBXCDMAVDECAC3BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_FBXCDMAVDECAC3BIN))

typedef struct _GstFbxcdmAvdecac3Bin GstFbxcdmAvdecac3Bin;
typedef struct _GstFbxcdmAvdecac3BinClass GstFbxcdmAvdecac3BinClass;

struct _GstFbxcdmAvdecac3Bin {
    GstBin bin;
};

struct _GstFbxcdmAvdecac3BinClass {
    GstBinClass parent_class;
};

GType gst_fbxcdm_avdecac3_bin_get_type(void);

G_END_DECLS

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
