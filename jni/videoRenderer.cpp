#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include <vector>
#include "log.h"
#include "videoRenderer.h"
#include "shader.h"

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
		playing = false;

		decodeThread.join ();
		uploadThread.join ();
		renderThread.join ();

		delete decodeQueue;
		delete uploadQueue;
		delete renderQueue;

		delete displayCurr;
	}

	delete info;
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
	info = new videoInfo (video->getWidth (), video->getHeight (),
		video->getFourCC (), video->getRange (), video->getMatrix ());

	if (!info->init)
		return false;

	videoSarWidth =		video->getSarWidth ();
	videoSarHeight =	video->getSarHeight ();
	videoFps = 			round ((double) video->getFpsNumerator () / video->getFpsDenominator ());

	LOGD ("Video info:");
	LOGD ("Video %ix%i (%i:%i)", info->width, info->height, videoSarWidth, videoSarHeight);
	LOGD ("FourCC: %c%c%c%c", reinterpret_cast <char*> (&info->fourCC)[0], reinterpret_cast <char*> (&info->fourCC)[1],
		reinterpret_cast <char*> (&info->fourCC)[2], reinterpret_cast <char*> (&info->fourCC)[3]);
	LOGD ("Matrix: %s (%s)", info->matrix == pMatrix::BT709 ? "BT.709" : "BT.601", (int) info->matrix == video->getMatrix () ? "upstream" : "guess");
	LOGD ("Range: %s (%s)",	info->range == pRange::TV ? "TV" : "PC", (int) info->range == video->getRange () ? "upstream" : "guess");
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
	const char* displayVP =
		#include "shaders/displayVert.h"
	const char* displayFP =
		#include "shaders/displayFrag.h"

	shader displayShader (displayVP, displayFP);
	GLuint displaySP = displayShader.loadProgram ();
	if (!displaySP)
		return false;
	LOGD ("Shaders: OK");

	// load coordinates
	const float VertexPositions[] = {
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 0.0f, 1.0f};

	const float VertexTexcoord[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f};

	const float VertexFlippedTexcoord[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f};

	glGenBuffers (3, vboIds);

	const GLuint vertexCoordLoc = 0;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexPositions), VertexPositions, GL_STATIC_DRAW);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	const GLuint textureFlippedCoordLoc = 1;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[1]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexFlippedTexcoord), VertexFlippedTexcoord, GL_STATIC_DRAW);
	glVertexAttribPointer (textureFlippedCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureFlippedCoordLoc);

	glBindBuffer (GL_ARRAY_BUFFER, vboIds[2]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexTexcoord), VertexTexcoord, GL_STATIC_DRAW);

	// set texture unit
	GLint displayTLoc = glGetUniformLocation (displaySP, "video");
	glUseProgram (displaySP);
	glUniform1i (displayTLoc, 0);

	// start threads
	decodeQueue = new queue<frameCPU> (8, info);
	decodeThread = std::thread (&videoRenderer::decode, this);

	uploadQueue = new queue<frameGPUu> (8, info);
	uploadThread = std::thread (&videoRenderer::upload, this);

	renderQueue = new queue<frameGPUo> (8, info);
	renderThread = std::thread (&videoRenderer::render, this);

	// wait till there's at least 1 frame to show
	while (uploadQueue->isEmpty ()) {
		usleep (10000);
	}

	// create display texture
	displayCurr = new frameGPUo (info);

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

	bool extHalf = false;		// half-float color buffer
	bool extFull = false;		// float color buffer
	bool extWriteOnly = false;
	for (unsigned int i = 0; i < extList.size (); i++) {
		if (!extList.at (i).compare ("GL_EXT_color_buffer_half_float"))
			extHalf = true;
		if (!extList.at (i).compare ("GL_EXT_color_buffer_float"))
			extFull = true;
		if (!extList.at (i).compare ("GL_QCOM_writeonly_rendering"))
			extWriteOnly = true;
	}

	// half-float texture is target and not supported
	if ((precisionTex == 1) && !extHalf)
		return false;

	// float texture is target and not supported
	if ((precisionTex == 2) && !extFull)
		return false;

	LOGD ("Extensions:");
	LOGD ("Target texture: %i, target processing: %s", precisionTex, precisionHighp ? "highp" : "mediump");
	LOGD ("Half-float texture: %s", extHalf ? "supported" : "not supported");
	LOGD ("Float texture: %s", extFull ? "supported" : "not supported");
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
			info->targetX = 0;
			info->targetY = 0;
			info->targetWidth = info->width;
			info->targetHeight = info->height;
			break;

		// touch from inside
		case 1:
			if ((long long) surfaceWidth * info->height * videoSarHeight < (long long) info->width * videoSarWidth * surfaceHeight) {
				info->targetX = 0;
				info->targetWidth = surfaceWidth;
				info->targetHeight = (long long) surfaceWidth * info->height * videoSarHeight / info->width / videoSarWidth;
				info->targetY = (surfaceHeight - info->targetHeight) / 2;
			} else {
				info->targetY = 0;
				info->targetHeight = surfaceHeight;
				info->targetWidth = (long long) surfaceHeight * info->width * videoSarWidth / info->height / videoSarHeight;
				info->targetX = (surfaceWidth - info->targetWidth) / 2;
			}
			break;

		// touch from outside
		case 2:
			if ((long long) surfaceWidth * info->height * videoSarHeight > (long long) info->width * videoSarWidth * surfaceHeight) {
				info->targetX = 0;
				info->targetWidth = surfaceWidth;
				info->targetHeight = (long long) surfaceWidth * info->height * videoSarHeight / info->width / videoSarWidth;
				info->targetY = (surfaceHeight - info->targetHeight) / 2;
			} else {
				info->targetY = 0;
				info->targetHeight = surfaceHeight;
				info->targetWidth = (long long) surfaceHeight * info->width * videoSarWidth / info->height / videoSarHeight;
				info->targetX = (surfaceWidth - info->targetWidth) / 2;
			}
			break;
	}

	glViewport (info->targetX, info->targetY, info->targetWidth, info->targetHeight);
}

void videoRenderer::genContexts () {
	mainContext = eglGetCurrentContext ();
	display = eglGetDisplay (EGL_DEFAULT_DISPLAY);

	EGLConfig config;
	EGLint numConfigs;
	EGLint attribListCfg[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE};
	EGLint attribListCtx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
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
	// LOGD ("frame %i timecode %i now %i repeat %i", frameNumber, f->timecode, tcNow (), repeat);
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
	frameCPU t (info);
	int size = info->width * info->height * info->Bpp / 8;

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

	frameCPU from (info);
	frameGPUu to (info);

	uploading = true;
	while (uploading) {
		if (!decodeQueue->isEmpty () && !uploadQueue->isFull ()) {
			decodeQueue->pop (from);

			to.timecode = from.timecode;

			glBindTexture (GL_TEXTURE_2D, to.plane[0]);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->width, info->height,
				info->lumaFormat, info->lumaType, (GLvoid*) from.plane);

			if (info->planes > 1) {
				glBindTexture (GL_TEXTURE_2D, to.plane[1]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from.plane + info->offset1));
			}

			if (info->planes > 2) {
				glBindTexture (GL_TEXTURE_2D, to.plane[2]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from.plane + info->offset2));
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

	// create FBO
	GLuint framebuffer;
	glGenFramebuffers (1, &framebuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);
	glViewport (0, 0, info->width, info->height);

	// load coordinates
	const GLuint vertexCoordLoc = 0;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	const GLuint textureCoordLoc = 1;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[2]);
	glVertexAttribPointer (textureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureCoordLoc);

	// shaders
	const char* renderVP =
		#include "shaders/displayVert.h"

	frameGPUi** internal = new frameGPUi*[8];
	pFormat internalType = pFormat::INT10;
	const char* precision = "highp";
	int internalCount = 0;

	// convert to internal format
	GLuint renderToInternalSP = 0;
	switch (info->planes) {
		case 3: {
			const char* render08ToInternalFP =
				#include "shaders/planar08ToInternal.h"
			const char* render16ToInternalFP =
				#include "shaders/planar16ToInternal.h"

			shader renderToInternalShader (renderVP, info->lumaType == GL_UNSIGNED_BYTE ? render08ToInternalFP : render16ToInternalFP, "highp");
			renderToInternalSP = renderToInternalShader.loadProgram ();

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Y"),  0);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cb"), 1);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cr"), 2);
			break;
		}

		case 1: {
			const char* renderRgbToInteralFP =
				#include "shaders/displayFrag.h"

				shader renderToInternalShader (renderVP, renderRgbToInteralFP, "highp");
				renderToInternalSP = renderToInternalShader.loadProgram ();

				glUseProgram (renderToInternalSP);
				glUniform1i (glGetUniformLocation (renderToInternalSP, "video"),  0);

			break;
		}
	}
	internal[internalCount++] = new frameGPUi (info->width, info->height, internalType);

	// convert 4:2:0 -> 4:2:2
	GLuint render420to422SP = 0;
	if (info->halfHeight) {
		const char* render420to422FP =
			#include "shaders/up420to422.h"
		GLfloat pitch[4] = {(float) (2.0 / info->height), (float) (1.0 / (info->height)), (float) (-0.5 / (info->height)), (float) (0.5 * (info->height))};

		shader render420to422Shader (renderVP, render420to422FP, precision);
		render420to422SP = render420to422Shader.loadProgram ();

		glUseProgram (render420to422SP);
		glUniform1i  (glGetUniformLocation (render420to422SP, "video"), 0);
		glUniform4fv (glGetUniformLocation (render420to422SP, "pitch"), 1, pitch);

		internal[internalCount++] = new frameGPUi (info->width, info->height, internalType);
	}

	// convert 4:2:2 -> 4:4:4
	GLuint render422to444SP = 0;
	if (info->halfWidth) {
		const char* render422to444FP =
			#include "shaders/up422to444.h"

		shader render422to444Shader (renderVP, render422to444FP, precision);
		render422to444SP = render422to444Shader.loadProgram ();

		glUseProgram (render422to444SP);
		glUniform1i (glGetUniformLocation (render422to444SP, "video"), 0);
		glUniform1f (glGetUniformLocation (render422to444SP, "pitch"), (float) (1.0 / info->width));

		internal[internalCount++] = new frameGPUi (info->width, info->height, internalType);
	}

	// convert YCbCr -> RGB
	GLuint renderYuvToRgbSP = 0;
	if (info->fourCC != pFormat::RGBA) {
		const char* renderYuvToRgbFP =
			#include "shaders/yuvToRgb.h"

		shader renderYuvToRgbShader (renderVP, renderYuvToRgbFP, precision);
		renderYuvToRgbSP = renderYuvToRgbShader.loadProgram ();

		glUseProgram (renderYuvToRgbSP);
		glUniform1i			(glGetUniformLocation (renderYuvToRgbSP, "video"),		0);
		glUniformMatrix3fv	(glGetUniformLocation (renderYuvToRgbSP, "conversion"), 1, GL_FALSE, info->colorConversion);
		glUniform3fv		(glGetUniformLocation (renderYuvToRgbSP, "offset"),		1, info->colorOffset);

		internal[internalCount++] = new frameGPUi (info->width, info->height, internalType);
	}

	// scale height
	GLuint renderScaleHeightSP = 0;
	if (info->targetHeight != info->height) {
		if (info->targetHeight > info->height) {

		} else {
			const char* renderScaleHeightFP =
				#include "shaders/displayFrag.h"

			shader renderScaleHeightShader (renderVP, renderScaleHeightFP, precision);
			renderScaleHeightSP = renderScaleHeightShader.loadProgram ();

			glUseProgram (renderScaleHeightSP);
			glUniform1i (glGetUniformLocation (renderScaleHeightSP, "video"), 0);
		}

		internal[internalCount++] = new frameGPUi (info->width, info->targetHeight, internalType);
	}

	// scale width
	GLuint renderScaleWidthSP = 0;
	if (info->targetWidth != info->width) {
		if (info->targetWidth > info->width) {

		} else {
			const char* renderScaleWidthFP =
				#include "shaders/displayFrag.h"

			shader renderScaleWidthShader (renderVP, renderScaleWidthFP, precision);
			renderScaleWidthSP = renderScaleWidthShader.loadProgram ();

			glUseProgram (renderScaleWidthSP);
			glUniform1i (glGetUniformLocation (renderScaleWidthSP, "video"), 0);
		}

		internal[internalCount++] = new frameGPUi (info->targetWidth, info->targetHeight, internalType);
	}

	// dither
	GLuint renderDitherSP = 0; {
		const char* renderDitherFP =
			#include "shaders/displayFrag.h"

		shader renderDitherShader (renderVP, renderDitherFP, precision);
		renderDitherSP = renderDitherShader.loadProgram ();

		glUseProgram (renderDitherSP);
		glUniform1i (glGetUniformLocation (renderDitherSP, "video"), 0);
	}
	internal[internalCount++] = new frameGPUi (info->targetWidth, info->targetHeight, internalType);


	frameGPUu from (info);
	frameGPUo to (info);

	rendering = true;
	while (rendering) {
		if (!uploadQueue->isEmpty () && !renderQueue->isFull ()) {
			uploadQueue->pop (from);

			to.timecode = from.timecode;

			int internalCurrent = 0;

			for (int i = 0; i < info->planes; i++) {
				glActiveTexture (GL_TEXTURE0 + i);
				glBindTexture (GL_TEXTURE_2D, from.plane[i]);
			}

			glActiveTexture (GL_TEXTURE0);
			glViewport (0, 0, info->width, info->height);
			glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internal[internalCurrent]->plane, 0);
			glUseProgram (renderToInternalSP);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

			if (render420to422SP) {
				glBindTexture (GL_TEXTURE_2D, internal[internalCurrent]->plane);
				glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internal[++internalCurrent]->plane, 0);
				glUseProgram (render420to422SP);
				glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
			}

			if (render422to444SP) {
				glBindTexture (GL_TEXTURE_2D, internal[internalCurrent]->plane);
				glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internal[++internalCurrent]->plane, 0);
				glUseProgram (render422to444SP);
				glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
			}

			if (renderYuvToRgbSP) {
				glBindTexture (GL_TEXTURE_2D, internal[internalCurrent]->plane);
				glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internal[++internalCurrent]->plane, 0);
				glUseProgram (renderYuvToRgbSP);
				glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
			}

			if (renderScaleHeightSP) {
				glViewport (0, 0, info->width, info->targetHeight);
				glBindTexture (GL_TEXTURE_2D, internal[internalCurrent]->plane);
				glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internal[++internalCurrent]->plane, 0);
				glUseProgram (renderScaleHeightSP);
				glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
			}

			if (renderScaleWidthSP) {
				glViewport (0, 0, info->targetWidth, info->targetHeight);
				glBindTexture (GL_TEXTURE_2D, internal[internalCurrent]->plane);
				glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internal[++internalCurrent]->plane, 0);
				glUseProgram (renderScaleWidthSP);
				glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
			}

			glViewport (0, 0, info->targetWidth, info->targetHeight);
			glBindTexture (GL_TEXTURE_2D, internal[internalCurrent]->plane);
			glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to.plane, 0);
			glUseProgram (renderDitherSP);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

			glFlush ();

			renderQueue->push (to);
		} else if (uploadQueue->isEmpty () && !uploading) {
			rendering = false;
		} else {
			usleep (10000);
		}
	}

	for (int i = 0; i < internalCount; i++)
		delete internal[i];
	delete[] internal;

	glDeleteFramebuffers (1, &framebuffer);
}

void videoRenderer::getGlError () {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
		switch (err) {
			case GL_INVALID_ENUM:
				LOGE ("OpenGL error: GL_INVALID_ENUM");
				break;
			case GL_INVALID_VALUE:
				LOGE ("OpenGL error: GL_INVALID_VALUE");
				break;
			case GL_INVALID_OPERATION:
				LOGE ("OpenGL error: GL_INVALID_OPERATION");
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				LOGE ("OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION");
				break;
			case GL_OUT_OF_MEMORY:
				LOGE ("OpenGL error: GL_OUT_OF_MEMORY");
				break;
		};
}

void videoRenderer::getFbStatus () {
	GLenum err = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	switch (err) {
		case GL_FRAMEBUFFER_COMPLETE:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_COMPLETE");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_UNSUPPORTED");
			break;
		default:
			LOGE ("Framebuffer: %i", err);
	};
}
