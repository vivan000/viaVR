## viaVR ##
Advanced video renderer for Android.

### Goals for initial release:
- large buffers to ensure smooth playback.
- smart presentation logic to minimize stutter.
- input support for common video formats (4:2:0, 4:2:2, 4:4:4 at 8, 10 and 16 bits).
- 10/16-bit processing.
- proper color conversion (BT.709 support, correct chroma location, etc.).
- native code (C++).
- full GPU acceleration (OpenGL ES 3.0).

### 1.0 goals:
- high quality scaling.
- high quality dithering.
- subtitle consumer.
- support for custom shaders.
- OSD (stats).
- settings.

### System requirements:
- OpenGL ES 3.0.
- a lot of free RAM.
