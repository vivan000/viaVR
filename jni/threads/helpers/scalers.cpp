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

#include <math.h>
#include "threads/helpers/scalers.h"

double lanczos (double x, int radius) {
	if (x == 0.0)
		return 1.0;
	if (x <= -radius || x >= radius)
		return 0.0;

	double pi_x = M_PI * x;
	return radius * sin (pi_x) * sin (pi_x / radius) / pi_x / pi_x;
}

scalers::scalers (kernel k, int taps, int src, int dst) {
	switch (k) {
		case kernel::Lanczos:
			weights = new float[8 * dst];
			for (int i = 0; i < dst; i++) {
				double x = (i + 0.5) * src / dst + 0.5;
				x = floor (x) - x;
				double sum = 0.0;
				for (int j = -3; j <= 4; j++)
					sum += lanczos (x + j, taps);
				for (int j = 0; j < 4; j++) {
					weights[i * 4 + j] = (float) (lanczos (x - j, taps) / sum);
					weights[dst * 4 + i * 4 + j] = (float) (lanczos (x + j + 1, taps) / sum);
				}
			}

			break;
	}
}

scalers::~scalers () {
	delete[] weights;
}

float* scalers::getWeights () {
	return weights;
}
