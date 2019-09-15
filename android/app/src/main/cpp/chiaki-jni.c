
#include <jni.h>
#include <android/log.h>

#include <chiaki/common.h>
#include <chiaki/log.h>
#include <chiaki/session.h>

#include <string.h>

#define LOG_TAG "Chiaki"
#define JNI_VERSION JNI_VERSION_1_6

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

static char *strdup_jni(const char *str)
{
    if(!str)
        return NULL;
    char *r = strdup(str);
    if(!r)
        return NULL;
    for(char *c=r; *c; c++)
    {
        if(*c & (1 << 7))
            *c = '?';
    }
    return r;
}

static ChiakiLog global_log;
static JavaVM *global_vm;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    global_vm = vm;
    chiaki_log_init(&global_log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, log_cb_android, NULL);
    CHIAKI_LOGI(&global_log, "Loading Chiaki Library");
    ChiakiErrorCode err = chiaki_lib_init();
    CHIAKI_LOGI(&global_log, "Chiaki Library Init Result: %s\n", chiaki_error_string(err));
    return JNI_VERSION;
}

#define E (*env)

JNIEXPORT jstring JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_errorCodeToString(JNIEnv *env, jobject obj, jint value)
{
    return E->NewStringUTF(env, chiaki_error_string((ChiakiErrorCode)value));
}

JNIEXPORT jstring JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_quitReasonToString(JNIEnv *env, jobject obj, jint value)
{
    return E->NewStringUTF(env, chiaki_quit_reason_string((ChiakiQuitReason)value));
}

typedef struct android_chiaki_session_t
{
    ChiakiSession session;
    jobject java_session;
    jclass java_session_class;
    jmethodID java_session_event_login_pin_request_meth;
    jmethodID java_session_event_quit_meth;
} AndroidChiakiSession;

static JNIEnv *attach_thread_jni()
{
    JNIEnv *env;
    int r = (*global_vm)->GetEnv(global_vm, (void **)&env, JNI_VERSION);
    if(r == JNI_OK)
        return env;

    if((*global_vm)->AttachCurrentThread(global_vm, &env, NULL) == 0)
        return env;

    CHIAKI_LOGE(&global_log, "Failed to get JNIEnv from JavaVM or attach");
}

static void android_chiaki_event_cb(ChiakiEvent *event, void *user)
{
    AndroidChiakiSession *session = user;

    JNIEnv *env = attach_thread_jni();
    if(!env)
        return;

    switch(event->type)
    {
        case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
            E->CallVoidMethod(env, session->java_session,
                    session->java_session_event_login_pin_request_meth,
                    (jboolean)event->login_pin_request.pin_incorrect);
            break;
        case CHIAKI_EVENT_QUIT:
        {
            char *reason_str = strdup_jni(event->quit.reason_str);
            jstring reason_str_java = E->NewStringUTF(env, reason_str ? reason_str : "");
            E->CallVoidMethod(env, session->java_session,
                              session->java_session_event_quit_meth,
                              (jint)event->quit.reason,
                              reason_str_java);
            E->DeleteLocalRef(env, reason_str_java);
            free(reason_str);
            break;
        }
    }

    (*global_vm)->DetachCurrentThread(global_vm);
}

JNIEXPORT void JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_sessionCreate(JNIEnv *env, jobject obj, jobject result, jobject connect_info_obj, jobject java_session)
{
    AndroidChiakiSession *session = NULL;
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
        CHIAKI_LOGE(&global_log, "Regist Key passed from Java has invalid length");
        err = CHIAKI_ERR_INVALID_DATA;
        goto beach;
    }
    jbyte *bytes = E->GetByteArrayElements(env, regist_key_array, NULL);
    memcpy(connect_info.regist_key, bytes, sizeof(connect_info.regist_key));
    E->ReleaseByteArrayElements(env, regist_key_array, bytes, JNI_ABORT);

    if(E->GetArrayLength(env, morning_array) != sizeof(connect_info.morning))
    {
        CHIAKI_LOGE(&global_log, "Morning passed from Java has invalid length");
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

    session = CHIAKI_NEW(AndroidChiakiSession);
    if(!session)
    {
        err = CHIAKI_ERR_MEMORY;
        goto beach;
    }

    err = chiaki_session_init(&session->session, &connect_info, &global_log);

    if(err != CHIAKI_ERR_SUCCESS)
    {
        session->java_session = NULL;
        goto beach;
    }

    session->java_session = E->NewGlobalRef(env, java_session);
    session->java_session_class = E->GetObjectClass(env, session->java_session);
    session->java_session_event_login_pin_request_meth = E->GetMethodID(env, session->java_session_class, "eventLoginPinRequest", "(Z)V");
    session->java_session_event_quit_meth = E->GetMethodID(env, session->java_session_class, "eventQuit", "(ILjava/lang/String;)V");

    chiaki_session_set_event_cb(&session->session, android_chiaki_event_cb, session);

beach:
    free(host_str);
    E->SetIntField(env, result, E->GetFieldID(env, result_class, "errorCode", "I"), (jint)err);
    E->SetLongField(env, result, E->GetFieldID(env, result_class, "sessionPtr", "J"), (jlong)session);
}

JNIEXPORT void JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_sessionFree(JNIEnv *env, jobject obj, jlong ptr)
{
    AndroidChiakiSession *session = (AndroidChiakiSession *)ptr;
    if(!session)
        return;
    E->DeleteGlobalRef(env, session->java_session);
    chiaki_session_fini(&session->session);
    free(session);
}

JNIEXPORT jint JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_sessionStart(JNIEnv *env, jobject obj, jlong ptr)
{
    AndroidChiakiSession *session = (AndroidChiakiSession *)ptr;
    return chiaki_session_start(&session->session);
}

JNIEXPORT jint JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_sessionStop(JNIEnv *env, jobject obj, jlong ptr)
{
    AndroidChiakiSession *session = (AndroidChiakiSession *)ptr;
    return chiaki_session_stop(&session->session);
}

JNIEXPORT jint JNICALL Java_com_metallic_chiaki_lib_ChiakiNative_sessionJoin(JNIEnv *env, jobject obj, jlong ptr)
{
    AndroidChiakiSession *session = (AndroidChiakiSession *)ptr;
    return chiaki_session_join(&session->session);
}
