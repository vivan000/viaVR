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

#pragma once
#include "frames/frames.h"

struct config {
	int targetX, targetY;
	int targetWidth, targetHeight;

	bool hwChroma = true;
	bool hwScale = true;
	bool hwChromaLinear = true;
	bool hwScaleLinear = true;

	iFormat internalType = iFormat::INT10;
	bool highp = true;
	int targetBitdepth = 8;

	bool debanding = false;
	double debandThreshold = 2.0;

	int displayMode = 1;
	int displayRefreshRate = 60;

	bool logEachFrame = false;
	bool measurePerformance = false;
};
