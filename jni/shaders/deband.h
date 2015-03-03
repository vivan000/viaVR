/*
 * Copyright (c) 2015 Ivan Valiulin
 *
 * Based on port/modification by Mathias Rauen (madhsi)
 * http://forum.doom9.org/showpost.php?p=1652132&postcount=296
 * of flash3kyuu_deband algorithm by Joe Hu (SAPikachu)
 * https://github.com/SAPikachu/flash3kyuu_deband
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
uniform sampler2D dither;
uniform vec3 thresh;
uniform vec2 pitch;
uniform vec2 resize;
in vec2 coord;
out vec4 outColor;

void main () {
	vec2 random = texture (dither, coord * resize).rg * 14.0 + 1.0;
	vec2 x = random * pitch.x;
	vec2 y = random * pitch.y;
	vec3 pix0 = texture (video, coord).rgb;
	vec3 pix1 = texture (video, vec2 (coord.x - x.x, coord.y - y.y)).rgb;
	vec3 pix2 = texture (video, vec2 (coord.x - x.y, coord.y + y.x)).rgb;
	vec3 pix3 = texture (video, vec2 (coord.x + x.x, coord.y + y.y)).rgb;
	vec3 pix4 = texture (video, vec2 (coord.x + x.y, coord.y - y.x)).rgb;

	vec3 avg = (pix1 + pix3) * 0.25 + (pix2 + pix4) * 0.25;
	vec3 avgDif = abs (avg - pix0);
	vec3 maxDif = max (
		max (abs (pix1 - pix0), abs (pix2 - pix0)),
		max (abs (pix3 - pix0), abs (pix4 - pix0)));
	vec3 midDif1 = abs (pix1 + pix3 - 2.0 * pix0);
	vec3 midDif2 = abs (pix2 + pix4 - 2.0 * pix0);
	vec3 factor = pow (
		clamp (avgDif  * thresh.x + 3.0, 0.0, 1.0) *
		clamp (maxDif  * thresh.y + 3.0, 0.0, 1.0) *
		clamp (midDif1 * thresh.z + 3.0, 0.0, 1.0) *
		clamp (midDif2 * thresh.z + 3.0, 0.0, 1.0),
		vec3 (0.1));
	outColor = vec4 (mix (pix0, avg, factor), 1.0);
})";
