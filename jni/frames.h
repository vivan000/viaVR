/*
 * Copyright (c) 2015 Ivan Valiulin
 *
 * This file is part of viaVR.
 *
 * viaVR is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * viaVR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with viaVR. If not, see http://www.gnu.org/licenses
 */

#include "videoInfo.h"
#include <algorithm>

// frame for upload - CPU
class frameCPU {
public:
	char* plane;
	int timecode;

	frameCPU (videoInfo* f);
	~frameCPU ();
	void swap (frameCPU& that);
};

// frame for upload, 1-3 planes, 1-4 channels, byte
class frameGPUu {
public:
	GLuint* plane;
	int timecode;

	frameGPUu (videoInfo* f);
	~frameGPUu ();
	void swap (frameGPUu& that);

private:
	int numberOfPlanes;
};

// internal frame, RGB (half-)float
class frameGPUi {
public:
	GLuint plane;
	int timecode;
	const int width, height;

	frameGPUi (int w, int h, pFormat type, videoInfo* f);
	~frameGPUi ();
};

// output frame, RGB byte
class frameGPUo {
public:
	GLuint plane;
	int timecode;

	frameGPUo (videoInfo* f);
	~frameGPUo ();
	void swap (frameGPUo& that);
};
