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