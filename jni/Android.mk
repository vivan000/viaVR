LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := viaVR
LOCAL_SRC_FILES := rendererWrapper.cpp videoRenderer.cpp videoInfo.cpp config.cpp log.cpp
LOCAL_SRC_FILES += decoders/rawDecoder.cpp decoders/patternGenerator.cpp
LOCAL_SRC_FILES += threads/helpers/renderingPass.cpp threads/helpers/shaderLoader.cpp threads/helpers/scalers.cpp
LOCAL_SRC_FILES += threads/decoder.cpp threads/renderer.cpp threads/presenter.cpp
LOCAL_SRC_FILES += frames/frameCPU.cpp frames/frameGPUu.cpp frames/frameGPUi.cpp frames/frameGPUo.cpp 
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
LOCAL_CPPFLAGS := -std=c++11 

include $(BUILD_SHARED_LIBRARY)