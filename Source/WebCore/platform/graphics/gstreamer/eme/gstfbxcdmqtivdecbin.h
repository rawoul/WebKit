#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_FBXCDMQTIVDECBIN (gst_fbxcdm_qtivdec_bin_get_type())
#define GST_FBXCDMQTIVDECBIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_FBXCDMQTIVDECBIN, GstFbxcdmQtivdecBin))
#define GST_FBXCDMQTIVDECBIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_FBXCDMQTIVDECBIN, GstFbxcdmQtivdecBinClass))
#define GST_IS_FBXCDMQTIVDECBIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_FBXCDMQTIVDECBIN))
#define GST_IS_FBXCDMQTIVDECBIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_FBXCDMQTIVDECBIN))

typedef struct _GstFbxcdmQtivdecBin GstFbxcdmQtivdecBin;
typedef struct _GstFbxcdmQtivdecBinClass GstFbxcdmQtivdecBinClass;

struct _GstFbxcdmQtivdecBin {
    GstBin bin;
};

struct _GstFbxcdmQtivdecBinClass {
    GstBinClass parent_class;
};

GType gst_fbxcdm_qtivdec_bin_get_type(void);

G_END_DECLS

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
