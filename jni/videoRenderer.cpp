#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include <vector>
#include "log.h"
#include "videoRenderer.h"
#include "shader.h"

const float videoRenderer::VertexPositions[] = {
	-1.0f,  1.0f, 0.0f, 1.0f,
	 1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f};

const float videoRenderer::VertexTexcoord[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f};

const char* videoRenderer::displayVP =
	#include "shaders/displayVert.h"

const char* videoRenderer::displayFP =
	#include "shaders/displayFrag.h"

const char* videoRenderer::up420to422FP =
	#include "shaders/up420to422.h"

const char* videoRenderer::up422to444FP =
	#include "shaders/up422to444.h"

videoRenderer::videoRenderer () {
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glClear (GL_COLOR_BUFFER_BIT);
	initialized = false;

	presentedFrames = 0;
	repeat = 0;
	frameNumber = 0;
}

videoRenderer::~videoRenderer () {
	if (initialized) {
		decoding = false;
		uploading = false;
		rendering = false;
		//backbuffering = false;
		playing = false;

		decodeThread.join ();
		uploadThread.join ();
		renderThread.join ();
		//backbufferThread.join ();

		delete decodeQueue;
		delete uploadQueue;
		delete renderQueue;
		//delete backbufferQueue;

		delete displayCurr;
	}
}

const char* getName () {
	return "viaVR";
}

int getMajorVersion () {
	return 0;
}

int getMinorVersion () {
	return 0;
}

int64_t videoRenderer::nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

int videoRenderer::tcNow () {
	return (int) ((nanotime () - start) / 1000000);
}

bool videoRenderer::addVideoDecoder (videoDecoder* video) {
	videoRenderer::video = video;

	videoWidth = 		video->getWidth ();
	videoHeight = 		video->getHeight ();
	videoSarWidth =		video->getSarWidth ();
	videoSarHeight =	video->getSarHeight ();
	videoFps = 			round ((double) video->getFpsNumerator () / video->getFpsDenominator ());

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

	LOGD ("Video info:");
	LOGD ("Video %ix%i (%i:%i)", videoWidth, videoHeight, videoSarWidth, videoSarHeight);
	LOGD ("FourCC: %c%c%c%c", reinterpret_cast <char*> (&videoFourCC)[0], reinterpret_cast <char*> (&videoFourCC)[1],
		reinterpret_cast <char*> (&videoFourCC)[2], reinterpret_cast <char*> (&videoFourCC)[3]);
	LOGD ("Matrix: %s (%s)", videoMatrix == pMatrix::BT709 ? "BT.709" : "BT.601", (int) videoMatrix == video->getMatrix () ? "upstream" : "guess");
	LOGD ("Range: %s (%s)",	videoRange == pRange::TV ? "TV" : "PC",	(int) videoRange == video->getRange () ? "upstream" : "guess");
	LOGD ("Framerate: %i/%i", video->getFpsNumerator (), video->getFpsDenominator ());

	return true;
}

void videoRenderer::setSize (int width, int height) {
}

void videoRenderer::setRefreshRate (int fps) {
	displayRefreshRate = fps;
}

bool videoRenderer::init () {
	genContexts ();
	setAspect ();

	// check extenions
	if (!checkExtensions ())
		return false;
	LOGD ("Extensions: OK");

	// load shaders
	shader displayShader (displayVP, displayFP);
	const GLuint vertexCoordLoc = 0;
	const GLuint textureCoordLoc = 1;
	displayShader.addAtrib ("vertexCoord", vertexCoordLoc);
	displayShader.addAtrib ("textureCoord", textureCoordLoc);
	GLuint displaySP = displayShader.loadProgram ();
	if (!displaySP)
		return false;
	LOGD ("Shaders: OK");

	// load coordinates
	glGenBuffers (2, vboIds);
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexPositions), VertexPositions, GL_STATIC_DRAW);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	glBindBuffer (GL_ARRAY_BUFFER, vboIds[1]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexTexcoord), VertexTexcoord, GL_STATIC_DRAW);
	glVertexAttribPointer (textureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureCoordLoc);

	// set texture unit
	GLint displayTLoc = glGetUniformLocation (displaySP, "texture");
	glUseProgram (displaySP);
	glUniform1i (displayTLoc, 0);

	// start threads
	decodeQueue = new queue<frameCPU> (8, videoWidth, videoHeight, videoFourCC);
	decodeThread = std::thread (&videoRenderer::decode, this);

	uploadQueue = new queue<frameGPUu> (8, videoWidth, videoHeight, videoFourCC);
	uploadThread = std::thread (&videoRenderer::upload, this);

	renderQueue = new queue<frameGPUo> (8, videoWidth, videoHeight, videoFourCC);
	renderThread = std::thread (&videoRenderer::render, this);
/*
	backbufferQueue = new queue<frameGPUo> (8, videoWidth, videoHeight, videoFourCC);
	backbufferThread = std::thread (&videoRenderer::backbuffer, this);
*/
	// wait till there's at least 1 frame to show
	while (uploadQueue->isEmpty ()) {
		usleep (10000);
	}

	// create display texture
	displayCurr = new frameGPUo (videoWidth, videoHeight, pFormat::RGBA);

	hardLate = -2000 / videoFps;
	softLate = 0;
	softEarly = 2000 / videoFps;
	hardEarly = 4000 / videoFps;
	repeatLim = displayRefreshRate / videoFps * videoFps;

	initialized = true;
	LOGD ("Initialization: OK");
	return true;
}

bool videoRenderer::checkExtensions () {
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
		if (!extList.at (i).compare ("GL_OES_texture_half_float"))
			extTex10 = true;
		if (!extList.at (i).compare ("GL_OES_texture_float"))
			extTex16 = true;
		if (!extList.at (i).compare ("GL_EXT_color_buffer_half_float"))
			extCol10 = true;
		if (!extList.at (i).compare ("GL_EXT_color_buffer_float"))
			extCol16 = true;
		if (!extList.at (i).compare ("GL_OES_fragment_precision_high"))
			extFrag16 = true;
		if (!extList.at (i).compare ("GL_QCOM_writeonly_rendering"))
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

	LOGD ("Extensions:");
	LOGD ("Target texture: %i bit, target processing: %i bit", storage16 ? 16 : 10, process16 ? 16 : 10);
	LOGD ("10 bit texture: %s", extTex10 ? "supported" : "not supported");
	LOGD ("16 bit texture: %s", extTex16 ? "supported" : "not supported");
	LOGD ("16 bit processing: %s", extFrag16 ? "supported" : "not supported");
	LOGD ("Write-only rendering: %s", extWriteOnly ? "supported" : "not supported");
	return true;
}

void videoRenderer::setAspect () {
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

void videoRenderer::genContexts () {
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

	renderContext = eglCreateContext (display, config, mainContext, attribListCtx);
	renderPBuffer = eglCreatePbufferSurface (display, config, attribListSrf);
	renderPBuffer2 = eglCreatePbufferSurface (display, config, attribListSrf);
}

void videoRenderer::drawFrame () {
	glFinish ();
	glClear (GL_COLOR_BUFFER_BIT);

	if (initialized) {
		if (playing) {
			int tc = tcNow ();
			while (repeat <= 0) {
				getNextFrame (displayCurr);
			}

			if (displayCurr->timecode < tc + softLate) {
				if (displayCurr->timecode < tc + hardLate) {
					// if very late - drop current frame
					LOGD ("hard drop (repeat: %i)", repeat);
					repeat -= videoFps;
				} else if (newFrame && (repeat >= repeatLim)) {
					// if just a bit late - try to find best frame to drop (that is repeated more times than others)
					LOGD ("soft drop (repeat: %i)", repeat);
					repeat -= videoFps;
				}
			} else if (displayCurr->timecode > tc + softEarly) {
				if (displayCurr->timecode > tc + hardEarly) {
					// if very early - repeat current frame
					LOGD ("hard repeat (repeat: %i)", repeat);
					repeat += videoFps;
				} else if (newFrame && (repeat <= repeatLim)) {
					// if just a bit early - try to find frame best frame to repeat (that is repeated less times than others)
					LOGD ("soft repeat (repeat: %i)", repeat);
					repeat += videoFps;
				}
			}

			presentFrame (displayCurr);
			if (uploadQueue->isEmpty () && !uploading)
				playing = false;
		} else {
			glBindTexture (GL_TEXTURE_2D, displayCurr->plane);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		}
	}
}

void videoRenderer::presentFrame (frameGPUo* f) {
	LOGD ("frame %i timecode %i now %i repeat %i", frameNumber, f->timecode, tcNow (), repeat);
	glBindTexture (GL_TEXTURE_2D, f->plane);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	presentedFrames++;
	newFrame = false;

	repeat -= videoFps;
}

void videoRenderer::getNextFrame (frameGPUo* f) {
	if (!renderQueue->isEmpty ()) {
		renderQueue->pop (*f);

		frameNumber++;
		newFrame = true;
	}

	repeat += displayRefreshRate;
}

void videoRenderer::play (int timecode) {
	start = nanotime () - (int64_t) timecode * 1000000LL;
	playing = true;
}

void videoRenderer::pause () {
	start = 0;
	playing = false;
}

void videoRenderer::decode () {
	frameCPU t (videoWidth, videoHeight, videoFourCC);
	int size = videoWidth * videoHeight * chooseBpp (videoFourCC) / 8;

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

void videoRenderer::upload () {
	eglMakeCurrent (display, uploadPBuffer, uploadPBuffer2, uploadContext);

	frameCPU from (videoWidth, videoHeight, videoFourCC);
	frameGPUu to (videoWidth, videoHeight, videoFourCC);

	int planes = chooseUPlanes (videoFourCC);
	int widthChroma = chooseUHalf (videoFourCC, false, true) ? videoWidth / 2 : videoWidth;
	int	heightChroma = chooseUHalf (videoFourCC, false, false) ? videoHeight / 2 : videoHeight;
	GLenum formatLuma = chooseUFormat (videoFourCC, true);
	GLenum formatChroma = chooseUFormat (videoFourCC, false);
	int offset1 = videoWidth * videoHeight * (chooseDoubleOffset (videoFourCC) ? 2 : 1);
	int offset2 = offset1 + widthChroma * heightChroma * (chooseDoubleOffset (videoFourCC) ? 2 : 1);

	uploading = true;
	while (uploading) {
		if (!decodeQueue->isEmpty () && !uploadQueue->isFull ()) {
			decodeQueue->pop (from);

			to.timecode = from.timecode;

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

			glFlush ();

			uploadQueue->push (to);
		} else if (decodeQueue->isEmpty () && !decoding) {
			uploading = false;
		} else {
			usleep (10000);
		}
	}
}

void videoRenderer::render () {
	eglMakeCurrent (display, renderPBuffer, renderPBuffer2, renderContext);

	frameGPUu from (videoWidth, videoHeight, videoFourCC);
	frameGPUo to (videoWidth, videoHeight, pFormat::RGBA);

	const GLuint vertexCoordLoc = 0;
	const GLuint textureCoordLoc = 1;

//	frameGPUi t (videoWidth, videoHeight, storage16 ? pFormat::FULL : pFormat::HALF);

	// create FBO
	GLuint framebuffer;
	glGenFramebuffers (1, &framebuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);
	glViewport (0, 0, videoWidth, videoHeight);

	// load shaders
	const char* renderFP =
		#include "shaders/upload3ToInternal.h"
	shader renderShader (displayVP, renderFP, "highp");
	renderShader.addAtrib ("vertexCoord", vertexCoordLoc);
	renderShader.addAtrib ("textureCoord", textureCoordLoc);
	GLuint renderSP = renderShader.loadProgram ();

	// load coordinates
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	glBindBuffer (GL_ARRAY_BUFFER, vboIds[1]);
	glVertexAttribPointer (textureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureCoordLoc);

	GLint renderYLoc = glGetUniformLocation (renderSP, "Y");
	GLint renderCbLoc = glGetUniformLocation (renderSP, "Cb");
	GLint renderCrLoc = glGetUniformLocation (renderSP, "Cr");

	glUseProgram (renderSP);

	glUniform1i (renderYLoc, 0);
	glUniform1i (renderCbLoc, 1);
	glUniform1i (renderCrLoc, 2);

	rendering = true;
	while (rendering) {
		if (!uploadQueue->isEmpty () && !renderQueue->isFull ()) {
			uploadQueue->pop (from);

			to.timecode = from.timecode;

			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, from.plane[0]);

			glActiveTexture (GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, from.plane[1]);

			glActiveTexture (GL_TEXTURE2);
			glBindTexture (GL_TEXTURE_2D, from.plane[2]);

			glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to.plane, 0);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

			glFlush ();

			renderQueue->push (to);
		} else if (uploadQueue->isEmpty () && !uploading) {
			rendering = false;
		} else {
			usleep (10000);
		}
	}

	glDeleteFramebuffers (1, &framebuffer);
}

void videoRenderer::backbuffer () {
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
