// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

package com.metallic.chiaki.regist

import com.metallic.chiaki.lib.ChiakiLog
import io.reactivex.Observable
import io.reactivex.subjects.BehaviorSubject
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

class ChiakiRxLog(levelMask: Int)
{
	private val accSubject: BehaviorSubject<String> = BehaviorSubject.create<String>().also {
		it.onNext("")
	}
	private val accMutex = ReentrantLock()
	val logText: Observable<String> get() = accSubject

	val log = ChiakiLog(levelMask, callback = { level, text ->
		accMutex.withLock {
			val cur = accSubject.value ?: ""
			accSubject.onNext(cur + (if(cur.isEmpty()) "" else "\n") + ChiakiLog.formatLog(level, text))
		}
	})
}