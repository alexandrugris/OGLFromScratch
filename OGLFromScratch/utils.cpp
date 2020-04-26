#include "pch.h"
#include "utils.h"

using namespace std;
using namespace glm;

Texture::Texture(const char* path, GLuint texChannel) : texGL(0), bLoaded(false), texChan(texChannel) {

    ILuint texIL = 0;

    // change origin
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    // generate image
    ilGenImages(1, &texIL);
    ilBindImage(texIL);

    // load
    if (ilLoadImage(path)) {
        cout << "Loaded " << path << endl;
        cout << "   Image width: " << ilGetInteger(IL_IMAGE_WIDTH) << endl;
        cout << "   Image height: " << ilGetInteger(IL_IMAGE_HEIGHT) << endl;
    }
    else {
        cout << ilGetError() << endl;
        return; 
    }

   
    // ensure the type is as set in OGL
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    //
    glGenTextures(1, &texGL); /* Texture name generation */

    glBindTexture(GL_TEXTURE_2D, texGL);
    glTexImage2D(GL_TEXTURE_2D, 
        0, 
        GL_RGBA,
        ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT),
        0, 
        GL_RGBA, 
        GL_UNSIGNED_BYTE,
        ilGetData());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    ilBindImage(0);
    ilDeleteImage(texIL);

    bLoaded = true;
}

Texture::~Texture() {
    Unapply();
    glDeleteTextures(1, &texGL);
}

void Texture::Apply() {
    glActiveTexture(texChan);
    glBindTexture(GL_TEXTURE_2D, texGL);
}

void Texture::Unapply() {
    glActiveTexture(texChan);
    glBindTexture(GL_TEXTURE_2D, 0);
}

ShaderProgram::ShaderProgram(const char* vtxShaderFilename, const char* fragmentShaderFilename) : bLoaded(false){

    // these should be kept in a shader library, not compiled every time
    GLuint vtxShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    if (LoadShaderFromFile(vtxShaderFilename, vtxShader) &&
        LoadShaderFromFile(fragmentShaderFilename, fragShader)) {

        cout << "Linking program" << endl;

        program = glCreateProgram();

        glAttachShader(program, vtxShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        GLint result = 0;
        GLint logLen = 0;

        glGetProgramiv(program, GL_LINK_STATUS, &result);
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);

        if (result == GL_FALSE) {
            cout << "Program linking failed" << endl;
        }

        if (logLen > 0) {
            char* ptr = (char*)_malloca(logLen);
            glGetProgramInfoLog(program, logLen, NULL, ptr);
            cout << ptr << endl;
            _freea(ptr);
        }

        glDetachShader(program, vtxShader);
        glDetachShader(program, fragShader);
    }

    // since we are not saving them anywhere:
    glDeleteShader(vtxShader);
    glDeleteShader(fragShader);

    bLoaded = true;
}

void ShaderProgram::SetUniformMatrix4x4(const char* name, const mat4& mtx) {

    int loc = glGetUniformLocation(program, name);

    if (loc == -1) {
        cout << "Could not find uniform " << name << endl;
        return;
    }

    const GLfloat* ptr = reinterpret_cast<const GLfloat*>(&mtx[0][0]);
    glUniformMatrix4fv(loc, 1, GL_FALSE, ptr);
}

ShaderProgram::~ShaderProgram() {
    Unapply();
    glDeleteProgram(program);
}

void ShaderProgram::Apply() {
    glUseProgram(program);
}

void ShaderProgram::Unapply() {
    glUseProgram(0);
}


bool ShaderProgram::LoadShaderFromFile(const char* filename, GLuint shader) {

    cout << "Compiling " << filename << endl;

    ifstream f(filename);
    if (!f.is_open())
        return false;

    string s((istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    const char* p = s.c_str();

    glShaderSource(shader, 1, &p, NULL);
    glCompileShader(shader);

    GLint compilationResult = GL_FALSE;
    GLint infoLen = 0;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compilationResult);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

    if (compilationResult == GL_FALSE) {
        cout << "Shader compilation failed" << endl;
    }

    if (infoLen > 0) {

        char* ptr = (char*)_malloca(infoLen + 1);
        glGetShaderInfoLog(shader, infoLen, NULL, ptr);
        cout << ptr << endl;
        _freea(ptr); ptr = nullptr;

        return false;
    }

    return true;
}