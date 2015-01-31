#include "rendererWrapper.h"
#include "videoRenderer.h"
#include "patternGenerator.h"

videoRenderer* r;
patternGenerator video;

void on_surface_created () {
	if (r) {
		delete r;
		r = NULL;
	}
	r = new videoRenderer;
	r->addVideoDecoder (&video);
	video.seek (0);
	r->init ();
	r->play (0);
};

void on_surface_changed (int width, int height) {
}

void on_draw_frame () {
	if (r)
		r->drawFrame ();
}
