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
%s

uniform sampler2D video;
uniform mat3 conversion;
uniform vec3 offset;
in vec2 coord;
out vec4 outColor;

void main () {
	vec3 result = texture (video, coord).rgb * conversion + offset;

#ifdef SIGMOIDAL
	float s = tanh (2.0);
	result = 0.5 + 0.25 * atanh (-s + 2.0 * s * result);
#endif

	outColor = vec4 (result, 1.0);
})";
