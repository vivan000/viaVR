#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "rendererWrapper.h"
#include "interface/IVideoRenderer.h"
#include "videoRenderer.h"
#include "decoders/rawDecoder.h"

static ANativeWindow* window = 0;
static IVideoRenderer* r = 0;
static rawDecoder video;

JNIEXPORT void Java_vivan_viavr_MainActivity_nativeOnStart (JNIEnv* jenv, jobject obj) {
}

JNIEXPORT void Java_vivan_viavr_MainActivity_nativeOnResume (JNIEnv* jenv, jobject obj) {
}

JNIEXPORT void Java_vivan_viavr_MainActivity_nativeOnPause (JNIEnv* jenv, jobject obj) {
	delete r;
	r = 0;
}

JNIEXPORT void Java_vivan_viavr_MainActivity_nativeOnStop (JNIEnv* jenv, jobject obj) {
}

JNIEXPORT void Java_vivan_viavr_MainActivity_nativeSetSurface (JNIEnv* jenv, jobject obj, jobject surface) {
	if (surface) {
		window = ANativeWindow_fromSurface (jenv, surface);
		video.seek (0);
		r = new videoRenderer;
		r->addWindow (window);
		r->addVideoDecoder (&video);
		r->init ();
		r->play (0);
	} else {
		ANativeWindow_release (window);
	}
}
