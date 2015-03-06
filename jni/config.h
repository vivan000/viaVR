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
#include "videoInfo.h"
#include "threads/helpers/scalers.h"
#define BENCHMARK 0

class config {
public:
	config ();
	~config ();

	int targetX, targetY;
	int targetWidth, targetHeight;

	int decodeQueueSize = 16;
	int renderQueueSize = 16;

	bool hwChroma = true;
	bool hwScale = true;
	bool hwChromaLinear = true;
	bool hwScaleLinear = true;

	kernel scaleKernel = kernel::Lanczos;
	int scaleTaps = 3;
	bool antiring = true;

	iFormat internalType = iFormat::INT10;
	bool highp = true;
	int targetBitdepth = 8;

	bool debanding = false;
	double debandAvgDif = 3.4;
	double debandMaxDif = 6.8;
	double debandMidDif = 3.3;

	int displayMode = 1;
	int displayRefreshRate = 60;

	bool logEachFrame = false;
};
