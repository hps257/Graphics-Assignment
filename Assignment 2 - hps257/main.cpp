#include "glsupport.h"
#include <GL/freeglut.h>
#include "matrix4.h"

void display(void);
void init(void);
void reshape(int w, int h);
void idle(void);

GLuint program;
GLuint vertPositionVBO;
GLuint positionAttribute;
GLuint colorUniform;
GLuint normalAttribute;
GLuint vertColorVBO;
GLuint cubeNormBO;
GLuint modelviewMatrixUniformLocation;
GLuint projectionMatrixUniformLocation;
GLuint normalMatrixUniformLocation;

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutCreateWindow("CS-6533");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
	return 0;
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vertPositionVBO);
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	glBindBuffer(GL_ARRAY_BUFFER, cubeNormBO);
	glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normalAttribute);

	glUniform3f(colorUniform, 0.0f, 0.0f, 1.0f);
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);

	Matrix4 objectMatrix;
	objectMatrix = objectMatrix.makeYRotation((timeSinceStart / 30.0));

	Matrix4 eyeMatrix;
	eyeMatrix = eyeMatrix.makeTranslation(Cvec3(0.0, 0.0, 40.0));

	Matrix4 modelViewMatrix = inv(eyeMatrix) * objectMatrix;

	GLfloat glmatrix[16];
	modelViewMatrix.writeToColumnMajorMatrix(glmatrix);
	glUniformMatrix4fv(modelviewMatrixUniformLocation, 1, false, glmatrix);

	Matrix4 projectionMatrix;
	projectionMatrix = projectionMatrix.makeProjection(45.0, 1.0, -0.1, -200.0);

	GLfloat glmatrixProjection[16];
	projectionMatrix.writeToColumnMajorMatrix(glmatrixProjection);
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, false, glmatrixProjection);

	Matrix4 invm = inv(modelViewMatrix);
	invm(0, 3) = invm(1, 3) = invm(2, 3) = 0;
	Matrix4 normalMatrix = transpose(invm);

	GLfloat glmatrixNormal[16];
	normalMatrix.writeToColumnMajorMatrix(glmatrixNormal);
	glUniformMatrix4fv(normalMatrixUniformLocation, 1, false, glmatrixNormal);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUniform3f(colorUniform, 0.5f, 0.0f, 0.5f);

	Matrix4 objectMatrix2;
	objectMatrix2 = objectMatrix2.makeTranslation(Cvec3(10.0, 0.0, 0.0));

	modelViewMatrix = inv(eyeMatrix) * objectMatrix * objectMatrix2;
	modelViewMatrix.writeToColumnMajorMatrix(glmatrix);
	glUniformMatrix4fv(modelviewMatrixUniformLocation, 1, false, glmatrix);

	projectionMatrix = projectionMatrix.makeProjection(45.0, 1.0, -0.1, -200.0);
	projectionMatrix.writeToColumnMajorMatrix(glmatrixProjection);
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, false, glmatrixProjection);

	invm = inv(modelViewMatrix);
	invm(0, 3) = invm(1, 3) = invm(2, 3) = 0;
	normalMatrix = transpose(invm);
	normalMatrix.writeToColumnMajorMatrix(glmatrixNormal);
	glUniformMatrix4fv(normalMatrixUniformLocation, 1, false, glmatrixNormal);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUniform3f(colorUniform, 0.27f, 0.73f, 0.0f);

	Matrix4 objectMatrix3;
	objectMatrix3 = objectMatrix3.makeTranslation(Cvec3(0.0, 10.0, 0.0));

	modelViewMatrix = inv(eyeMatrix) * objectMatrix * objectMatrix2* objectMatrix3;
	modelViewMatrix.writeToColumnMajorMatrix(glmatrix);
	glUniformMatrix4fv(modelviewMatrixUniformLocation, 1, false, glmatrix);

	projectionMatrix = projectionMatrix.makeProjection(45.0, 1.0, -0.1, -200.0);
	projectionMatrix.writeToColumnMajorMatrix(glmatrixProjection);
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, false, glmatrixProjection);

	invm = inv(modelViewMatrix);
	invm(0, 3) = invm(1, 3) = invm(2, 3) = 0;
	normalMatrix = transpose(invm);
	normalMatrix.writeToColumnMajorMatrix(glmatrixNormal);
	glUniformMatrix4fv(normalMatrixUniformLocation, 1, false, glmatrixNormal);

	glDrawArrays(GL_TRIANGLES, 0, 36); 

	glDisableVertexAttribArray(positionAttribute);
	glutSwapBuffers();
}

void init() {
	
	GLint GlewInitResult = glewInit();
	glClearDepth(0.0f);
	if (GLEW_OK != GlewInitResult)
	{
		printf("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}
	glClearDepth(0.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glReadBuffer(GL_BACK);

	program = glCreateProgram();
	readAndCompileShader(program, "vertex.glsl", "fragment.glsl");
	glUseProgram(program);

	modelviewMatrixUniformLocation = glGetUniformLocation(program, "modelViewMatrix");
	projectionMatrixUniformLocation = glGetUniformLocation(program, "projectionMatrix");
	normalMatrixUniformLocation = glGetUniformLocation(program, "normalMatrix");
	colorUniform = glGetUniformLocation(program, "uColor");

	positionAttribute = glGetAttribLocation(program, "position");
	normalAttribute = glGetAttribLocation(program, "normal");

	glGenBuffers(1, &vertPositionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertPositionVBO);
	GLfloat cubeVerts[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 108 * sizeof(GLfloat), cubeVerts, GL_STATIC_DRAW);

	glGenBuffers(1, &cubeNormBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeNormBO);
	GLfloat cubeNormals[] = {
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		-1.0f, 0.0f,0.0f,
		-1.0f, 0.0f,0.0f,
		-1.0f, 0.0f,0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f,0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f,0.0f, 1.0f,
		0.0f,0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f,0.0f,0.0f,
		1.0f, 0.0f,0.0f,
		1.0f,0.0f,0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f,0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f,0.0f,
		0.0f, 1.0f,0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f,0.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 108 * sizeof(GLfloat), cubeNormals, GL_STATIC_DRAW);

}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
}

void idle(void) {
	glutPostRedisplay();
}
