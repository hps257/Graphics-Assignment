#include "glsupport.h"
#include "matrix4.h"
#include "quat.h"
#include "geometrymaker.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


GLuint program, screenTrianglesProgram, screenTrianglesProgram2;

GLuint frameBuffer, frameBuffer2;
GLuint frameBufferTexture, frameBufferTexture2;
GLuint depthBufferTexture, depthBufferTexture2;

GLuint timeUniform;

GLuint normalviewMatrixUniformLocation, modelviewMatrixUniformLocation, projectionMatrixUniformLocation;

GLuint vertexBO, indexBO;
GLuint positionAttribute, normalAttribute, texCoordAttribute, binormalAttribute, tangentAttribute;

GLuint diffuseTexture, specularTexture, normalTexture;
GLuint diffuseTextureUniformLocation, specularUniformLocation, normalUniformLocation;

GLuint lightDirectionUniformLocation0, lightDirectionUniformLocation1, lightDirectionUniformLocation2;
GLuint lightColorUniform0, lightColorUniform1, lightColorUniform2;
GLuint specularLightColorUniform0, specularLightColorUniform1, specularLightColorUniform2;
GLuint frameBufferUniformLocation, frameBufferUniformLocation2;

GLuint screenTrianglesPositionBuffer, screenTrianglesPositionAttribute, screenTrianglesUVBuffer, screenTrianglesTexCoordAttribute;
GLuint screenTrianglesPositionBuffer2, screenTrianglesPositionAttribute2, screenTrianglesUVBuffer2, screenTrianglesTexCoordAttribute2;

int numIndices;

struct VertexPNTBTG {
	Cvec3f p, n, b, tg;
	Cvec2f t;
	VertexPNTBTG() {}
	VertexPNTBTG(float x, float y, float z, float nx, float ny, float nz) : p(x, y, z), n(nx, ny, nz) {}
	VertexPNTBTG& operator = (const GenericVertex& v) {
		p = v.pos;
		n = v.normal;
		t = v.tex;
		b = v.binormal;
		tg = v.tangent;
		return *this;
	}
};

struct Transform {
	Quat rotation;
	Cvec3 scale;
	Cvec3 position;
	Transform() : scale(1.0f, 1.0f, 1.0f) {
	}
	Matrix4 createMatrix(Cvec3 translate) {
		Matrix4 matrix;
		Matrix4 positionMatrix = positionMatrix.makeTranslation(translate);
		int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
		rotation = Quat::makeYRotation(timeSinceStart / 100.0f);
		Matrix4 rotationMatrix = quatToMatrix(rotation);
		Matrix4 scaleMatrix = scaleMatrix.makeScale(scale);
		matrix = positionMatrix * rotationMatrix * scaleMatrix;
		return matrix;
	};
};

struct Entity {
	Transform transform;
	void Draw(Matrix4 &eyeInverse, GLuint positionAttribute, GLuint normalAttribute, GLuint texCoordAttribute, GLuint binormalAttribute, GLuint tangentAttribute, GLuint modelviewMatrixUniformLocation, GLuint normalMatrixUniformLocation, Cvec3 translate) {
		glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBO);


		glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNTBTG), (GLvoid*)offsetof(VertexPNTBTG, p));
		glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNTBTG), (GLvoid*)offsetof(VertexPNTBTG, n));
		glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPNTBTG), (GLvoid*)offsetof(VertexPNTBTG, t));
		glVertexAttribPointer(binormalAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNTBTG), (void*)offsetof(VertexPNTBTG, b));
		glVertexAttribPointer(tangentAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPNTBTG), (void*)offsetof(VertexPNTBTG, tg));

		glEnableVertexAttribArray(positionAttribute);
		glEnableVertexAttribArray(normalAttribute);
		glEnableVertexAttribArray(texCoordAttribute);
		glEnableVertexAttribArray(binormalAttribute);
		glEnableVertexAttribArray(tangentAttribute);

		Matrix4 modelViewMatrix = eyeInverse *  transform.createMatrix(translate);

		GLfloat glModelViewMatrix[16];
		modelViewMatrix.writeToColumnMajorMatrix(glModelViewMatrix);
		glUniformMatrix4fv(modelviewMatrixUniformLocation, 1, false, glModelViewMatrix);

		Matrix4 normalMatrix = transpose(inv(modelViewMatrix));

		GLfloat glNormalMatrix[16];
		normalMatrix.writeToColumnMajorMatrix(glNormalMatrix);
		glUniformMatrix4fv(normalMatrixUniformLocation, 1, false, glNormalMatrix);

		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(positionAttribute);
		glDisableVertexAttribArray(normalAttribute);
		glDisableVertexAttribArray(texCoordAttribute);
		glDisableVertexAttribArray(binormalAttribute);
		glDisableVertexAttribArray(tangentAttribute);
	}
};

void calculateFaceTangent(const Cvec3f &v1, const Cvec3f &v2, const Cvec3f &v3, const Cvec2f &texCoord1, const Cvec2f &texCoord2,
	const Cvec2f &texCoord3, Cvec3f &tangent, Cvec3f &binormal) {
	Cvec3f side0 = v1 - v2;
	Cvec3f side1 = v3 - v1;
	Cvec3f normal = cross(side1, side0);
	normalize(normal);
	float deltaV0 = texCoord1[1] - texCoord2[1];
	float deltaV1 = texCoord3[1] - texCoord1[1];
	tangent = side0 * deltaV1 - side1 * deltaV0;
	normalize(tangent);
	float deltaU0 = texCoord1[0] - texCoord2[0];
	float deltaU1 = texCoord3[0] - texCoord1[0];
	binormal = side0 * deltaU1 - side1 * deltaU0;
	normalize(binormal);
	Cvec3f tangentCross = cross(tangent, binormal);
	if (dot(tangentCross, normal) < 0.0f) {
		tangent = tangent * -1;
	}
}

void loadObjFile(const std::string &fileName, std::vector<VertexPNTBTG> &outVertices, std::vector<unsigned short> &outIndices) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str(), NULL, true);
	if (ret) {
		for (int i = 0; i < shapes.size(); i++) {
			for (int j = 0; j < shapes[i].mesh.indices.size(); j++) {
				unsigned int vertexOffset = shapes[i].mesh.indices[j].vertex_index * 3;
				unsigned int normalOffset = shapes[i].mesh.indices[j].normal_index * 3;
				unsigned int texOffset = shapes[i].mesh.indices[j].texcoord_index * 2;
				VertexPNTBTG v;
				v.p[0] = attrib.vertices[vertexOffset];
				v.p[1] = attrib.vertices[vertexOffset + 1];
				v.p[2] = attrib.vertices[vertexOffset + 2];
				v.n[0] = attrib.normals[normalOffset];
				v.n[1] = attrib.normals[normalOffset + 1];
				v.n[2] = attrib.normals[normalOffset + 2];
				v.t[0] = attrib.texcoords[texOffset];
				v.t[1] = 1.0 - attrib.texcoords[texOffset + 1];
				outVertices.push_back(v);
				outIndices.push_back(outVertices.size() - 1);
			}
		}
		for (int i = 0; i < outVertices.size(); i += 3) {
			Cvec3f tangent;
			Cvec3f binormal;
			calculateFaceTangent(outVertices[i].p, outVertices[i + 1].p, outVertices[i + 2].p,
				outVertices[i].t, outVertices[i + 1].t, outVertices[i + 2].t, tangent, binormal);
			outVertices[i].tg = tangent;
			outVertices[i + 1].tg = tangent;
			outVertices[i + 2].tg = tangent;
			outVertices[i].b = binormal;
			outVertices[i + 1].b = binormal;
			outVertices[i + 2].b = binormal;
		}
	}
	else {
		std::cout << err << std::endl;
		assert(false);
	}
}

void display(void) {
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glViewport(0, 0, 800, 800);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	

	glUniform3f(lightDirectionUniformLocation0, -20.0, 20.0, 0.0);
	glUniform3f(lightColorUniform0, 0.1, 0.1, 0.0);
	glUniform3f(specularLightColorUniform0, 1.0, 1.0, 1.0);

	glUniform3f(lightDirectionUniformLocation1, 0.0, 10.0, -20.0);
	glUniform3f(lightColorUniform1, 0, 0, 0.1);
	glUniform3f(specularLightColorUniform1, 1.0, 1.0, 1.0);

	glUniform3f(lightDirectionUniformLocation2, 0.0, 20.0, 0.0);
	glUniform3f(lightColorUniform2, 0, 0, 0);
	glUniform3f(specularLightColorUniform2, 1.0, 1.0, 0.0);

	glUniform1i(diffuseTextureUniformLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseTexture);

	glUniform1i(specularUniformLocation, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularTexture);

	glUniform1i(normalUniformLocation, 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normalTexture);

	Matrix4 eyeMatrix;
	eyeMatrix = eyeMatrix.makeTranslation(Cvec3(0.0, 5.0, 35.0));
	Matrix4 eyeInverse = inv(eyeMatrix);

	Matrix4 projectionMatrix;
	projectionMatrix = projectionMatrix.makeProjection(45, 1.0, -0.1, -150);
	GLfloat glmatrixProjection[16];
	projectionMatrix.writeToColumnMajorMatrix(glmatrixProjection);
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, false, glmatrixProjection);

	Entity entity;

	entity.Draw(eyeInverse, positionAttribute, normalAttribute, texCoordAttribute, binormalAttribute, tangentAttribute, modelviewMatrixUniformLocation, normalviewMatrixUniformLocation, Cvec3(-10, 0, 0));
	entity.Draw(eyeInverse, positionAttribute, normalAttribute, texCoordAttribute, binormalAttribute, tangentAttribute, modelviewMatrixUniformLocation, normalviewMatrixUniformLocation, Cvec3(0, 0, 0));
	entity.Draw(eyeInverse, positionAttribute, normalAttribute, texCoordAttribute, binormalAttribute, tangentAttribute, modelviewMatrixUniformLocation, normalviewMatrixUniformLocation, Cvec3(10, 0, 0));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, 800, 800);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer2);
	glViewport(0, 0, 800, 800);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(screenTrianglesProgram2);

	glUniform1i(frameBufferUniformLocation2, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frameBufferTexture);

	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesPositionBuffer2);
	glVertexAttribPointer(screenTrianglesPositionAttribute2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(screenTrianglesPositionAttribute2);

	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesUVBuffer2);
	glVertexAttribPointer(screenTrianglesTexCoordAttribute2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(screenTrianglesTexCoordAttribute2);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(screenTrianglesPositionAttribute2);
	glDisableVertexAttribArray(screenTrianglesTexCoordAttribute2);

	// 
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, 800, 800);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(screenTrianglesProgram);

	glUniform1i(frameBufferUniformLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frameBufferTexture2);

	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesPositionBuffer);
	glVertexAttribPointer(screenTrianglesPositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(screenTrianglesPositionAttribute);

	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesUVBuffer);
	glVertexAttribPointer(screenTrianglesTexCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(screenTrianglesTexCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(screenTrianglesPositionAttribute);
	glDisableVertexAttribArray(screenTrianglesTexCoordAttribute);

	//2nd set


	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glClearColor(0, 0, 0, 1);
	glutSwapBuffers();
}

void init() {
	GLint GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult)
	{
		printf("ERROR: %s\n", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glReadBuffer(GL_BACK);
	//glEnable(GL_MULTISAMPLE);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK); 
	glClearDepth(0.0f);

	
	
	program = glCreateProgram();
	readAndCompileShader(program, "vertex.glsl", "fragment.glsl");
//	glUseProgram(program);

	screenTrianglesProgram = glCreateProgram();
	readAndCompileShader(screenTrianglesProgram, "vertex_texture.glsl", "fragment_texture.glsl");


	screenTrianglesProgram2 = glCreateProgram();
	readAndCompileShader(screenTrianglesProgram2, "vertex_texture_bw.glsl", "fragment_texture_bw.glsl");

	positionAttribute = glGetAttribLocation(program, "position");
	normalAttribute = glGetAttribLocation(program, "normal");
	texCoordAttribute = glGetAttribLocation(program, "texCoord");
	binormalAttribute = glGetAttribLocation(program, "binormal");
	tangentAttribute = glGetAttribLocation(program, "tangent");

	modelviewMatrixUniformLocation = glGetUniformLocation(program, "modelViewMatrix");
	normalviewMatrixUniformLocation = glGetUniformLocation(program, "normalMatrix");
	projectionMatrixUniformLocation = glGetUniformLocation(program, "projectionMatrix");

	lightDirectionUniformLocation0 = glGetUniformLocation(program, "lights[0].lightDirection");
	lightDirectionUniformLocation1 = glGetUniformLocation(program, "lights[1].lightDirection");
	lightDirectionUniformLocation2 = glGetUniformLocation(program, "lights[2].lightDirection");

	lightColorUniform0 = glGetUniformLocation(program, "lights[0].lightColor");
	lightColorUniform1 = glGetUniformLocation(program, "lights[1].lightColor");
	lightColorUniform2 = glGetUniformLocation(program, "lights[2].lightColor");

	specularLightColorUniform0 = glGetUniformLocation(program, "lights[0].specularLightColor");
	specularLightColorUniform1 = glGetUniformLocation(program, "lights[1].specularLightColor");
	specularLightColorUniform2 = glGetUniformLocation(program, "lights[2].specularLightColor");

	std::vector<VertexPNTBTG> meshVertices;
	std::vector<unsigned short> meshIndices;
	loadObjFile("Monk_Giveaway_Fixed.obj", meshVertices, meshIndices);
	numIndices = meshIndices.size();
	diffuseTexture = loadGLTexture("Monk_D.tga");
	specularTexture = loadGLTexture("Monk_Sc.tga");
	normalTexture = loadGLTexture("Monk_N_Normal_Bump.tga");
	diffuseTextureUniformLocation = glGetUniformLocation(program, "diffuseTexture");
	specularUniformLocation = glGetUniformLocation(program, "specularTexture");
	normalUniformLocation = glGetUniformLocation(program, "normalTexture");

	glGenBuffers(1, &vertexBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPNTBTG) * meshVertices.size(), meshVertices.data(), GL_STATIC_DRAW);
	glGenBuffers(1, &indexBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * meshIndices.size(), meshIndices.data(), GL_STATIC_DRAW);

	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	
	glGenTextures(1, &frameBufferTexture);
	glBindTexture(GL_TEXTURE_2D, frameBufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB,
		GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, frameBufferTexture, 0);

	glGenTextures(1, &depthBufferTexture);
	glBindTexture(GL_TEXTURE_2D, depthBufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 800, 800, 0, GL_DEPTH_COMPONENT,
		GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 800, 800);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
		depthBufferTexture, 0);

	//2nd frame buffer
	glGenFramebuffers(1, &frameBuffer2);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer2);

	glGenTextures(1, &frameBufferTexture2);
	glBindTexture(GL_TEXTURE_2D, frameBufferTexture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB,
		GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, frameBufferTexture2, 0);

	glGenTextures(1, &depthBufferTexture2);
	glBindTexture(GL_TEXTURE_2D, depthBufferTexture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 800, 800, 0, GL_DEPTH_COMPONENT,
		GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 800, 800);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
		depthBufferTexture2, 0);


	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glUseProgram(screenTrianglesProgram);

	screenTrianglesPositionAttribute = glGetAttribLocation(screenTrianglesProgram, "position");
	screenTrianglesTexCoordAttribute = glGetAttribLocation(screenTrianglesProgram, "texCoord");

	frameBufferUniformLocation = glGetUniformLocation(screenTrianglesProgram, "screenFrameBuffer");

	glGenBuffers(1, &screenTrianglesUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesUVBuffer);
	GLfloat screenTriangleUVs[] = {
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), screenTriangleUVs, GL_STATIC_DRAW);



	glGenBuffers(1, &screenTrianglesPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesPositionBuffer);
	GLfloat screenTrianglePositions[] = {
		1.0f, 1.0f,
		1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), screenTrianglePositions, GL_STATIC_DRAW);

	//2nd set of triangles
	
	screenTrianglesPositionAttribute2 = glGetAttribLocation(screenTrianglesProgram2, "position");
	screenTrianglesTexCoordAttribute2 = glGetAttribLocation(screenTrianglesProgram2, "texCoord");

	frameBufferUniformLocation2 = glGetUniformLocation(screenTrianglesProgram2, "texture");

	glGenBuffers(1, &screenTrianglesUVBuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesUVBuffer2);
	GLfloat screenTriangleUV2s[] = {
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), screenTriangleUV2s, GL_STATIC_DRAW);

	glGenBuffers(1, &screenTrianglesPositionBuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, screenTrianglesPositionBuffer2);
	GLfloat screenTrianglePositions2[] = {
		1.0f, 1.0f,
		1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), screenTrianglePositions2, GL_STATIC_DRAW);


}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
}

void idle(void) {
	glutPostRedisplay();
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
	glutInitWindowSize(800, 800);
	glutCreateWindow("CS-6533");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);

	init();
	glutMainLoop();
	return 0;
}