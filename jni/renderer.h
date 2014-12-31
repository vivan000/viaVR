#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <thread>
#include "frames.h"
#include "queue.h"
#include "videoDecoder.h"

class Renderer {
public:
	Renderer ();
	~Renderer ();
	bool addVideoDecoder (videoDecoder* video);
	void setSize (int width, int height);
	void setRefreshRate (double fps);
	bool init ();
	void drawFrame ();
	void play (int timecode);
	void pause ();

	// void setMode ();
	// void setSubtitleDecoder ();
	// void flush ();
	const char* getName ();
	const char* getVersion ();

private:
	int64_t nanotime ();
	int tcNow ();

	bool checkExtensions ();
	GLuint loadShader (GLenum type, const char *shaderSrc);
	GLuint loadProgram (GLuint vertexShader, GLuint fragmentShader);

	void setAspect ();
	void genContexts ();

	void presentFrame (frameGPUo* f);
	void getNextFrame (frameGPUo* f);

	void upload ();
	void decode ();
	void render ();
	void backbuffer ();

	// video config
	videoDecoder* video;
	int videoWidth, videoHeight;
	int videoSarWidth, videoSarHeight;
	double videoFps;
	pRange videoRange;
	pMatrix videoMatrix;
	pFormat videoFourCC;

	bool decoding, uploading, rendering, backbuffering, playing;

	queue<frameCPU>* decodeQueue;
	queue<frameGPUo>* uploadQueue;
	queue<frameGPUo>* renderQueue;
	queue<frameGPUo>* backbufferQueue;

	std::thread decodeThread;
	std::thread uploadThread;
	std::thread renderThread;
	std::thread backbufferThread;

	EGLContext mainContext;
	EGLContext uploadContext;
	EGLContext renderContext;
	EGLContext backbufferContext;

	EGLDisplay display;

	EGLSurface uploadPBuffer, uploadPBuffer2;
	EGLSurface renderPBuffer, renderPBuffer2;
	EGLSurface backbufferPBuffer, backbufferPBuffer2;

	bool storage16 = false;
	bool process16 = false;

	bool initialized;
	int64_t start;

	int surfaceWidth, surfaceHeight;
	double displayRefreshRate = 60.0;

	int targetWidth, targetHeight;
	int targetX, targetY;

	frameGPUo* displayCurr;
	double repeat, factor;
	bool newFrame;
	int frameNumber, presentedFrames;

	GLuint displaySP;
	GLuint vboIds[2];

	const static GLuint vertexCoordLoc;
	const static GLuint textureCoordLoc;

	const static char* displayVP;
	const static char* displayFP;
	const static float VertexPositions[];
	const static float VertexTexcoord[];

	const static char* up420to422FP;
	const static char* up422to444FP;
};
