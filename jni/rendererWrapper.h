extern "C" {
	JNIEXPORT void JNICALL Java_vivan_viavr_MainActivity_nativeOnStart (JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_vivan_viavr_MainActivity_nativeOnResume (JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_vivan_viavr_MainActivity_nativeOnPause (JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_vivan_viavr_MainActivity_nativeOnStop (JNIEnv* jenv, jobject obj);
	JNIEXPORT void JNICALL Java_vivan_viavr_MainActivity_nativeSetSurface (JNIEnv* jenv, jobject obj, jobject surface);
};
