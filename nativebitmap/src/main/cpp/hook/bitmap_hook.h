//
// Created by ptrain on 2023/1/2.
//

#ifndef NATIVEBITMAP_BITMAP_HOOK_H
#define NATIVEBITMAP_BITMAP_HOOK_H

#include <jni.h>
#include <pthread.h>

namespace BitmapHook {

    extern void *newNonMovableArrayOrigin;
    extern void *addressOfOrigin;
    extern void *deleteWeakGlobalRefOrigin;
    extern void *allocateJavaPixelRefOrigin;

    bool init(JNIEnv *env);

    jbyteArray newNonMovableArrayProxy(JNIEnv *env, jobject obj, jclass javaElementClass,
                                              jint bitmapSize);

    jlong addressOfProxy(JNIEnv *env, jobject obj, jbyteArray javaArray);

    void deleteWeakGlobalRefProxy(JNIEnv *env, jweak obj);

    void* allocateJavaPixelRefProxy(JNIEnv *env, void *param1, void *param2);

    void registerNativeFree(JNIEnv *env, int size);

    void registerNativeAllocation(JNIEnv *env, int size);
};
#endif //NATIVEBITMAP_BITMAP_HOOK_H
