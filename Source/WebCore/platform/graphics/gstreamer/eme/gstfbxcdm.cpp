#include "config.h"
#include "gstfbxcdm.h"

#if ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)

#include <libfbxcdmdecrypt.h>

#include <gst/allocators/gstdmabuf.h>
#include <gst/base/gstbytereader.h>
#include <gst/gst.h>

#include <wtf/glib/WTFGType.h>

#include "GStreamerCommon.h"
#include "GStreamerEMEUtilities.h"

#include "CDMProxyFbxcdm.h"

using namespace WebCore;

static void constructed(GObject*);
static gboolean decide_allocation(GstBaseTransform* trans, GstQuery* query);
static gboolean transform_meta(GstBaseTransform* trans, GstBuffer* outbuf, GstMeta* meta, GstBuffer* inbuf);
static bool cdmProxyAttached(WebKitMediaCommonEncryptionDecrypt* decryptor, const RefPtr<CDMProxy>& cdmProxy);
static const char* protectionSystemId(WebKitMediaCommonEncryptionDecrypt* decryptor);
static bool decrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer*, GstBuffer*, GstBuffer*, unsigned, GstBuffer*);
static bool decrypt2(WebKitMediaCommonEncryptionDecrypt* decryptor, GstBuffer* inbuf, GstBuffer* outbuf);

struct GstFbxcdmPrivate {
    RefPtr<CDMProxyFbxcdm> cdmProxy;
    struct fbxcdmdecrypt_client* m_fbxcdmdecrypt_client;
    struct fbxcdmdecrypt_buffer m_input_buf_desc;
    struct fbxcdmdecrypt_buffer m_output_buf_desc;

    GstFbxcdmPrivate();
    ~GstFbxcdmPrivate();
    bool decrypt(WebKitMediaCommonEncryptionDecrypt* decryptor, GstBuffer* inbuf, GstBuffer* outbuf);
};

GST_DEBUG_CATEGORY_STATIC(gst_fbxcdm_debug);
#define GST_CAT_DEFAULT gst_fbxcdm_debug

#define gst_fbxcdm_parent_class parent_class
WEBKIT_DEFINE_TYPE(GstFbxcdm, gst_fbxcdm, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void
gst_fbxcdm_class_init(GstFbxcdmClass* klass)
{
    GST_DEBUG_CATEGORY_INIT(gst_fbxcdm_debug, "fbxcdm", 0, "fbxcdm");

    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->constructed = constructed;

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(elementClass, "Decrypt encrypted content using Fbxcdm", GST_ELEMENT_FACTORY_KLASS_DECRYPTOR, "Decrypts encrypted media using Fbxcdm.", "Tanguy Rozier <trozier@freebox.fr>");

    {
        GstCaps* caps = gst_caps_new_empty();

        gst_caps_append(caps, gst_caps_from_string("application/x-cenc, "
                                                   "  original-media-type = (string) video/x-h264; "

                                                   "application/x-cenc, "
                                                   "  original-media-type = (string) video/x-h265; "

                                                   "application/x-cenc, "
                                                   "  original-media-type = (string) audio/mpeg, "
                                                   "  mpegversion = (int) { 2, 4 }; "

                                                   "application/x-cenc, "
                                                   "  original-media-type = (string) audio/x-ac3; "

                                                   "application/x-cenc, "
                                                   "  original-media-type = (string) audio/x-eac3; "));

        unsigned size = gst_caps_get_size(caps);

        auto& supportedKeySystems = CDMProxyFactoryFbxcdm::singleton().supportedKeySystems();

        for (unsigned i = 0; i < size; ++i) {

            GstStructure* incomingStructure = gst_caps_get_structure(caps, i);

            for (const auto& keySystem : supportedKeySystems) {

                GstStructure* outgoingStructure = gst_structure_copy(incomingStructure);

                gst_structure_set(outgoingStructure, "protection-system", G_TYPE_STRING, GStreamerEMEUtilities::keySystemToUuid(keySystem), nullptr);

                gst_caps_append_structure(caps, outgoingStructure);
            }
        }

        gst_element_class_add_pad_template(elementClass, gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, caps));
    }

    GRefPtr<GstCaps> gstSrcPadTemplateCaps = adoptGRef(gst_caps_new_empty());
    gst_caps_append(gstSrcPadTemplateCaps.get(),
        gst_caps_from_string("video/x-h264; "

                             "video/x-h265; "

                             "audio/mpeg, "
                             "  mpegversion = (int) { 2, 4 }; "

                             "audio/x-ac3; "

                             "audio/x-eac3; "));
    gst_element_class_add_pad_template(elementClass, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, gstSrcPadTemplateCaps.get()));

    GstBaseTransformClass* baseTransformClass = GST_BASE_TRANSFORM_CLASS(klass);
    baseTransformClass->decide_allocation = GST_DEBUG_FUNCPTR(decide_allocation);
    baseTransformClass->transform_meta = GST_DEBUG_FUNCPTR(transform_meta);
    baseTransformClass->passthrough_on_same_caps = FALSE;

    WebKitMediaCommonEncryptionDecryptClass* commonClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    commonClass->decrypt = GST_DEBUG_FUNCPTR(decrypt);
    commonClass->decrypt2 = GST_DEBUG_FUNCPTR(decrypt2);
    commonClass->protectionSystemId = GST_DEBUG_FUNCPTR(protectionSystemId);
    commonClass->cdmProxyAttached = GST_DEBUG_FUNCPTR(cdmProxyAttached);
}

GstFbxcdmPrivate::GstFbxcdmPrivate()
{
    m_fbxcdmdecrypt_client = fbxcdmdecrypt_connect(nullptr, nullptr);

    m_input_buf_desc.fd = -1;
    m_input_buf_desc.offset = 0;
    m_input_buf_desc.size = 0;
    m_input_buf_desc.secure = false;

    m_output_buf_desc.fd = -1;
    m_output_buf_desc.offset = 0;
    m_output_buf_desc.size = 0;
    m_output_buf_desc.secure = false;
}

GstFbxcdmPrivate::~GstFbxcdmPrivate()
{
    fbxcdmdecrypt_disconnect(m_fbxcdmdecrypt_client);
    m_fbxcdmdecrypt_client = nullptr;

    if (m_input_buf_desc.fd != -1) {
        close(m_input_buf_desc.fd);
        m_input_buf_desc.fd = -1;
    }

    if (m_output_buf_desc.fd != -1) {
        close(m_output_buf_desc.fd);
        m_output_buf_desc.fd = -1;
    }
}

static void constructed(GObject* object)
{
    GstBaseTransform* base = GST_BASE_TRANSFORM(object);

    G_OBJECT_CLASS(gst_fbxcdm_parent_class)->constructed(object);

    gst_base_transform_set_in_place(base, FALSE);
}

static gboolean
decide_allocation(GstBaseTransform* trans, GstQuery* query)
{
    (void)trans;
    (void)query;
    return TRUE;
}

static gboolean
transform_meta(GstBaseTransform* trans, GstBuffer* outbuf, GstMeta* meta, GstBuffer* inbuf)
{
    if (meta->info->api == GST_PROTECTION_META_API_TYPE)
        return FALSE;

    return GST_BASE_TRANSFORM_CLASS(parent_class)->transform_meta(trans, outbuf, meta, inbuf);
}

static bool
cdmProxyAttached(WebKitMediaCommonEncryptionDecrypt* decryptor, const RefPtr<CDMProxy>& cdmProxy)
{
    GstFbxcdm* self = GST_FBXCDM(decryptor);
    self->priv->cdmProxy = reinterpret_cast<CDMProxyFbxcdm*>(cdmProxy.get());
    return !!self->priv->cdmProxy;
}

static const char*
protectionSystemId(WebKitMediaCommonEncryptionDecrypt* decryptor)
{
    GstFbxcdm* self = GST_FBXCDM(decryptor);
    return GStreamerEMEUtilities::keySystemToUuid(self->priv->cdmProxy->keySystem());
}

static bool
decrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer*, GstBuffer*, GstBuffer*, unsigned, GstBuffer*)
{
    return false;
}

static bool
decrypt2(WebKitMediaCommonEncryptionDecrypt* decryptor, GstBuffer* inbuf, GstBuffer* outbuf)
{
    GstFbxcdm* self = GST_FBXCDM(decryptor);
    return self->priv->decrypt(decryptor, inbuf, outbuf);
}

static void fitBuf(struct fbxcdmdecrypt_buffer& buf_desc, size_t data_len)
{
    size_t min = 2 * 1024 * 1024;

    if (data_len < min)
        data_len = min;

    size_t rem = data_len % 4096;

    if (rem > 0)
        data_len += 4096 - rem;

    if (data_len > buf_desc.size) {
        if (buf_desc.fd != -1)
            close(buf_desc.fd);

        buf_desc.fd = -1;
    }

    if (buf_desc.fd == -1)
        fbxcdmdecrypt_unsecure_alloc(&buf_desc, data_len);
}

bool GstFbxcdmPrivate::decrypt(WebKitMediaCommonEncryptionDecrypt* decryptor, GstBuffer* inbuf, GstBuffer* outbuf)
{
    GstFbxcdm* self = GST_FBXCDM(decryptor);

    if (!m_fbxcdmdecrypt_client)
        m_fbxcdmdecrypt_client = fbxcdmdecrypt_connect(nullptr, nullptr);
    if (!m_fbxcdmdecrypt_client) {
        GST_ERROR_OBJECT(self, "fbxcdmdecrypt_connect failed");
        return false;
    }

    struct fbxcdmdecrypt_command command;
    memset(&command, 0, sizeof(command));

    command.input.scheme = FBXCDMDECRYPT_SCHEME_NONE;
    command.keys = -1;

    command.flags = 0;

    command.pts = GST_BUFFER_PTS(inbuf);
    command.dts = GST_BUFFER_DTS(inbuf);

    {
        GstBaseTransform* base = GST_BASE_TRANSFORM(decryptor);
        GstPad* pad = GST_BASE_TRANSFORM_SINK_PAD(base);

        gchar* stream_id = gst_pad_get_stream_id(pad);
        snprintf(command.stream_name, sizeof(command.stream_name), "%s", stream_id);
        g_free(stream_id);

        GstCaps* caps = gst_pad_get_current_caps(pad);
        GstStructure* structure = gst_caps_get_structure(caps, 0);
        const gchar* media_type;

        media_type = gst_structure_get_string(structure, "original-media-type");
        if (!media_type)
            media_type = gst_structure_get_name(structure);

        if (g_str_has_prefix(media_type, "video")) {
            command.input.is_video = true;

            gint val;

            if (gst_structure_get_int(structure, "width", &val))
                command.input.video_width = val;

            if (gst_structure_get_int(structure, "height", &val))
                command.input.video_height = val;
        }
    }

    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(inbuf));
    if (protectionMeta) {
        gboolean encrypted = false;
        if (!gst_structure_get_boolean(protectionMeta->info, "encrypted", &encrypted)) {
            GST_ERROR_OBJECT(self, "Failed to get encrypted flag");
            return false;
        }

        if (encrypted)
            command.input.scheme = FBXCDMDECRYPT_SCHEME_CENC;

        bool pattern = false;

        const gchar* cipherModeBuf = gst_structure_get_string(protectionMeta->info, "cipher-mode");
        if (g_strcmp0(cipherModeBuf, "cenc") == 0) {
            command.input.scheme = FBXCDMDECRYPT_SCHEME_CENC;
        } else if (g_strcmp0(cipherModeBuf, "cbc1") == 0) {
            command.input.scheme = FBXCDMDECRYPT_SCHEME_CBC1;
        } else if (g_strcmp0(cipherModeBuf, "cens") == 0) {
            command.input.scheme = FBXCDMDECRYPT_SCHEME_CENS;
            pattern = true;
        } else if (g_strcmp0(cipherModeBuf, "cbcs") == 0) {
            command.input.scheme = FBXCDMDECRYPT_SCHEME_CBCS;
            pattern = true;
        }

        if (command.input.scheme != FBXCDMDECRYPT_SCHEME_NONE) {
            {
                const GValue* kidValue = gst_structure_get_value(protectionMeta->info, "kid");
                if (!kidValue) {
                    GST_ERROR_OBJECT(self, "Failed to get kidValue");
                    return false;
                }

                GstBuffer* kidBuffer = gst_value_get_buffer(kidValue);
                if (!kidBuffer) {
                    GST_ERROR_OBJECT(self, "Failed to get kidBuffer");
                    return false;
                }

                GstMappedBuffer kidMapped(kidBuffer, GST_MAP_READ);
                if (!kidMapped) {
                    GST_ERROR_OBJECT(self, "Failed to map kidBuffer");
                    return false;
                }

                auto kid = kidMapped.createVector();

                size_t kid_len = kid.size();

                if (kid_len > sizeof(command.input.kid))
                    kid_len = sizeof(command.input.kid);

                memcpy(&command.input.kid[0], kid.data(), kid_len);

                auto weakClientPtr = webKitMediaCommonEncryptionDecryptGetCDMProxyDecryptionClient(decryptor);

                command.keys = cdmProxy->getKeysID(kid, WTFMove(weakClientPtr));
                if (command.keys < 0) {
                    GST_ERROR_OBJECT(self, "Failed to get command.keys");
                    return false;
                }
            }

            {
                const GValue* ivValue = gst_structure_get_value(protectionMeta->info, "iv");
                if (!ivValue) {
                    GST_ERROR_OBJECT(self, "Failed to get ivValue");
                    return false;
                }

                GstBuffer* ivBuffer = gst_value_get_buffer(ivValue);
                if (!ivBuffer) {
                    GST_ERROR_OBJECT(self, "Failed to get ivBuffer");
                    return false;
                }

                GstMappedBuffer ivMapped(ivBuffer, GST_MAP_READ);
                if (!ivMapped) {
                    GST_ERROR_OBJECT(self, "Failed to map ivBuffer");
                    return false;
                }

                size_t iv_len = ivMapped.size();

                if (iv_len > sizeof(command.input.iv))
                    iv_len = sizeof(command.input.iv);

                memcpy(&command.input.iv[0], ivMapped.data(), iv_len);
            }

            {
                guint subsamplesCount = 0;

                if (!gst_structure_get_uint(protectionMeta->info, "subsample_count", &subsamplesCount) && !pattern) {
                    GST_ERROR_OBJECT(self, "Failed to get subsample_count");
                    return false;
                }

                if (subsamplesCount > 0) {
                    const GValue* subSamplesValue = gst_structure_get_value(protectionMeta->info, "subsamples");
                    if (!subSamplesValue) {
                        GST_ERROR_OBJECT(self, "Failed to get subsamples");
                        return false;
                    }

                    GstBuffer* subsamplesBuffer = gst_value_get_buffer(subSamplesValue);
                    if (!subsamplesBuffer) {
                        GST_ERROR_OBJECT(self, "There is no subsamples buffer, but a positive subsample count");
                        return false;
                    }

                    GstMappedBuffer subsamplesMapped(subsamplesBuffer, GST_MAP_READ);
                    if (!subsamplesMapped) {
                        GST_ERROR_OBJECT(self, "Failed to map subsamplesBuffer");
                        return false;
                    }

                    GUniquePtr<GstByteReader> reader(gst_byte_reader_new(subsamplesMapped.data(), subsamplesMapped.size()));
                    if (!reader)
                        return false;

                    struct fbxcdmdecrypt_subsample* it = &command.input.subsamples_info.subsamples[0];

                    for (guint i = 0; i < subsamplesCount; i++) {
                        if (!(it < &command.input.subsamples_info.subsamples[sizeof (command.input.subsamples_info.subsamples) / sizeof (command.input.subsamples_info.subsamples[0])]))
                            return false;

                        uint16_t clear;
                        if (!gst_byte_reader_get_uint16_be(reader.get(), &clear))
                            return false;

                        it->bytes_of_clear_data = clear;

                        uint32_t prot;
                        if (!gst_byte_reader_get_uint32_be(reader.get(), &prot))
                            return false;

                        it->bytes_of_protected_data = prot;

                        command.input.subsamples_info.nb_subsamples++;

                        it++;
                    }
                }
            }

            if (pattern) {
                guint val_crypt = 0;
                guint val_skip = 0;

                if (!gst_structure_get_uint(protectionMeta->info, "crypt_byte_block", &val_crypt) ||
                    !gst_structure_get_uint(protectionMeta->info, "skip_byte_block", &val_skip)) {
                    GST_DEBUG_OBJECT(self, "did not get crypt_byte_block and/or skip_byte_block, value is 0/0");
                    val_crypt = 0;
                    val_skip = 0;
                }

                command.input.crypt_byte_block = val_crypt;
                command.input.skip_byte_block = val_skip;
            }
        }
    }

    bool result;

    {
        if (!inbuf)
            return false;

        GstMappedBuffer inMapped(inbuf, GST_MAP_READ);
        if (!inMapped) {
            GST_ERROR_OBJECT(self, "Failed to map inbuf");
            return false;
        }

        fitBuf(m_input_buf_desc, inMapped.size());

        void* map_input = fbxcdmdecrypt_mmap(&m_input_buf_desc, inMapped.size());
        if (!map_input) {
            GST_ERROR_OBJECT(self, "fbxcdmdecrypt_mmap failed");
            return false;
        }

        memcpy(map_input, inMapped.data(), inMapped.size());

        fbxcdmdecrypt_munmap(&m_input_buf_desc, map_input, inMapped.size());
        map_input = nullptr;

        command.input.buf_desc = m_input_buf_desc;

        switch (command.input.scheme) {
        case FBXCDMDECRYPT_SCHEME_NONE:
            command.input.subsamples_info.nb_subsamples = 1;
            command.input.subsamples_info.subsamples[0].bytes_of_clear_data = inMapped.size();
            command.input.subsamples_info.subsamples[0].bytes_of_protected_data = 0;
            break;

        case FBXCDMDECRYPT_SCHEME_CENS:
        case FBXCDMDECRYPT_SCHEME_CBCS:
        case FBXCDMDECRYPT_SCHEME_CENC:
        case FBXCDMDECRYPT_SCHEME_CBC1:
            if (command.input.subsamples_info.nb_subsamples > 0)
                break;

            command.input.subsamples_info.nb_subsamples = 1;
            command.input.subsamples_info.subsamples[0].bytes_of_clear_data = 0;
            command.input.subsamples_info.subsamples[0].bytes_of_protected_data = inMapped.size();
            break;
        }

        struct fbxcdmdecrypt_reponse response;
        memset(&response, 0, sizeof(response));

        if (!outbuf)
            return false;

        GstMemory* mem = nullptr;

        if ((gst_buffer_n_memory(outbuf) == 1) && (mem = gst_buffer_peek_memory(outbuf, 0)) && gst_is_dmabuf_memory(mem)) {
            command.output.buf_desc.secure = true;

            command.output.buf_desc.fd = gst_dmabuf_memory_get_fd(mem);
            if (command.output.buf_desc.fd < 0)
                return false;

            gsize offset;
            gsize maxsize;
            gst_memory_get_sizes(mem, &offset, &maxsize);
            if (maxsize < inMapped.size())
                return false;

            command.output.buf_desc.offset = offset;
            command.output.buf_desc.size = maxsize;

            int r = fbxcdmdecrypt_decrypt_sync(m_fbxcdmdecrypt_client, &command, &response, 5 * 1000);
            if (r < 0)
                return false;

            result = (response.response_id == FBXCDMDECRYPT_RESPONSE_ID_SUCCESS);
        } else {
            fitBuf(m_output_buf_desc, inMapped.size());

            command.output.buf_desc = m_output_buf_desc;

            int r = fbxcdmdecrypt_decrypt_sync(m_fbxcdmdecrypt_client, &command, &response, 5 * 1000);
            if (r < 0)
                return false;

            GstMappedBuffer outMapped(outbuf, GST_MAP_READ | GST_MAP_WRITE);
            if (!outMapped) {
                GST_ERROR_OBJECT(self, "Failed to map outbuf");
                return false;
            }

            void* map_output = fbxcdmdecrypt_mmap(&m_output_buf_desc, inMapped.size());
            if (!map_output) {
                GST_ERROR_OBJECT(self, "fbxcdmdecrypt_mmap failed");
                return false;
            }

            memcpy(outMapped.data(), map_output, inMapped.size());

            fbxcdmdecrypt_munmap(&m_output_buf_desc, map_output, inMapped.size());
            map_output = nullptr;

            result = (response.response_id == FBXCDMDECRYPT_RESPONSE_ID_SUCCESS);
        }

        if (result) {
            gst_buffer_resize(outbuf, 0, inMapped.size());
        }
    }

    return result;
}

#endif // ENABLE(ENCRYPTED_MEDIA) && ENABLE(FBXCDM) && USE(GSTREAMER)
