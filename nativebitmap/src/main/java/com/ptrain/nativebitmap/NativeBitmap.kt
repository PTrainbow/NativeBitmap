package com.ptrain.nativebitmap

import android.os.Build
import android.util.Log
import com.bytedance.shadowhook.ShadowHook
import java.util.concurrent.atomic.AtomicBoolean

object NativeBitmap {
    const val TAG = "nativebitmap"
    private val isInitialized = AtomicBoolean(false)

    fun init(isDebug: Boolean) {
        Thread {
            if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP_MR1 ||
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
            ) {
                Log.e(TAG, "init failed! unSupport os version!")
                return@Thread
            }
            if (isInitialized.compareAndSet(false, true)) {
                ShadowHook.init(
                    ShadowHook.ConfigBuilder().setDebuggable(BuildConfig.DEBUG)
                        .setMode(ShadowHook.Mode.UNIQUE).build()
                )
                System.loadLibrary(TAG)
                nInit(isDebug)
                Log.i(TAG, "init done!")
            } else {
                Log.e(TAG, "has been initialized!")
            }
        }.start()
    }

    private external fun nInit(isDebug: Boolean)
}