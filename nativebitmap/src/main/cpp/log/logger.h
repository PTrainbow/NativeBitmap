//
// Created by ptrain on 2022/12/27.
//

#ifndef NATIVEBITMAP_LOGGER_H
#define NATIVEBITMAP_LOGGER_H
#define TAG "nativebitmap"

#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#endif //NATIVEBITMAP_LOGGER_H
