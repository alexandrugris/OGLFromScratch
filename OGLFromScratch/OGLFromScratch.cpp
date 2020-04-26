#include "pch.h"
#include "utils.h"

using namespace std;
using namespace glm;

// window width and height
float w;
float h;

class OGL3DObject {

protected:
    OGL3DObject() : vbo(0), diffuseTexture(0), program(0), vao(0), worldTransform(1.0f){
    }

public:

    virtual ~OGL3DObject() {
        DeleteGraphicObjects();
    }

    void DeleteGraphicObjects(){
        // delete buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &vbo);

        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);

        vao = 0;
        vbo = 0;
    }

    void SetDiffuse(const char* texture) {
        diffuseTexture = shared_ptr<Texture>(new Texture(texture, GL_TEXTURE0));
    }
       
    void SetShaders(const char* vtxShaderFilename, const char* fragmentShaderFilename) {
        program = shared_ptr<ShaderProgram>(new ShaderProgram(vtxShaderFilename, fragmentShaderFilename));
    }

    static OGL3DObject* CreateTexturedPlane(const char* texture) {
        OGL3DObject* ret = new OGL3DObject();

        ret->vertices = {
            {-1, 1, 0, 1},
            {-1, -1, 0, 1},
            {1, 1, 0, 1},
            {1, -1, 0, 1}
        };

        ret->txcoords = {
            {0, 1, 0},
            {0, 0, 0},
            {1, 1, 0},
            {1, 0, 0}
        };

        // all normals pointing towards the camera
        ret->normals = {
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0}
        };

        ret->faces = {
            Face(VtxIdx(0, 0, 0), VtxIdx(1, 1, 1), VtxIdx(2, 2, 2)),
            Face(VtxIdx(2, 2, 2), VtxIdx(1, 1, 1), VtxIdx(3, 3, 3))
        };

        ret->SetDiffuse(texture);        
        ret->SetShaders("Shaders/vertex_diffuse.glsl", "Shaders/fragment_diffuse.glsl");

        // create the vertex buffer and send it to the GPU
        ret->UpdateGeometry();

        return ret;
    }

    static OGL3DObject* CreateFromModel(const char* model, const char* diffuse) {

        ifstream f(model);
        if (!f.is_open()) {
            return nullptr;
        }

        #define CHECK_LINE_NUMBERS(n)  if (numbers.size() < n) {cout << "line error: " << line; break; }

        OGL3DObject* ret = new OGL3DObject();

        char* ln = (char*)_malloca(1024);
        for (string line; std::getline(f, line); )
        {
            strcpy_s(ln, 1024, line.c_str());
            char* ctx = NULL;
            char* tok = strtok_s(ln, " \t/", &ctx);
            char type = 0; // vertex

            vector<float> numbers;

            while (tok != NULL) {
                
                if (*tok == '#') // comment
                    break;

                if (_stricmp(tok, "v") == 0) {
                    type = 'v';
                }
                else if (_stricmp(tok, "vn") == 0) {
                    type = 'n';
                }
                else if (_stricmp(tok, "f") == 0) {
                    type = 'f';
                }
                else if (_stricmp(tok, "vt") == 0) {
                    type = 't';
                }
                else if(*tok >= '0' && *tok <='9' || *tok == '-' || *tok == '.'){
                    numbers.push_back(float(atof(tok)));
                }

                tok = strtok_s(NULL, " \t/", &ctx);
            }

            switch (type)
            {
            case 0:
                break; // comment

            case 'f':
                CHECK_LINE_NUMBERS(9);
                ret->faces.push_back(Face(
                    VtxIdx(int(numbers[0]) - 1, int(numbers[2]) - 1, int(numbers[1]) - 1),
                    VtxIdx(int(numbers[3]) - 1, int(numbers[5]) - 1, int(numbers[4]) - 1),
                    VtxIdx(int(numbers[6]) - 1, int(numbers[8]) - 1, int(numbers[7]) - 1)
                ));
                break;
            case 'n':
                CHECK_LINE_NUMBERS(3);
                ret->normals.push_back(vec4(numbers[0], numbers[1], numbers[2], 0.0));
                break;
            case 'v':
                CHECK_LINE_NUMBERS(3);
                ret->vertices.push_back(vec4(numbers[0], numbers[1], numbers[2], 1.0));
                break;
            case 't':
                CHECK_LINE_NUMBERS(3);
                ret->txcoords.push_back(vec3(numbers[0], numbers[1], numbers[2]));
                break;
            default:
                cout << "Unknown line: " << line << endl;
                break;
            }
            
        }        
        _freea(ln);

        ret->SetDiffuse(diffuse);
        ret->SetShaders("Shaders/vertex_diffuse.glsl", "Shaders/fragment_diffuse.glsl");
        ret->UpdateGeometry();
        return ret;
    }

    
    void UpdateGeometry() {

        /* we will use simple triangles here, not indexed geometry, for simplicity*/
        /* it is faster to use indexed geometry and uses less memory */
        /* but our object loader suppors different indices for vtx, normals, tx while ogl supports a single idx*/

        int size = sizeof(Vertex) * faces.size() * 3; // 3 vertices / face
        Vertex* v = new Vertex[size];
      
        for (unsigned i = 0; i < faces.size(); i++) {
            for (int j = 0; j < 3; j++) {
                v[i * 3 + j].v = vertices [faces[i].vertices[j].vtx];
                v[i * 3 + j].n = normals  [faces[i].vertices[j].normal];
                v[i * 3 + j].t = txcoords [faces[i].vertices[j].txcoord];
            }
        }

        // clear current vbo, vao
        DeleteGraphicObjects();

        // vao
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size, v, GL_STATIC_DRAW);

        if (GL_NO_ERROR != glGetError()) {
            cout << "Error binding array buffer" << endl;
        }

        // vertex attrib pointers
        const int sizes[] = { 4, 4, 3 };
        const int offsets[] = { OFFSETOF(Vertex, v),  OFFSETOF(Vertex, n), OFFSETOF(Vertex, t) };

        for (int i = 0; i < 3; i++) {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(
                i,
                sizes[i],           // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                sizeof(Vertex),     // stride
                reinterpret_cast<void*>(offsets[i])     // array buffer offset
            );

            if (GL_NO_ERROR != glGetError()) {
                cout << "Error setting vtx attrib pointer" << endl;
            }
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        delete[] v; v = nullptr;

    }

    virtual void Render(float dt, const mat4& viewTransform, const mat4& projectionTransform) {

        SCOPED_APPLY(diffuseTexture);
        SCOPED_APPLY(program);

        program->SetUniformMatrix4x4("projection", projectionTransform);
        program->SetUniformMatrix4x4("view", viewTransform);
        program->SetUniformMatrix4x4("model", worldTransform);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, faces.size() * 3); 
    }

protected:
    vector<vec4> vertices;
    vector<vec4> normals;
    vector<vec3> txcoords;
    vector<Face> faces;

    mat4 worldTransform;
    
    GLuint vbo;
    GLuint vao;
   
    shared_ptr<Texture> diffuseTexture;
    shared_ptr<ShaderProgram> program;
};

vector <shared_ptr<OGL3DObject>> vScene;

void enableDepthTest() {
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
}

void clear() {
    glClearColor(0.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void setCullingAndWireframe(bool wireframe) {

    if (wireframe) {
        glDisable(GL_CULL_FACE);
    }
    else {
        glEnable(GL_CULL_FACE);
    }

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
}

mat4 g_ViewTransform(1.0f);
mat4 g_ProjectionTransform(1.0f);

void update(float dt) {
    g_ProjectionTransform = ortho(-w / h, w / h, -1.0f, 1.0f, -10.0f, 1000.0f);
}

void render(float dt, bool wireframe=false) {

    setCullingAndWireframe(wireframe);
    enableDepthTest();
    
    clear();

    update(dt);

    for (auto it = vScene.begin(); it != vScene.end(); it++) {
        (*it)->Render(dt, g_ViewTransform, g_ProjectionTransform);
    }
}

void WindowResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    w = float(width);
    h = float(height);
}


int main()
{
    if (!glfwInit())
        return EXIT_FAILURE;

    GLFWwindow* window = glfwCreateWindow(int(w = 2048.0f), int(h = 1024.0f), "OGL From Scratch", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);

    // make sure we load OpenGL 4.0
    glewExperimental = GL_TRUE;
    glewInit();

    // image loading library
    ilInit();

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    // initialize our scene with objects
    //auto pObj = std::shared_ptr<OGL3DObject>(OGL3DObject::CreateTexturedPlane("Assets/african_head_diffuse.png"));
    auto pObj = std::shared_ptr<OGL3DObject>(OGL3DObject::CreateFromModel("Assets/african_head.obj", "Assets/african_head_diffuse.png"));

    vScene.push_back(pObj);

    
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    double fOldTime = glfwGetTime();
    do {

        double fNewTime = glfwGetTime();
        render(float(fNewTime - fOldTime));

        fOldTime = fNewTime;

        // Swap buffers
        glfwSwapBuffers(window);

        // let the event loop work
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    glfwTerminate();

    return EXIT_SUCCESS;
}
