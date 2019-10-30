/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

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
