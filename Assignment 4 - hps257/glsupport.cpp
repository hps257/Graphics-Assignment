#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>

#include "glsupport.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void ignoreGlErrors() {
  GLenum errCode;
  do {
     errCode = glGetError();
  } while (errCode != GL_NO_ERROR);
}

void checkGlErrors(const char* filename, int lineno) {
  const GLenum errCode = glGetError();

  if (errCode != GL_NO_ERROR) {
    string error("GL Error: ");
    error += reinterpret_cast<const char*>(gluErrorString(errCode));
    error += " ";
    error += filename;
    error += ":";
    error += std::to_string(lineno);
    cerr << error << endl;
    throw runtime_error(error);
  }
}

// Dump text file into a character vector, throws exception on error
static void readTextFile(const char *fn, vector<char>& data) {
  // Sets ios::binary bit to prevent end of line translation, so that the
  // number of bytes we read equals file size
  ifstream ifs(fn, ios::binary);
  if (!ifs)
    throw runtime_error(string("Cannot open file ") + fn);

  // Sets bits to report IO error using exception
  ifs.exceptions(ios::eofbit | ios::failbit | ios::badbit);
  ifs.seekg(0, ios::end);
  size_t len = (size_t)ifs.tellg();

  data.resize(len);

  ifs.seekg(0, ios::beg);
  ifs.read(&data[0], len);
}

// Print info regarding an GL object
static void printInfoLog(GLuint obj, const string& filename) {
  GLint infologLength = 0;
  if (infologLength > 0) {
    GLchar messages[1024];
    glGetShaderInfoLog(obj, sizeof(messages), 0, &messages[0]);
    std::cerr << "##### Log [" << filename << "]:\n" << messages << endl;
  }
}

void readAndCompileSingleShader(GLuint shaderHandle, const char *fn) {
  vector<char> source;
  readTextFile(fn, source);

  const char *ptrs[] = {&source[0]};
  const GLint lens[] = {(GLint)source.size()};
  glShaderSource(shaderHandle, 1, ptrs, lens);   // load the shader sources

  glCompileShader(shaderHandle);

  printInfoLog(shaderHandle, fn);

  GLint compiled = 0;
  glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compiled);
  checkGlErrors(__FILE__, __LINE__);
  if (!compiled) {
    GLint maxLength = 0;
    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shaderHandle, maxLength, &maxLength, &errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    cerr << &errorLog[0];

    // Exit with failure.
    glDeleteShader(shaderHandle); // Don't leak the shader.
    throw runtime_error("fails to compile GL shader");
  }
  checkGlErrors(__FILE__, __LINE__);
}

void linkShader(GLuint programHandle, GLuint vs, GLuint fs) {
  glAttachShader(programHandle, vs);
  glAttachShader(programHandle, fs);

  glLinkProgram(programHandle);

  glDetachShader(programHandle, vs);
  glDetachShader(programHandle, fs);

  GLint linked = 0;
  glGetProgramiv(programHandle, GL_LINK_STATUS, &linked);
  printInfoLog(programHandle, "linking");

  if (!linked)
    throw runtime_error("fails to link shaders");
}


void readAndCompileShader(GLuint programHandle, const char * vertexShaderFileName, const char * fragmentShaderFileName) {
  GlShader vs(GL_VERTEX_SHADER);
  GlShader fs(GL_FRAGMENT_SHADER);

  readAndCompileSingleShader(vs, vertexShaderFileName);
  checkGlErrors(__FILE__, __LINE__);

  readAndCompileSingleShader(fs, fragmentShaderFileName);
  checkGlErrors(__FILE__, __LINE__);

  linkShader(programHandle, vs, fs);
  checkGlErrors(__FILE__, __LINE__);
}

GLuint loadGLTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    
    if(image == nullptr) {
        std::cout << "Unable to load image. Make sure the image is in the same path as the executable.\n";
        assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(image);
    return retTexture;
}