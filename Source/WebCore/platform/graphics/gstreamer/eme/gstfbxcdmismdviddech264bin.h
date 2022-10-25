#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_FBXCDMISMDVIDDECH264BIN (gst_fbxcdm_ismdviddech264_bin_get_type())
#define GST_FBXCDMISMDVIDDECH264BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_FBXCDMISMDVIDDECH264BIN, GstFbxcdmIsmdviddech264Bin))
#define GST_FBXCDMISMDVIDDECH264BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_FBXCDMISMDVIDDECH264BIN, GstFbxcdmIsmdviddech264BinClass))
#define GST_IS_FBXCDMISMDVIDDECH264BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_FBXCDMISMDVIDDECH264BIN))
#define GST_IS_FBXCDMISMDVIDDECH264BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_FBXCDMISMDVIDDECH264BIN))

typedef struct _GstFbxcdmIsmdviddech264Bin GstFbxcdmIsmdviddech264Bin;
typedef struct _GstFbxcdmIsmdviddech264BinClass GstFbxcdmIsmdviddech264BinClass;

struct _GstFbxcdmIsmdviddech264Bin {
    GstBin bin;
};

struct _GstFbxcdmIsmdviddech264BinClass {
    GstBinClass parent_class;
};

GType gst_fbxcdm_ismdviddech264_bin_get_type(void);

G_END_DECLS

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
