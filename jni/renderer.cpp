#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include <vector>
#include <android/log.h>
#include "renderer.h"

#define ALOG(...) __android_log_print (ANDROID_LOG_INFO, "viaVR", __VA_ARGS__)

const float Renderer::VertexPositions[] = {
	-1.0f,  1.0f, 0.0f, 1.0f,
	 1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f};

const float Renderer::VertexTexcoord[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f};

const char* Renderer::displayVP =
	"attribute vec4 displayVC;											\n"
	"attribute vec2 displayTC;								 		    \n"
	"                                                                   \n"
	"varying   vec2 displayTCI;											\n"
	"																	\n"
	"void main () {														\n"
	"    gl_Position = displayVC;										\n"
	"    displayTCI = displayTC;										\n"
	"}																	\n";
const char* Renderer::displayFP =
	"precision lowp float;												\n"
	"																	\n"
	"uniform   sampler2D displayT;										\n"
	"varying   vec2      displayTCI;									\n"
	"																	\n"
	"void main () {														\n"
	"    gl_FragColor = texture2D (displayT, displayTCI);				\n"
	"}																	\n";

/*
	NV12 (LUMINANCE, LUMINANCE_ALPHA)
	y = t1.r/255.0
	u = t2.r/255.0
	v = t2.a/255.0

	P010 (LUMINANCE_ALPHA, LUMINANCE_ALPHA, LUMINANCE_ALPHA)
	y = t1.r/255.0 + t1.a/65535.0
	u = t2.r/255.0 + t2.a/65535.0
	v = t3.r/255.0 + t3.a/65535.0
*/

const GLuint Renderer::displayVCLoc = 0;
const GLuint Renderer::displayTCLoc = 1;

Renderer::Renderer () {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	initialized = false;

	presentedFrames = 0;
	repeat = 0;
	frameNumber = 0;
}

Renderer::~Renderer () {
	decoding = false;
	uploading = false;
	//rendering = false;
	//backbuffering = false;
	playing = false;

	decodeThread.join ();
	uploadThread.join ();
	//rendererThread.join ();
	//backbufferThread.join ();

	delete decodeQueue;
	delete uploadQueue;
	//delete rendererQueue;
	//delete backbufferQueue;

	delete displayCurr;
}

const char* getName () {
	return "viaVR";
}

const char* getVersion () {
	return "0.1";
}

int64_t Renderer::nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

int Renderer::tcNow () {
	return (int) ((nanotime () - start) / 1000000);
}

bool Renderer::addVideoDecoder (videoDecoder* video) {
	Renderer::video = video;

	videoWidth = 		video->getWidth ();
	videoHeight = 		video->getHeight ();
	videoSarWidth =		video->getSarWidth ();
	videoSarHeight =	video->getSarHeight ();
	videoFps = 			(double) video->getFpsNumerator () / video->getFpsDenominator();

	switch (video->getRange ()) {
		case (int) pRange::UNKNOWN:
		case (int) pRange::TV:
			videoRange = pRange::TV;
			break;
		case (int) pRange::PC:
			videoRange = pRange::PC;
			break;
		default:
			return false;
	}

	switch (video->getMatrix ()) {
		case (int) pMatrix::UNKNOWN:
			videoMatrix = videoWidth > 1024 ? pMatrix::BT709 : pMatrix::BT601;
			break;
		case (int) pMatrix::BT601:
			videoMatrix = pMatrix::BT601;
			break;
		case (int) pMatrix::BT709:
			videoMatrix = pMatrix::BT709;
			break;
		default:
			return false;
	}

	switch (video->getFourCC ()) {
		case (int) pFormat::P008:
			videoFourCC = pFormat::P008;
			break;
		case (int) pFormat::P208:
			videoFourCC = pFormat::P208;
			break;
		case (int) pFormat::P408:
			videoFourCC = pFormat::P408;
			break;
		case (int) pFormat::P010:
			videoFourCC = pFormat::P010;
			break;
		case (int) pFormat::P210:
			videoFourCC = pFormat::P210;
			break;
		case (int) pFormat::P410:
			videoFourCC = pFormat::P410;
			break;
		case (int) pFormat::P016:
			videoFourCC = pFormat::P016;
			break;
		case (int) pFormat::P216:
			videoFourCC = pFormat::P216;
			break;
		case (int) pFormat::P416:
			videoFourCC = pFormat::P416;
			break;
		case (int) pFormat::NV12:
			videoFourCC = pFormat::NV12;
			break;
		case (int) pFormat::NV21:
			videoFourCC = pFormat::NV21;
			break;
		case (int) pFormat::RGBA:
			videoFourCC = pFormat::RGBA;
			break;
		default:
			return false;
	}

	return true;
}

void Renderer::setSize (int width, int height) {
}

void Renderer::setRefreshRate (double fps) {
	displayRefreshRate = fps;
}

bool Renderer::init () {
	genContexts ();
	setAspect ();

	// check extenions
	if (!checkExtensions ())
		return false;

	// load shaders
	GLuint hVertexShader = loadShader (GL_VERTEX_SHADER, displayVP);
	GLuint hFragmentShader = loadShader (GL_FRAGMENT_SHADER, displayFP);
	if (!hVertexShader || !hFragmentShader)
		return false;

	// load program
	displaySP = loadProgram (hVertexShader, hFragmentShader);
	if (!displaySP)
		return false;

	// load coordinates
	glGenBuffers (2, vboIds);
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexPositions), VertexPositions, GL_STATIC_DRAW);
	glVertexAttribPointer(displayVCLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (displayVCLoc);

	glBindBuffer (GL_ARRAY_BUFFER, vboIds[1]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexTexcoord), VertexTexcoord, GL_STATIC_DRAW);
	glVertexAttribPointer(displayTCLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (displayTCLoc);

	// start threads
	decodeQueue = new queue<frameCPU> (8, videoWidth, videoHeight, videoFourCC);
	decodeThread = std::thread (&Renderer::decode, this);

	uploadQueue = new queue<frameGPU> (8, videoWidth, videoHeight, videoFourCC);
	uploadThread = std::thread (&Renderer::upload, this);
/*
	renderQueue = new queue<frameGPUo> (8, videoWidth, videoHeight, videoFourCC);
	decodeThread = std::thread (&Renderer::render, this);

	backbufferQueue = new queue<frameGPUo> (8, videoWidth, videoHeight, videoFourCC);
	backbufferThread = std::thread (&Renderer::backbuffer, this);
*/
	// wait till there's at least 1 frame to show
	while (uploadQueue->isEmpty()) {
		usleep (10000);
	}

	// create display texture
	displayCurr = new frameGPU (videoWidth, videoHeight, pFormat::RGBA);

	factor = displayRefreshRate / videoFps;
	glUseProgram (displaySP);
	initialized = true;
	return true;
}

bool Renderer::checkExtensions () {
	std::string ext ((const char*) glGetString (GL_EXTENSIONS));
	std::vector<std::string> extList;

	int s = 0;
	int e;
	for (e = 0; ext[e] != 0; e++) {
		if (ext[e] == ' ') {
			extList.push_back (ext.substr (s, e - s));
			s = e + 1;
		}
	}
	extList.push_back (ext.substr (s, e - s));

	bool extTex10 = false;		// 10-bit textures
	bool extTex16 = false;		// 16-bit textures
	bool extCol10 = false;		// 10-bit color buffer
	bool extCol16 = false;		// 16-bit color buffer
	bool extFrag16 = false;		// 16-bit processing
	bool extWriteOnly = false;
	for (unsigned int i = 0; i < extList.size (); i++) {
		if (!extList.at(i).compare ("GL_OES_texture_half_float"))
			extTex10 = true;
		if (!extList.at(i).compare ("GL_OES_texture_float"))
			extTex16 = true;
		if (!extList.at(i).compare ("GL_EXT_color_buffer_half_float"))
			extCol10 = true;
		if (!extList.at(i).compare ("GL_EXT_color_buffer_float"))
			extCol16 = true;
		if (!extList.at(i).compare ("GL_OES_fragment_precision_high"))
			extFrag16 = true;
		if (!extList.at(i).compare ("GL_QCOM_writeonly_rendering"))
			extWriteOnly = true;
	}

	// 10 bit storage is target and not supported
	if (!storage16 && (!extTex10 || !extCol10))
		return false;

	// 16 bit storage is target and not supported
	if (storage16 && (!extTex16 || !extCol16))
		return false;

	// 16 bit processing is target and not supported
	if (process16 && !extFrag16)
		return false;

/*
	ALOG ("target texture: %i bit, target processing: %i bit", storage16 ? 16 : 10, process16 ? 16 : 10);
	ALOG ("10 bit texture: %s", extTex10 ? "supported" : "not supported");
	ALOG ("16 bit texture: %s", extTex16 ? "supported" : "not supported");
	ALOG ("16 bit processing: %s", extFrag16 ? "supported" : "not supported");
	ALOG ("write-only rendering: %s", extWriteOnly ? "supported" : "not supported");
*/
	return true;
}

GLuint Renderer::loadShader (GLenum type, const char *shaderSrc) {
	GLuint shader = glCreateShader (type);
	if (shader == 0) {
		ALOG ("Error creating shader");
		return 0;
	}

	glShaderSource (shader, 1, &shaderSrc, NULL);
	glCompileShader (shader);

	GLint compiled;
	glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetShaderInfoLog (shader, infoLen, NULL, infoLog);
			ALOG ("Error compiling shader:\n%s", infoLog);
			delete[] infoLog;
		}
		glDeleteShader (shader);
		return 0;
	}

	return shader;
}

GLuint Renderer::loadProgram (GLuint vertexShader, GLuint fragmentShader) {
	GLuint programObject = glCreateProgram();
	if	(programObject == 0) {
		ALOG ("Error creating program");
		return 0;
	}

	glAttachShader (programObject, vertexShader);
	glAttachShader (programObject, fragmentShader);
	glBindAttribLocation (programObject, displayVCLoc, "displayVC");
	glBindAttribLocation (programObject, displayTCLoc, "displayTC");
	glLinkProgram (programObject);

	GLint linked;
	glGetProgramiv (programObject, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv (programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetProgramInfoLog (programObject, infoLen, NULL, infoLog);
			ALOG ("Error linking program:\n%s", infoLog);
			delete[] infoLog;
		}
		return 0;
	}

	glDeleteShader (vertexShader);
	glDeleteShader (fragmentShader);

	return programObject;
}

void Renderer::setAspect () {
	int rectangle[4];
	glGetIntegerv (GL_VIEWPORT, rectangle);
	surfaceWidth = rectangle[2] - rectangle[0];
	surfaceHeight = rectangle[3] - rectangle[1];

	int mode = 1;
	switch (mode) {
		// stretch
		case 0:
			targetX = 0;
			targetY = 0;
			targetWidth = videoWidth;
			targetHeight = videoHeight;
			break;

		// touch from inside
		case 1:
			if ((long long) surfaceWidth * videoHeight * videoSarHeight < (long long) videoWidth * videoSarWidth * surfaceHeight) {
				targetX = 0;
				targetWidth = surfaceWidth;
				targetHeight = (long long) surfaceWidth * videoHeight * videoSarHeight / videoWidth / videoSarWidth;
				targetY = (surfaceHeight - targetHeight) / 2;
			} else {
				targetY = 0;
				targetHeight = surfaceHeight;
				targetWidth = (long long) surfaceHeight * videoWidth * videoSarWidth / videoHeight / videoSarHeight;
				targetX = (surfaceWidth - targetWidth) / 2;
			}
			break;

		// touch from outside
		case 2:
			if ((long long) surfaceWidth * videoHeight * videoSarHeight > (long long) videoWidth * videoSarWidth * surfaceHeight) {
				targetX = 0;
				targetWidth = surfaceWidth;
				targetHeight = (long long) surfaceWidth * videoHeight * videoSarHeight / videoWidth / videoSarWidth;
				targetY = (surfaceHeight - targetHeight) / 2;
			} else {
				targetY = 0;
				targetHeight = surfaceHeight;
				targetWidth = (long long) surfaceHeight * videoWidth * videoSarWidth / videoHeight / videoSarHeight;
				targetX = (surfaceWidth - targetWidth) / 2;
			}
			break;
	}

	glViewport (targetX, targetY, targetWidth, targetHeight);
}

void Renderer::genContexts () {
	mainContext = eglGetCurrentContext ();
	display = eglGetDisplay (EGL_DEFAULT_DISPLAY);

	EGLConfig config;
	EGLint numConfigs;
	EGLint attribListCfg[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE};
	EGLint attribListCtx[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	EGLint attribListSrf[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
	eglChooseConfig (display, attribListCfg, &config, 1, &numConfigs);
	uploadContext = eglCreateContext (display, config, mainContext, attribListCtx);
	uploadPBuffer = eglCreatePbufferSurface (display, config, attribListSrf);
	uploadPBuffer2 = eglCreatePbufferSurface (display, config, attribListSrf);

}

void Renderer::drawFrame () {
	glFinish();
	glClear (GL_COLOR_BUFFER_BIT);

	if (initialized) {
		if (playing) {
			int tc = tcNow ();
			while (repeat <= 0.0) {
				repeat += factor;
				getNextFrame (displayCurr);
			}

			if (displayCurr->timecode < tc) {
				if (displayCurr->timecode < tc - 2000 / videoFps) {
					// if very late - drop current frame
					ALOG ("hard drop (repeat: %f, factor: %f)", repeat, factor);
					repeat -= 1.0;
				} else if (newFrame && (repeat >= floor (factor))) {
					// if just a bit early - try to find best frame to drop (that is repeated more times than others)
					ALOG ("soft drop (repeat: %f, factor: %f)", repeat, factor);
					repeat -= 1.0;
				}
			} else if (displayCurr->timecode > tc + 1000 / videoFps) {
				if (displayCurr->timecode > tc + 3000 / videoFps) {
					// if very early - repeat current frame
					ALOG ("hard repeat (repeat: %f, factor: %f)", repeat, factor);
					repeat += 1.0;
				} else if (newFrame && (repeat <= floor (factor))) {
					// if just a but early - try to find frame best frame to drop (that is repeated less times than others)
					// < should be better (since equality means that frame would be repeated more times),
					// but then it will never fix offsync with int factors
					ALOG ("soft repeat (repeat: %f, factor: %f)", repeat, factor);
					repeat += 1.0;
				}
			}

			presentFrame (displayCurr);
	/*
			if ((frameNumber > 10) && (repeat <= 0.0)) {
				factor = (double) presentedFrames / frameNumber;
			}
	*/
			if (uploadQueue->isEmpty() && !uploading)
				playing = false;
		} else {
			glBindTexture (GL_TEXTURE_2D, displayCurr->plane);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		}
	}
}

void Renderer::presentFrame (frameGPU* f) {
	//ALOG ("f %i t %i n %i r %.3f f %.3f", frameNumber, f->timecode, tcNow (), repeat, factor);
	glBindTexture (GL_TEXTURE_2D, f->plane);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	presentedFrames++;
	repeat -= 1.0;
	newFrame = false;
}

void Renderer::getNextFrame (frameGPU* f) {
	if (!uploadQueue->isEmpty()) {
		uploadQueue->pop (*f);

		frameNumber++;
		newFrame = true;
	}
}

void Renderer::play (int timecode) {
	start = nanotime () - (int64_t) timecode * 1000000LL;
	playing = true;
}

void Renderer::pause () {
	start = 0;
	playing = false;
}

void Renderer::decode () {
	frameCPU t (videoWidth, videoHeight, videoFourCC);
	int size = videoWidth * videoHeight * 4;

	decoding = true;
	while (decoding) {
		if (!decodeQueue->isFull ()) {
			t.timecode = video->getNextVideoframe (t.plane, size);

			if (t.timecode != -1) {
				decodeQueue->push (t);
			} else {
				decoding = false;
			}
		} else {
			usleep (10000);
		}
	}
}

void Renderer::upload () {
	eglMakeCurrent (display, uploadPBuffer, uploadPBuffer2, uploadContext);

	frameCPU from (videoWidth, videoHeight, videoFourCC);
	frameGPU to (videoWidth, videoHeight, videoFourCC);

	int planes = chooseUPlanes (videoFourCC);
	int widthChroma = chooseUHalf (videoFourCC, false, true) ? videoWidth / 2 : videoWidth;
	int	heightChroma = chooseUHalf (videoFourCC, false, false) ? videoHeight / 2 : videoHeight;
	GLenum formatLuma = chooseUFormat (videoFourCC, true);
	GLenum formatChroma = chooseUFormat (videoFourCC, false);

	int offset1, offset2;
	switch (videoFourCC) {
		case pFormat::P008:
		case pFormat::P208:
		case pFormat::P408:
		case pFormat::NV12:
		case pFormat::NV21:
		case pFormat::RGBA:
			offset1 = videoWidth * videoHeight;
			offset2 = offset1 + widthChroma * heightChroma;
			break;

		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			offset1 = videoWidth * videoHeight * 2;
			offset2 = offset1 + widthChroma * heightChroma * 2;
			break;

		default:
			return;
	}

	uploading = true;
	while (uploading) {
		if (!decodeQueue->isEmpty () && !uploadQueue->isFull ()) {
			decodeQueue->pop (from);

			to.timecode = from.timecode;
			glBindTexture (GL_TEXTURE_2D, to.plane);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, videoWidth, videoHeight, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) from.plane);
/*
			glBindTexture (GL_TEXTURE_2D, to.plane[0]);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, videoWidth, videoHeight,
					formatLuma, GL_UNSIGNED_BYTE, (GLvoid*) from.plane);

			if (planes > 1) {
				glBindTexture (GL_TEXTURE_2D, to.plane[1]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, widthChroma, heightChroma,
						formatChroma, GL_UNSIGNED_BYTE, (GLvoid*) (from.plane + offset1));
			}

			if (planes > 2) {
				glBindTexture (GL_TEXTURE_2D, to.plane[2]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, widthChroma, heightChroma,
						formatChroma, GL_UNSIGNED_BYTE, (GLvoid*) (from.plane + offset2));
			}
*/
			glFlush ();

			uploadQueue->push (to);
		} else if (decodeQueue->isEmpty () && !decoding) {
			uploading = false;
		} else {
			usleep (10000);
		}
	}
}

void Renderer::render () {
	eglMakeCurrent (display, renderPBuffer, renderPBuffer2, renderContext);

	frameGPU from (videoWidth, videoHeight, videoFourCC);
	frameGPUo to (videoWidth, videoHeight, pFormat::RGBA);

	frameGPUi t (videoWidth, videoHeight, storage16 ? pFormat::FULL : pFormat::HALF);

	rendering = true;
	while (rendering) {
		if (!uploadQueue->isEmpty () && !renderQueue->isFull ()) {
			uploadQueue->pop (from);

			// from
			//  V
			// upscale chroma + merge into 1 plane
			//  V
			//  t
			//  V
			// scale, dither
			//  V
			// to

			renderQueue->push (to);
		} else if (uploadQueue->isEmpty () && !uploading) {
			rendering = false;
		} else {
			usleep (10000);
		}
	}
}

void Renderer::backbuffer () {
	eglMakeCurrent (display, backbufferPBuffer, backbufferPBuffer2, backbufferContext);

	frameGPUo t (videoWidth, videoHeight, videoFourCC);

	backbuffering = true;
	while (backbuffering) {
		if (!renderQueue->isEmpty () && !backbufferQueue->isFull ()) {
			renderQueue->pop (t);

			backbufferQueue->push (t);
		} else if (renderQueue->isEmpty () && !rendering) {
			backbuffering = false;
		} else {
			usleep (10000);
		}
	}
}
/*
	// create FBO
	GLuint framebuffer;
	GLuint depthRenderbuffer;
	GLuint texture;

	glGenFramebuffers(1, &framebuffer);
	glGenRenderbuffers(1, &depthRenderbuffer);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sWidth, sHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, sWidth, sHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
*/

/*
	// delete FBO
	glDeleteRenderbuffers(1, &depthRenderbuffer);
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &texture);
*/
