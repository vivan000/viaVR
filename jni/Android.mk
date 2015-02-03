LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := viaVR
LOCAL_SRC_FILES := rendererWrapper.cpp
LOCAL_SRC_FILES += rawDecoder.cpp
LOCAL_SRC_FILES += videoRenderer.cpp renderingPass.cpp shader.cpp videoInfo.cpp
LOCAL_SRC_FILES += frames/frameCPU.cpp frames/frameGPUu.cpp frames/frameGPUi.cpp frames/frameGPUo.cpp 
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
LOCAL_CPPFLAGS := -std=c++11 

include $(BUILD_SHARED_LIBRARY)