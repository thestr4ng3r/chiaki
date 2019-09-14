
#include <jni.h>

#include <chiaki/common.h>

JNIEXPORT jstring JNICALL Java_com_metallic_chiaki_lib_Chiaki_stringFromJNI(JNIEnv* env, jobject thiz)
{
    return (*env)->NewStringUTF(env, chiaki_error_string(CHIAKI_ERR_SUCCESS));
}
