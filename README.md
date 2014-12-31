## viaVR ##
Advanced video renderer for Android.

### Goals for initial release:
- large buffers to ensure smooth playback.
- input support for common video formats (4:2:0, 4:2:2, 4:4:4 at 8, 10 and 16 bits).
- 10/16-bit processing.
- proper color conversion (BT.709 support, correct chroma location, etc.).
- native code (C++).
- full GPU acceleration (OpenGL ES 2.0).

### 1.0 goals:
- high quality scaling.
- high quality dithering.
- subtitle consumer.
- support for custom shaders.
- OSD (stats).
- settings.

### System requirements:
- OpenGL ES 2.0.
- **GL_OES_texture_half_float** and **GL_EXT_color_buffer_half_float** for 10-bit precision.
- **GL_OES_texture_float**, **GL_EXT_color_buffer_float** and **GL_OES_fragment_precision_high** for 16-bit precision.
- (or OpenGL ES 3.0 for both).
- a lot of free RAM.
