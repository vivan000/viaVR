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

#include "config.h"
#include "log.h"
#include <fstream>
#include <istream>

config::config () {
	std::ifstream f ("/storage/emulated/0/video/config.txt");
	char str[64], opt[64], val[64];
	if (f.good ()) {
		LOGD ("Config override:");
		while (!f.eof ()) {
			f.getline (str, 64);
			if (sscanf (str, "%s = %s", opt, val)) {
				LOGD ("%s -> %s", opt, val);

				// queues
				if (!strcmp (opt, "decodeQueueSize"))
					sscanf (val, "%i", &decodeQueueSize);
				if (!strcmp (opt, "renderQueueSize"))
					sscanf (val, "%i", &renderQueueSize);

				// scaling
				if (!strcmp (opt, "hwChroma"))
					hwChroma = !strcmp (val, "true");
				if (!strcmp (opt, "hwScale"))
					hwScale = !strcmp (val, "true");
				if (!strcmp (opt, "hwChromaLinear"))
					hwChromaLinear = !strcmp (val, "true");
				if (!strcmp (opt, "hwScaleLinear"))
					hwScaleLinear = !strcmp (val, "true");

				if (!strcmp (opt, "scaleKernel"))
					scaleKernel = kernel::Lanczos;
				if (!strcmp (opt, "scaleTaps"))
					sscanf (val, "%i", &scaleTaps);
				if (!strcmp (opt, "antiring"))
					antiring = !strcmp (val, "true");

				// processing
				if (!strcmp (opt, "internalType")) {
					if (!strcmp (val, "INT8"))
						internalType = iFormat::INT8;
					if (!strcmp (val, "INT10"))
						internalType = iFormat::INT10;
					if (!strcmp (val, "FLOAT16"))
						internalType = iFormat::FLOAT16;
					if (!strcmp (val, "FLOAT32"))
						internalType = iFormat::FLOAT32;
				}
				if (!strcmp (opt, "highp"))
					highp = !strcmp (val, "true");
				if (!strcmp (opt, "targetBitdepth"))
					sscanf (val, "%i", &targetBitdepth);

				// debanding
				if (!strcmp (opt, "debanding"))
					debanding = !strcmp (val, "true");
				if (!strcmp (opt, "debandAvgDif"))
					sscanf (val, "%lf", &debandAvgDif);
				if (!strcmp (opt, "debandMaxDif"))
					sscanf (val, "%lf", &debandMaxDif);
				if (!strcmp (opt, "debandAvgDif"))
					sscanf (val, "%lf", &debandMidDif);

				// display
				if (!strcmp (opt, "displayMode"))
					sscanf (val, "%i", &displayMode);
				if (!strcmp (opt, "displayRefreshRate"))
					sscanf (val, "%i", &displayRefreshRate);
				if (!strcmp (opt, "blending"))
					blending = !strcmp (val, "true");

				// logging
				if (!strcmp (opt, "logEachFrame"))
					logEachFrame = !strcmp (val, "true");

				// override input tags
				if (!strcmp (opt, "overrideRange")) {
					if (!strcmp (val, "UNKNOWN"))
						overrideRange = pRange::UNKNOWN;
					if (!strcmp (val, "TV"))
						overrideRange = pRange::TV;
					if (!strcmp (val, "PC"))
						overrideRange = pRange::PC;
				}
				if (!strcmp (opt, "overrideMatrix")) {
					if (!strcmp (val, "UNKNOWN"))
						overrideMatrix = pMatrix::UNKNOWN;
					if (!strcmp (val, "BT601"))
						overrideMatrix = pMatrix::BT601;
					if (!strcmp (val, "BT709"))
						overrideMatrix = pMatrix::BT709;
				}
			}
		}
	}
	f.close ();
}

config::~config () {
}
