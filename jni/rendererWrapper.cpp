#include "rendererWrapper.h"
#include "renderer.h"

Renderer* r;
videoDecoder video;

void on_surface_created () {
	int timecode = video.getCurrentTimecode();
	if (r) {
		delete r;
		r = NULL;
	}
	r = new Renderer;
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
