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

R"(#version 300 es
precision %s float;
#define taps %i

uniform sampler2D video;
uniform vec3 pitch;
uniform float weight[taps];
uniform float offset[taps];
in vec2 coord;
out vec4 outColor;

void main () {
	vec3 sum = vec3 (0.0);

	for (int i = 0; i < taps; i++) {
		sum += texture (video, vec2 (coord.x - offset[i], coord.y)).rgb * weight[i];
		sum += texture (video, vec2 (coord.x + offset[i], coord.y)).rgb * weight[i];
	}

	outColor = vec4 (sum, 1.0);
})";
