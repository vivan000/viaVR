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
precision highp float;
#define BLEND %i

uniform sampler2D video0;
uniform sampler2D dither;
uniform vec2 ditherDepth;
uniform vec2 ditherResize;
uniform vec3 ditherOffset;
#if BLEND
uniform sampler2D video1;
uniform float blendingFactor;
#endif
in vec2 coord;
out vec4 outColor;

void main () {
#if BLEND
	vec3 frame0 = texture (video0, coord).rgb;
	vec3 frame1 = texture (video1, coord).rgb;
	vec3 result = mix (frame1, frame0, blendingFactor);
#else
	vec3 result = texture (video0, coord).rgb;
#endif

	vec3 pattern = texture (dither, coord * ditherResize + ditherOffset.xy).rgb - 0.5;
	pattern = (ditherOffset.z == 0.0) ?
		pattern.rrr : ((ditherOffset.z == 1.0) ?
			pattern.ggg : pattern.bbb);
	pattern.g = -pattern.g;
	result = round (result * ditherDepth.x + pattern) * ditherDepth.y;

	outColor = vec4 (result, 1.0);
})";
