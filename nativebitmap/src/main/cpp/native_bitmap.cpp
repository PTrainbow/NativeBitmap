#include <jni.h>
#include <string>
#include <map>
#include <set>
#include <dlfcn.h>
#include "logger.h"
#include "shadowhook.h"
#include "bitmap_hook.h"

/**
 * inline hook
 * JavaPixelAllocator
 * 5.1-7.1 _ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable
 */
static void* hookJavaPixelAllocator() {
    void *allocatorStub = shadowhook_hook_sym_name(
            "libandroid_runtime.so", "_ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable",
            (void *) BitmapHook::allocateJavaPixelRefProxy,
            (void **) &BitmapHook::allocateJavaPixelRefOrigin
    );
    return allocatorStub;
}

/**
 * inline hook
 * addressof
 * 5.1 - 7.1 _ZN3artL19VMRuntime_addressOfEP7_JNIEnvP8_jobjectS3_
 */
static void* hookAddressOf() {
    const char *addressFunName = "_ZN3artL19VMRuntime_addressOfEP7_JNIEnvP8_jobjectS3_";
    void *addressStub = shadowhook_hook_sym_name(
            "libart.so", addressFunName, (void *) BitmapHook::addressOfProxy,
            (void **) &BitmapHook::addressOfOrigin
    );
    return addressStub;
}

/**
 * inline hook
 * DeleteWeakGlobalRefType
 * 5.1 - 7.1 _ZN3art3JNI19DeleteWeakGlobalRefEP7_JNIEnvP8_jobject
 */
static void* hookDeleteWeakGlobalRef() {
    const char *deleteFunName = "_ZN3art3JNI19DeleteWeakGlobalRefEP7_JNIEnvP8_jobject";
    void *deleteStub = shadowhook_hook_sym_name(
            "libart.so", deleteFunName, (void *) BitmapHook::deleteWeakGlobalRefProxy,
            (void **) &BitmapHook::deleteWeakGlobalRefOrigin
    );
    return deleteStub;
}

/**
 * inline hook
 * NewNonMovableArray
 * 5.1 - 7.1 _ZN3artL28VMRuntime_newNonMovableArrayEP7_JNIEnvP8_jobjectP7_jclassi
 */
static void* hookNewNonMovableArray() {
    const char *newNonMovableArraySymbol = "_ZN3artL28VMRuntime_newNonMovableArrayEP7_JNIEnvP8_jobjectP7_jclassi";
    void *arrayStub = shadowhook_hook_sym_name("libart.so", newNonMovableArraySymbol,
                                               (void *) BitmapHook::newNonMovableArrayProxy,
                                               (void **) &BitmapHook::newNonMovableArrayOrigin);
    return arrayStub;
}

extern "C" JNIEXPORT void JNICALL
Java_com_ptrain_nativebitmap_NativeBitmap_nInit(JNIEnv *env, jobject thiz, jboolean debug) {
    // hook
    if (BitmapHook::init(env)) {
        void* deleteStub = hookDeleteWeakGlobalRef();
        if (deleteStub != nullptr) {
            LOGI("delete hook successful!");
        } else {
            LOGE("init failed! delete hook failed!");
            return;
        }
        void* addressStub =  hookAddressOf();
        if (addressStub != nullptr) {
            LOGI("addressOf hook successful!");
        } else {
            shadowhook_unhook(deleteStub);
            LOGE("init failed! addressOf hook failed!");
            return;
        }
        void* arrayStub = hookNewNonMovableArray();
        if (arrayStub != nullptr) {
            LOGI("newNonMovableArray hook successful！");
        } else {
            shadowhook_unhook(deleteStub);
            shadowhook_unhook(addressStub);
            LOGE("init failed! newNonMovableArray hook failed！");
            return;
        }
        void* allocator = hookJavaPixelAllocator();
        if (allocator != nullptr) {
            LOGI("hookJavaPixelAllocator hook successful！");
        } else {
            shadowhook_unhook(deleteStub);
            shadowhook_unhook(addressStub);
            shadowhook_unhook(arrayStub);
            LOGE("init failed! hookJavaPixelAllocator hook failed！");
        }
    }
}
