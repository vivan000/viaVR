## viaVR ##
Advanced video renderer for Android. [Overview](https://github.com/vivan000/viaVR/wiki/Why-high-quality-video-renderering-is-important).

### Features:
- large buffers to ensure smooth playback.
- smart presentation logic to minimize stutter, blending to minimize judder.
- input support for common video formats (4:2:0, 4:2:2, 4:4:4 at 8, 10 and 16 bit-depth).
- high bit-depth processing.
- proper color conversion (BT.709 support, correct chroma location).
- fast h/w bilinear scaling and chroma upsampling.
- better chroma upsampling (Catmull-Rom).
- better image upscaling (Lanczos 3/4 + antiringing).
- debanding.
- high quality dithering (dynamic ordered).
- native code (C++), full GPU acceleration (OpenGL ES 3.0).

### Planned features:
- subtitle consumer.
- OSD (stats).
- settings.
