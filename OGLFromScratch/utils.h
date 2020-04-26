#pragma once

#include "pch.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

struct VtxIdx {
    VtxIdx(int v, int n, int t) : vtx(v), normal(n), txcoord(t) {}
    int vtx;
    int normal;
    int txcoord;
};

struct Vertex {
    glm::vec4 v;
    glm::vec4 n;
    glm::vec3 t;
};

struct Face {
    Face(VtxIdx&& _v1, VtxIdx&& _v2, VtxIdx&& _v3) : v1(_v1), v2(_v2), v3(_v3) {}
    union {
        struct {
            VtxIdx v1;
            VtxIdx v2;
            VtxIdx v3;
        };
        VtxIdx vertices[3];
    };
};

class Texture {

public:

    Texture(const char* path, GLuint texChannel);
    ~Texture();

    void Apply();
    void Unapply();

    bool IsLoaded() {
        return bLoaded;
    }

private:
    GLuint texGL;
    GLuint texChan;
    bool bLoaded;
};

class ShaderProgram {

public:
    ShaderProgram(const char* vtxShaderFilename, const char* fragmentShaderFilename);
    ~ShaderProgram();

    void SetUniformMatrix4x4(const char* str, const glm::mat4& m);

    void Apply();
    void Unapply();
    bool IsLoaded() {
        return bLoaded;
    }

private:
   bool LoadShaderFromFile(const char* filename, GLuint shader);

private:

    GLuint program;
    bool bLoaded;

};

template<class T>class ScopedApply {
public:

    ScopedApply(const T& ptr) : ptrObj(ptr) {
        ptr->Apply();
    }

    ~ScopedApply() {
        ptrObj->Unapply();
    }

private:
    T ptrObj;

};

template<class T> ScopedApply<T> MakeScopedApply(const T& ptr) {
    return ScopedApply<T>(ptr);
}

#define SCOPED_APPLY(v) auto v_##v = MakeScopedApply(v); 

