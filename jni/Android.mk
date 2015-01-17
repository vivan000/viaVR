LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := viaVR
LOCAL_SRC_FILES := shader.cpp videoRenderer.cpp rendererWrapper.cpp jni.cpp
LOCAL_SRC_FILES += videoDecoder.cpp videoInfo.cpp
LOCAL_SRC_FILES += frames/frameCPU.cpp frames/frameGPUu.cpp frames/frameGPUi.cpp frames/frameGPUo.cpp 
LOCAL_LDLIBS := -llog -lEGL -lGLESv3
LOCAL_CPPFLAGS := -std=c++11 

include $(BUILD_SHARED_LIBRARY)