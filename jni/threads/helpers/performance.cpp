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

#include "threads/helpers/performance.h"
#define GL_TIME_ELAPSED_EXT 0x88BF

performance::performance (int size, int selection) : size (size), selection (selection) {
	queries = new GLuint[size];
	glGenQueries (size, queries);
}

performance::~performance () {
	glDeleteQueries (size, queries);
	delete[] queries;
}

void performance::begin () {
	glBeginQuery (GL_TIME_ELAPSED_EXT, queries[current++]);
}

void performance::end () {
	glEndQuery (GL_TIME_ELAPSED_EXT);
}

float performance::measure () {
	if (current == size) {
		ready = true;
		current = 0;
	}

	if (!ready)
		return 0.0;

	GLuint timeElapsed = 0;
	glGetQueryObjectuiv (queries[current], GL_QUERY_RESULT, &timeElapsed);

	if (!timeElapsed)
		return 0.0;

	counter++;
	total += timeElapsed / 1000000.0;

	if (counter != selection)
		return 0.0;

	float avg = total / counter;
	total = 0.0;
	counter = 0;
	return avg;
}
