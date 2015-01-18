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

#include "frames.h"

frameCPU::frameCPU (videoInfo* f) {
	plane = new char[f->width * f->height * f->Bpp / 8];

	timecode = 0;
}

frameCPU::~frameCPU () {
	delete[] plane;
}

void frameCPU::swap (frameCPU& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}
