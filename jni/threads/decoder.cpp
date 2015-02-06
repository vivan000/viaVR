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

#include "threads/threads.h"

decoder::decoder (videoInfo* info, queue<frameCPU>* decodeQueue, IVideoDecoder* video) {
	size = info->width * info->height * info->Bpp / 8;
	decoder::decodeQueue = decodeQueue;
	decoder::video = video;

	to = new frameCPU (info);

	start ();
}

decoder::~decoder () {
	if (!joined)
		stop ();

	delete to;
}

void decoder::start () {
	thread = std::thread (&decoder::decode, this);
}

void decoder::stop () {
	working = false;
	thread.join ();
	joined = true;
}

void decoder::decode () {
	joined = false;
	working = true;
	while (working) {
		if (!decodeQueue->isFull ()) {
			to->timecode = video->getNextVideoframe (to->plane, size);

			switch (to->timecode) {
				case -1:
					// stop playback
					working = false;
					break;
				case -2:
					// try later
					usleep (10000);
					break;
				default:
					decodeQueue->push (to);
			}
		} else {
			usleep (10000);
		}
	}
}
