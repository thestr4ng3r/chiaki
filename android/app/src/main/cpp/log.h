// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_JNI_LOG_H
#define CHIAKI_JNI_LOG_H

#include <chiaki/log.h>

#include <jni.h>

typedef struct android_jni_chiaki_log_t
{
    jobject java_log;
    jmethodID java_log_meth;
    ChiakiLog log;
} AndroidChiakiJNILog;

ChiakiErrorCode android_chiaki_file_log_init(ChiakiLog *log, uint32_t level, const char *file);
void android_chiaki_file_log_fini(ChiakiLog *log);

void android_chiaki_jni_log_init(AndroidChiakiJNILog *log, JNIEnv *env, jobject java_log);
void android_chiaki_jni_log_fini(AndroidChiakiJNILog *log, JNIEnv *env);

#endif
