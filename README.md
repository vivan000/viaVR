## viaVR ##
Advanced video renderer for Android. [Overview](https://github.com/vivan000/viaVR/wiki/Why-high-quality-video-renderering-is-important).

### Goals for initial release:
- large buffers to ensure smooth playback.
- smart presentation logic to minimize stutter.
- input support for common video formats (4:2:0, 4:2:2, 4:4:4 at 8, 10 and 16 bits).
- high bit-depth processing.
- proper color conversion (BT.709 support, correct chroma location, etc.).
- bilinear scaling and chroma upsampling.
- high quality dithering (dynamic ordered).
- native code (C++).
- full GPU acceleration (OpenGL ES 3.0).

### 1.0 goals:
- high quality chroma upsampling.
- high quality scaling.
- subtitle consumer.
- support for custom shaders.
- OSD (stats).
- settings.
