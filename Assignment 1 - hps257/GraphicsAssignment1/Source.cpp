#include "glsupport.h"
#include <stdio.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifdef __APPLE__
# include <GLUT/freeglut.h>
#else
# include <GL/freeglut.h>
#endif

void display(void);
void init(void);
void reshape(int w, int h);
void idle(void);

GLint positionAttribute;
GLint texCoordAttribute;
GLint programID;
GLint positionUniform;
GLuint testTexture;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 500);
	glutCreateWindow("Graphics - cs6533");

	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		printf("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);

	init();
	glutMainLoop();
	return 0;
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(programID);
	glBindTexture(GL_TEXTURE_2D, testTexture);

	float vertices[] = {
		-0.5, -0.2,
		 0.5, -0.2,
		 0.5,  0.5,
		-0.5, -0.2,
	  	 0.5,  0.5,
		-0.5,  0.5
	};

	glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(positionAttribute);

	float texCoords[] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f };

	glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(texCoordAttribute);

	positionUniform = glGetUniformLocation(programID, "location");
	glUniform2f(positionUniform, -0.5, 0.0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUniform2f(positionUniform, 0.5, 0.0);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(positionAttribute);
	glDisableVertexAttribArray(texCoordAttribute);

	glutSwapBuffers();
}

void init()
{
	glClearColor(0.2, 0.2, 0.2, 0.0);
	int w, h, comp;
	unsigned char* image = stbi_load("emoji.png", &w, &h, &comp, STBI_rgb_alpha);

	if (image == nullptr) {
		std::cout << "Unable to load image. Make sure the image is in the same path as the executable.\n";
		assert(false);
	}

	glGenTextures(1, &testTexture);

	glBindTexture(GL_TEXTURE_2D, testTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (comp == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	else if (comp == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image);

	programID = glCreateProgram();
	readAndCompileShader(programID, "vertex_textured.glsl", "fragment_textured.glsl");

	positionAttribute = glGetAttribLocation(programID, "position");
	texCoordAttribute = glGetAttribLocation(programID, "texCoord");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

void idle(void)
{
	glutPostRedisplay();
}
