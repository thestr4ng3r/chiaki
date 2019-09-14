
#include <jni.h>
#include <android/log.h>

#include <chiaki/common.h>
#include <chiaki/log.h>
#include <chiaki/session.h>

#include <string.h>

#define LOG_TAG "Chiaki"

static void log_cb_android(ChiakiLogLevel level, const char *msg, void *user)
{
    int prio;
    switch(level)
    {
        case CHIAKI_LOG_DEBUG:
            prio = ANDROID_LOG_DEBUG;
            break;
        case CHIAKI_LOG_VERBOSE:
            prio = ANDROID_LOG_VERBOSE;
            break;
        case CHIAKI_LOG_INFO:
            prio = ANDROID_LOG_INFO;
            break;
        case CHIAKI_LOG_WARNING:
            prio = ANDROID_LOG_ERROR;
            break;
        case CHIAKI_LOG_ERROR:
            prio = ANDROID_LOG_ERROR;
            break;
        default:
            prio = ANDROID_LOG_INFO;
            break;
    }
    __android_log_write(prio, LOG_TAG, msg);
}

ChiakiLog log_ctx;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    chiaki_log_init(&log_ctx, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, log_cb_android, NULL);
    CHIAKI_LOGI(&log_ctx, "Loading Chiaki Library");
    ChiakiErrorCode err = chiaki_lib_init();
    CHIAKI_LOGI(&log_ctx, "Chiaki Library Init Result: %s\n", chiaki_error_string(err));
    return JNI_VERSION_1_2;
}

#define E (*env)

JNIEXPORT jstring JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_errorCodeToString(JNIEnv *env, jobject obj, jint value)
{
    return E->NewStringUTF(env, chiaki_error_string((ChiakiErrorCode)value));
}

JNIEXPORT void JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_createSession(JNIEnv *env, jobject obj, jobject result, jobject connect_info_obj)
{
    ChiakiSession *session = NULL;
    ChiakiErrorCode err = CHIAKI_ERR_SUCCESS;
    char *host_str = NULL;

    jclass result_class = E->GetObjectClass(env, result);

    jclass connect_info_class = E->GetObjectClass(env, connect_info_obj);
    jstring host_string = E->GetObjectField(env, connect_info_obj, E->GetFieldID(env, connect_info_class, "host", "Ljava/lang/String;"));
    jbyteArray regist_key_array = E->GetObjectField(env, connect_info_obj, E->GetFieldID(env, connect_info_class, "registKey", "[B"));
    jbyteArray morning_array = E->GetObjectField(env, connect_info_obj, E->GetFieldID(env, connect_info_class, "morning", "[B"));
    jobject connect_video_profile_obj = E->GetObjectField(env, connect_info_obj, E->GetFieldID(env, connect_info_class, "videoProfile", "Lcom/metallic/chiaki/lib/ConnectVideoProfile;"));
    jclass connect_video_profile_class = E->GetObjectClass(env, connect_video_profile_obj);

    ChiakiConnectInfo connect_info;
    const char *str_borrow = E->GetStringUTFChars(env, host_string, NULL);
    connect_info.host = host_str = strdup(str_borrow);
    E->ReleaseStringUTFChars(env, host_string, str_borrow);
    if(!connect_info.host)
    {
        err = CHIAKI_ERR_MEMORY;
        goto beach;
    }

    if(E->GetArrayLength(env, regist_key_array) != sizeof(connect_info.regist_key))
    {
        CHIAKI_LOGE(&log_ctx, "Regist Key passed from Java has invalid length");
        err = CHIAKI_ERR_INVALID_DATA;
        goto beach;
    }
    jbyte *bytes = E->GetByteArrayElements(env, regist_key_array, NULL);
    memcpy(connect_info.regist_key, bytes, sizeof(connect_info.regist_key));
    E->ReleaseByteArrayElements(env, regist_key_array, bytes, JNI_ABORT);

    if(E->GetArrayLength(env, morning_array) != sizeof(connect_info.morning))
    {
        CHIAKI_LOGE(&log_ctx, "Morning passed from Java has invalid length");
        err = CHIAKI_ERR_INVALID_DATA;
        goto beach;
    }
    bytes = E->GetByteArrayElements(env, morning_array, NULL);
    memcpy(connect_info.morning, bytes, sizeof(connect_info.morning));
    E->ReleaseByteArrayElements(env, morning_array, bytes, JNI_ABORT);


    connect_info.video_profile.width = (unsigned int)E->GetIntField(env, connect_video_profile_obj, E->GetFieldID(env, connect_video_profile_class, "width", "I"));
    connect_info.video_profile.height = (unsigned int)E->GetIntField(env, connect_video_profile_obj, E->GetFieldID(env, connect_video_profile_class, "height", "I"));
    connect_info.video_profile.max_fps = (unsigned int)E->GetIntField(env, connect_video_profile_obj, E->GetFieldID(env, connect_video_profile_class, "maxFPS", "I"));
    connect_info.video_profile.bitrate = (unsigned int)E->GetIntField(env, connect_video_profile_obj, E->GetFieldID(env, connect_video_profile_class, "bitrate", "I"));

    session = CHIAKI_NEW(ChiakiSession);
    if(!session)
    {
        err = CHIAKI_ERR_MEMORY;
        goto beach;
    }

    err = chiaki_session_init(session, &connect_info, &log_ctx);

beach:
    free(host_str);
    E->SetIntField(env, result, E->GetFieldID(env, result_class, "errorCode", "I"), (jint)err);
    E->SetLongField(env, result, E->GetFieldID(env, result_class, "sessionPtr", "J"), (jlong)session);
}