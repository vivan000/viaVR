#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <thread>
#include "frames.h"
#include "queue.h"
#include "videoDecoder.h"

class videoRenderer {
public:
	videoRenderer ();
	~videoRenderer ();
	bool addVideoDecoder (videoDecoder* video);
	void setSize (int width, int height);
	void setRefreshRate (int fps);
	bool init ();
	void drawFrame ();
	void play (int timecode);
	void pause ();

	// void setMode ();
	// void setSubtitleDecoder ();
	// void flush ();
	const char* getName ();
	int getMajorVersion ();
	int getMinorVersion ();

private:
	int64_t nanotime ();
	int tcNow ();

	bool checkExtensions ();

	void setAspect ();
	void genContexts ();

	void presentFrame (frameGPUo* f);
	void getNextFrame (frameGPUo* f);

	void upload ();
	void decode ();
	void render ();

	void getGlError ();
	void getFbStatus ();

	// video config
	videoDecoder* video;
	videoInfo* info;
	int videoSarWidth, videoSarHeight;
	int videoFps;

	bool decoding, uploading, rendering, playing;

	queue<frameCPU>* decodeQueue;
	queue<frameGPUu>* uploadQueue;
	queue<frameGPUo>* renderQueue;

	std::thread decodeThread;
	std::thread uploadThread;
	std::thread renderThread;

	EGLContext mainContext;
	EGLContext uploadContext;
	EGLContext renderContext;

	EGLDisplay display;

	EGLSurface uploadPBuffer, uploadPBuffer2;
	EGLSurface renderPBuffer, renderPBuffer2;

	bool storage16 = false;
	bool process16 = false;

	bool initialized;
	int64_t start;

	int surfaceWidth, surfaceHeight;
	int displayRefreshRate = 60;

	frameGPUo* displayCurr;
	int repeat, repeatLim;
	bool newFrame;
	int frameNumber, presentedFrames;
	int hardLate, softLate, softEarly, hardEarly;

	GLuint vboIds[2];

	const static char* displayVP;
	const static char* displayFP;
	const static float VertexPositions[];
	const static float VertexTexcoord[];

	const static char* up420to422FP;
	const static char* up422to444FP;
};
