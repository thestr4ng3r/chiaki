// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

package com.metallic.chiaki.common.ext

import androidx.lifecycle.LiveDataReactiveStreams
import io.reactivex.BackpressureStrategy
import io.reactivex.Observable
import io.reactivex.Single
import org.reactivestreams.Publisher

fun <T> Publisher<T>.toLiveData() = LiveDataReactiveStreams.fromPublisher(this)
fun <T> Observable<T>.toLiveData() = this.toFlowable(BackpressureStrategy.LATEST).toLiveData()
fun <T> Single<T>.toLiveData() = this.toFlowable().toLiveData()