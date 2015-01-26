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

uniform sampler2D video;
uniform sampler2D blur;
uniform vec3 threshold;
in vec2 coord;
out vec4 outColor;

void main () {
	vec3 tex = texture (video, coord).rgb;
	vec3 avg = texture (blur, coord).rgb;
	bool diff = all (lessThan (abs (tex - avg), threshold));
	outColor = vec4 (diff ? avg : tex, 1.0);
	// vec3 diff = max (abs (tex - avg) * threshold, vec3 (1.0));
	// outColor = vec4 (mix (tex, avg, diff), 1.0);
})";
