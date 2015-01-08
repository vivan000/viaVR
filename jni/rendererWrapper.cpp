#include "rendererWrapper.h"
#include "videoRenderer.h"

videoRenderer* r;
videoDecoder video;

void on_surface_created () {
	int timecode = video.getCurrentTimecode();
	if (r) {
		delete r;
		r = NULL;
	}
	r = new videoRenderer;
	r->addVideoDecoder (&video);
	r->init ();
	r->play (timecode);
};

void on_surface_changed (int width, int height) {
}

void on_draw_frame () {
	if (r)
		r->drawFrame ();
}
