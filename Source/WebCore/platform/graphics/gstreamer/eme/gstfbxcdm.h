#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <gst/gst.h>

#include "WebKitCommonEncryptionDecryptorGStreamer.h"

G_BEGIN_DECLS

#define GST_TYPE_FBXCDM (gst_fbxcdm_get_type())
#define GST_FBXCDM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_FBXCDM, GstFbxcdm))
#define GST_FBXCDM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_FBXCDM, GstFbxcdmClass))
#define GST_IS_FBXCDM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_FBXCDM))
#define GST_IS_FBXCDM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_FBXCDM))

typedef struct _GstFbxcdm GstFbxcdm;
typedef struct _GstFbxcdmClass GstFbxcdmClass;

struct GstFbxcdmPrivate;

struct _GstFbxcdm {
    WebKitMediaCommonEncryptionDecrypt parent;

    GstFbxcdmPrivate* priv;
};

struct _GstFbxcdmClass {
    WebKitMediaCommonEncryptionDecryptClass parent_class;
};

GType gst_fbxcdm_get_type(void);

G_END_DECLS

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
