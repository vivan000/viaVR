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
				// scaling
				if (!strcmp (opt, "hwChroma"))
					hwChroma = !strcmp (val, "true");
				if (!strcmp (opt, "hwScale"))
					hwScale = !strcmp (val, "true");
				if (!strcmp (opt, "hwChromaLinear"))
					hwChromaLinear = !strcmp (val, "true");
				if (!strcmp (opt, "hwScaleLinear"))
					hwScaleLinear = !strcmp (val, "true");

				// processing
				if (!strcmp (opt, "internalType")) {
					int type;
					sscanf (val, "%i", &type);
					switch (type) {
						case 0:
							internalType = iFormat::INT8;
							break;
						case 1:
							internalType = iFormat::INT10;
							break;
						case 2:
							internalType = iFormat::FLOAT16;
							break;
						case 3:
							internalType = iFormat::FLOAT32;
							break;
					}
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

				// logging
				if (!strcmp (opt, "logEachFrame"))
					logEachFrame = !strcmp (val, "true");
			}
		}
	}
	f.close ();
}

config::~config () {
}
