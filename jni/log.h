#include <android/log.h>
#define LOGE(...) __android_log_print (ANDROID_LOG_ERROR, "viaVR", __VA_ARGS__)
#define LOGD(...) __android_log_print (ANDROID_LOG_DEBUG, "viaVR", __VA_ARGS__)
#define LOGI(...) __android_log_print (ANDROID_LOG_INFO, "viaVR", __VA_ARGS__)
