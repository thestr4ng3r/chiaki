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

#include "log.h"

#include <android/log.h>

#include <string.h>
#include <errno.h>
#include <jni.h>

#include "chiaki-jni.h"

#define LOG_TAG "Chiaki"

void log_cb_android(ChiakiLogLevel level, const char *msg, void *user)
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

static void log_cb_android_file(ChiakiLogLevel level, const char *msg, void *user)
{
	log_cb_android(level, msg, user);
	FILE *f = user;
	if(!f)
		return;
	switch(level)
	{
		case CHIAKI_LOG_DEBUG:
			fwrite("[D] ", 4, 1, f);
			break;
		case CHIAKI_LOG_VERBOSE:
			fwrite("[V] ", 4, 1, f);
			break;
		case CHIAKI_LOG_INFO:
			fwrite("[I] ", 4, 1, f);
			break;
		case CHIAKI_LOG_WARNING:
			fwrite("[W] ", 4, 1, f);
			break;
		case CHIAKI_LOG_ERROR:
			fwrite("[E] ", 4, 1, f);
			break;
		default:
			fwrite("[?] ", 4, 1, f);
			break;
	}
	fwrite(msg, strlen(msg), 1, f);
	fwrite("\n", 1, 1, f);
}

ChiakiErrorCode android_chiaki_file_log_init(ChiakiLog *log, uint32_t level, const char *file)
{
	chiaki_log_init(log, level, log_cb_android, NULL);
	if(file)
	{
		FILE *f = fopen(file, "w+");
		if(!f)
		{
			CHIAKI_LOGE(log, "Failed to open log file %s for writing: %s", file, strerror(errno));
			return CHIAKI_ERR_UNKNOWN;
		}
		else
		{
			log->user = f;
			log->cb = log_cb_android_file;
			CHIAKI_LOGI(log, "Logging to file %s", file);
		}
	}
	return CHIAKI_ERR_SUCCESS;
}

void android_chiaki_file_log_fini(ChiakiLog *log)
{
	if(log->user)
	{
		FILE *f = log->user;
		fclose(f);
		log->user = NULL;
	}
}



static void android_chiaki_log_cb(ChiakiLogLevel level, const char *msg, void *user)
{
	log_cb_android(level, msg, NULL);

	AndroidChiakiJNILog *log = user;
	JNIEnv *env = attach_thread_jni();
	if(!env)
		return;
	E->CallVoidMethod(env, log->java_log, log->java_log_meth, (jint)level, jnistr_from_ascii(env, msg));
	(*global_vm)->DetachCurrentThread(global_vm);
}

void android_chiaki_jni_log_init(AndroidChiakiJNILog *log, JNIEnv *env, jobject java_log)
{
	log->java_log = E->NewGlobalRef(env, java_log);
	jclass log_class = E->GetObjectClass(env, log->java_log);
	log->java_log_meth = E->GetMethodID(env, log_class, "log", "(ILjava/lang/String;)V");
	log->log.level_mask = (uint32_t)E->GetIntField(env, log->java_log, E->GetFieldID(env, log_class, "levelMask", "I"));
	log->log.cb = android_chiaki_log_cb;
	log->log.user = log;
}

void android_chiaki_jni_log_fini(AndroidChiakiJNILog *log, JNIEnv *env)
{
	E->DeleteGlobalRef(env, log->java_log);
}