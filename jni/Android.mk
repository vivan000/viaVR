LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := renderer
LOCAL_SRC_FILES := shader.cpp videoRenderer.cpp rendererWrapper.cpp jni.cpp
LOCAL_SRC_FILES += videoDecoder.cpp
LOCAL_SRC_FILES += frames/frames.cpp frames/frameCPU.cpp frames/frameGPUu.cpp frames/frameGPUi.cpp frames/frameGPUo.cpp 
LOCAL_LDLIBS := -llog -lEGL -lGLESv2
LOCAL_CPPFLAGS := -std=c++11 -pthread

include $(BUILD_SHARED_LIBRARY)