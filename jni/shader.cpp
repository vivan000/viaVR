#include <cstdio>
#include <string.h>
#include "log.h"
#include "shader.h"

shader::shader (const char* vertexShaderSource, const char* fragmentShaderSource, ...) {
	int size = strlen (fragmentShaderSource) + 32;
	char* fragmentShaderSourceProcessed = new char[size];

	va_list argptr;
	va_start (argptr, fragmentShaderSource);
	vsprintf (fragmentShaderSourceProcessed, fragmentShaderSource, argptr);
	va_end (argptr);

	vertexShader = loadShader (GL_VERTEX_SHADER, vertexShaderSource);
	fragmentShader = loadShader (GL_FRAGMENT_SHADER, fragmentShaderSourceProcessed);
	programObject = glCreateProgram ();

	if (programObject == 0) {
		LOGE ("Error creating program");
	}

	if (programObject && vertexShader && fragmentShader) {
		glAttachShader (programObject, vertexShader);
		glAttachShader (programObject, fragmentShader);
	}

	delete[] fragmentShaderSourceProcessed;
}

shader::~shader () {
}

void shader::addAtrib (const char* attribName, const GLuint attribLoaction) {
	if (programObject && vertexShader && fragmentShader) {
		glBindAttribLocation (programObject, attribLoaction, attribName);
	}
}

GLuint shader::loadProgram () {
	if (programObject && vertexShader && fragmentShader) {
		glLinkProgram (programObject);

		GLint linked;
		glGetProgramiv (programObject, GL_LINK_STATUS, &linked);
		if (!linked) {
			GLint infoLen = 0;
			glGetProgramiv (programObject, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen > 1) {
				char* infoLog = new char[infoLen];
				glGetProgramInfoLog (programObject, infoLen, NULL, infoLog);
				LOGE ("Error linking program:\n%s", infoLog);
				delete[] infoLog;
			}
			return 0;
		}

		glDeleteShader (vertexShader);
		glDeleteShader (fragmentShader);
		return programObject;
	}
	return 0;
}

GLuint shader::loadShader (GLenum type, const char* shaderSource) {
	GLuint shader = glCreateShader (type);
	if (shader == 0) {
		LOGE ("Error creating shader");
		return 0;
	}

	glShaderSource (shader, 1, &shaderSource, NULL);
	glCompileShader (shader);

	GLint compiled;
	glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetShaderInfoLog (shader, infoLen, NULL, infoLog);
			LOGE ("Error compiling shader:\n%s", infoLog);
			delete[] infoLog;
		}
		glDeleteShader (shader);
		return 0;
	}

	return shader;
}
