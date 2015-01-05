#include <GLES2/gl2.h>

class shader {
public:
	shader (const char* vertexShaderSource, const char* fragmentShaderSource, ...);
	~shader ();

	void addAtrib (const char* attribName, const GLuint attribLoaction);

	GLuint loadProgram ();
private:
	GLuint loadShader (GLenum type, const char* shaderSource);

	GLuint vertexShader, fragmentShader;
	GLuint programObject;
};
