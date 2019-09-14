
#include <jni.h>
#include <android/log.h>

#include <chiaki/common.h>
#include <chiaki/log.h>

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
    __android_log_print(prio, LOG_TAG, "%s", msg);
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

JNIEXPORT jstring JNICALL Java_com_metallic_chiaki_lib_Chiaki_stringFromJNI(JNIEnv *env, jobject thiz)
{
    return (*env)->NewStringUTF(env, chiaki_error_string(CHIAKI_ERR_SUCCESS));
}
