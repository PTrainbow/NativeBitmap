//
// Created by ptrain on 2023/1/2.
//
#include <dlfcn.h>
#include <cstdlib>
#include "bitmap_hook.h"
#include "logger.h"

namespace BitmapHook {
    void *newNonMovableArrayOrigin = nullptr;
    void *addressOfOrigin = nullptr;
    void *deleteWeakGlobalRefOrigin = nullptr;
    void *allocateJavaPixelRefOrigin = nullptr;

    int fakeArrSelfLen = sizeof(int) + sizeof(jobject) + sizeof(void *);
    int magicNum = 0x13572468;
    pthread_key_t canHook;
    jclass byteClazz;
    jobject VMRuntime;
    jmethodID registerNativeAllocationId, registerNativeFreeId;

    typedef void (*DeleteWeakGlobalRefType)(JNIEnv *env, jweak obj);

    typedef jlong (*AddressOfType)(JNIEnv *env, jobject, jobject javaArray);

    typedef jbyteArray (*NewNonMovableArrayType)(JNIEnv *env, jobject obj, jclass javaElementClass,
                                                 jint length);

    typedef void* (*AllocateJavaPixelRefType)(JNIEnv *env, void* param1, void* param2);

    bool init(JNIEnv *env) {
        // 创建一个 jbyteArray 获取其 jclass
        jbyteArray byteArray = env->NewByteArray(1);
        // 使用 GlobalRef 存储，不然无法引用
        byteClazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->GetObjectClass(byteArray)));
        env->DeleteLocalRef(byteArray);
        // 初始化 tls
        pthread_key_create(&canHook, nullptr);
        // 反射 VMRuntime 低版本不考虑 hidden api
        jclass dalvik = (jclass) env->NewGlobalRef(env->FindClass("dalvik/system/VMRuntime"));
        jmethodID getRuntime = env->GetStaticMethodID(dalvik, "getRuntime",
                                                      "()Ldalvik/system/VMRuntime;");
        VMRuntime = env->NewGlobalRef(env->CallStaticObjectMethod(dalvik, getRuntime));
        registerNativeFreeId = env->GetMethodID(dalvik, "registerNativeFree", "(I)V");
        registerNativeAllocationId = env->GetMethodID(dalvik, "registerNativeAllocation", "(I)V");
        return true;
    }

    /**
     * DeleteWeakGlobalRef 代理
     * 判断是 bitmap 申请的 byte[]，且带有我们插入的 magic number
     * 处理 fake array 逻辑(还原大小)，释放 native 内存，释放 newNonMovableArrayProxy 时创建的 global ref
     * 此时 fake array 为:
     * | kclass_ | uint32_t monitor_ | size(fake array 本身大小) | magic number | global ref | native byte[] pointer |
     *
     * @param env
     * @param obj
     */
    void deleteWeakGlobalRefProxy(JNIEnv *env, jweak obj) {
        if (env->IsSameObject(obj, nullptr) || addressOfOrigin == nullptr) {
            ((DeleteWeakGlobalRefType) deleteWeakGlobalRefOrigin)(env, obj);
            return;
        }
        jlong addr = 0;
        if (env->IsInstanceOf(obj, byteClazz)) {
            addr = ((AddressOfType) addressOfOrigin)(env, VMRuntime, obj);
        }
        if (addr != 0 && *(int32_t *) (addr) == magicNum) {
            jobject globalRef = *(jobject *) (addr + sizeof(int));
            void *bitmap = *(void **) (addr + sizeof(int) + sizeof(jobject));
            int realBitmapSize = *(int *) (addr - sizeof(int));
            *(int32_t *) (addr - sizeof(int)) = fakeArrSelfLen;
            env->DeleteGlobalRef(globalRef);
            if (bitmap != nullptr) {
//                LOGE("native bitmap free");
                free(bitmap);
                registerNativeFree(env, realBitmapSize);
            }
        }
        ((DeleteWeakGlobalRefType) deleteWeakGlobalRefOrigin)(env, obj);
    }

    /**
     * addressOf 代理
     *
     * 判断当前是否需要 hook(是来自 allocateJavaPixelRefProxy 的调用)
     * 申请 native 内存，修改 fake array
     * 此时 fake array 为:
     * | kclass_ | uint32_t monitor_ | size(Java 层真实申请的大小) | magic number | global ref | native byte[] pointer |
     *
     * @param env
     * @param obj
     * @param javaArray
     * @return
     */
    jlong addressOfProxy(JNIEnv *env, jobject obj, jbyteArray javaArray) {
//        LOGE("call  addressOfProxy");
        if (pthread_getspecific(canHook) == nullptr) {
//            LOGE("call  addressOfProxy origin! bitmap allocate null");
            return ((AddressOfType) addressOfOrigin)(env, obj, javaArray);
        }
//        LOGE("do addressOfProxy hook!");
        pthread_setspecific(canHook, nullptr);
        jlong addr = ((AddressOfType) addressOfOrigin)(env, obj, javaArray);
        bool isNativeBitmap = addr != 0 && *(int32_t *) addr == magicNum;
        if (isNativeBitmap) {
            jint bitmapSize = *(int32_t *) (addr - sizeof(int32_t));
//            LOGE("native bitmap malloc");
            void *bitmap = calloc(bitmapSize, 1);
            registerNativeAllocation(env, bitmapSize);
            *(void **) (addr + sizeof(int) + sizeof(jobject)) = bitmap;
            addr = reinterpret_cast<jlong>(bitmap);
        }
        return addr;
    }

    /**
     * newNonMovableArray 代理
     *
     * 判断当前是否需要 hook(是来自 allocateJavaPixelRefProxy 的调用)
     * 构造 fake array，global ref 存储
     * 此时 fake array 为:
     * | kclass_ | uint32_t monitor_ | size(Java 层真实申请的大小) | magic number | global ref | nullptr |
     *
     * @param env
     * @param obj
     * @param javaElementClass
     * @param bitmapSize
     * @return
     */
    jbyteArray newNonMovableArrayProxy(JNIEnv *env, jobject obj, jclass javaElementClass,
                                       jint bitmapSize) {
//        LOGE("call  newNonMovableArrayProxy");
        if (pthread_getspecific(canHook) == nullptr || addressOfOrigin == nullptr) {
//            LOGE("call  newNonMovableArray origin! bitmap is null");
            return ((NewNonMovableArrayType) newNonMovableArrayOrigin)(env, obj, javaElementClass, bitmapSize);
        }
        int fakeArraySize = fakeArrSelfLen;
        jbyteArray fakeArray = ((NewNonMovableArrayType) newNonMovableArrayOrigin)(env, obj,
                                                                                   javaElementClass,
                                                                                   fakeArraySize);
//        LOGE("do newNonMovableArrayProxy hook");
        jobject globalRef = env->NewGlobalRef(fakeArray);
        jlong fakeAddr = ((AddressOfType) addressOfOrigin)(env, VMRuntime, fakeArray);
        *(int32_t *) (fakeAddr - sizeof(int)) = bitmapSize;
        *(int32_t *) (fakeAddr) = magicNum;
        *(jobject *) (fakeAddr + sizeof(int)) = globalRef;
        if (bitmapSize != env->GetArrayLength(fakeArray)) {
            return ((NewNonMovableArrayType) newNonMovableArrayOrigin)(env, obj, javaElementClass,
                                                                       bitmapSize);
        } else {
            return fakeArray;
        }
    }

    void* allocateJavaPixelRefProxy(JNIEnv *env, void *param1, void *param2) {
        pthread_setspecific(canHook, (void *) (1));
        return ((AllocateJavaPixelRefType)allocateJavaPixelRefOrigin)(env, param1, param2);
    }

    void registerNativeAllocation(JNIEnv *env, int size) {
        env->CallVoidMethod(VMRuntime, registerNativeAllocationId, size);
    }

    void registerNativeFree(JNIEnv *env, int size) {
        env->CallVoidMethod(VMRuntime, registerNativeFreeId, size);
    }
}
