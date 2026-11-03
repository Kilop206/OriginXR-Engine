#include "originxr/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Small OpenGL 3.3 function loader, using GLFW's platform-independent resolver.
// Kept local to this renderer so the core remains completely graphics-independent.
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

namespace originxr {
namespace {
// On Windows APIENTRY is __stdcall, and on POSIX it is empty.
using GenVertexArraysFn = void (APIENTRY*)(GLsizei,GLuint*);
using BindVertexArrayFn = void (APIENTRY*)(GLuint);
using DeleteVertexArraysFn = void (APIENTRY*)(GLsizei,const GLuint*);
using GenBuffersFn = void (APIENTRY*)(GLsizei,GLuint*);
using BindBufferFn = void (APIENTRY*)(GLenum,GLuint);
using BufferDataFn = void (APIENTRY*)(GLenum,std::ptrdiff_t,const void*,GLenum);
using DeleteBuffersFn = void (APIENTRY*)(GLsizei,const GLuint*);
using VertexAttribPointerFn = void (APIENTRY*)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
using EnableVertexAttribArrayFn = void (APIENTRY*)(GLuint);
using CreateShaderFn = GLuint (APIENTRY*)(GLenum);
using ShaderSourceFn = void (APIENTRY*)(GLuint,GLsizei,const char* const*,const GLint*);
using CompileShaderFn = void (APIENTRY*)(GLuint);
using GetShaderivFn = void (APIENTRY*)(GLuint,GLenum,GLint*);
using GetShaderInfoLogFn = void (APIENTRY*)(GLuint,GLsizei,GLsizei*,char*);
using DeleteShaderFn = void (APIENTRY*)(GLuint);
using CreateProgramFn = GLuint (APIENTRY*)();
using AttachShaderFn = void (APIENTRY*)(GLuint,GLuint);
using LinkProgramFn = void (APIENTRY*)(GLuint);
using GetProgramivFn = void (APIENTRY*)(GLuint,GLenum,GLint*);
using GetProgramInfoLogFn = void (APIENTRY*)(GLuint,GLsizei,GLsizei*,char*);
using DeleteProgramFn = void (APIENTRY*)(GLuint);
using UseProgramFn = void (APIENTRY*)(GLuint);
using GetUniformLocationFn = GLint (APIENTRY*)(GLuint,const char*);
using UniformMatrix4fvFn = void (APIENTRY*)(GLint,GLsizei,GLboolean,const GLfloat*);
using Uniform3fFn = void (APIENTRY*)(GLint,GLfloat,GLfloat,GLfloat);
using Uniform1iFn = void (APIENTRY*)(GLint,GLint);

template<typename T> T load(const char* name) {
    auto address=glfwGetProcAddress(name);
    if(!address) throw std::runtime_error(std::string("Missing OpenGL function: ")+name);
    return reinterpret_cast<T>(address);
}

struct Api {
    GenVertexArraysFn genVAO=load<GenVertexArraysFn>("glGenVertexArrays");
    BindVertexArrayFn bindVAO=load<BindVertexArrayFn>("glBindVertexArray");
    DeleteVertexArraysFn deleteVAO=load<DeleteVertexArraysFn>("glDeleteVertexArrays");
    GenBuffersFn genBuffers=load<GenBuffersFn>("glGenBuffers");
    BindBufferFn bindBuffer=load<BindBufferFn>("glBindBuffer");
    BufferDataFn bufferData=load<BufferDataFn>("glBufferData");
    DeleteBuffersFn deleteBuffers=load<DeleteBuffersFn>("glDeleteBuffers");
    VertexAttribPointerFn attribPointer=load<VertexAttribPointerFn>("glVertexAttribPointer");
    EnableVertexAttribArrayFn enableAttrib=load<EnableVertexAttribArrayFn>("glEnableVertexAttribArray");
    CreateShaderFn createShader=load<CreateShaderFn>("glCreateShader");
    ShaderSourceFn shaderSource=load<ShaderSourceFn>("glShaderSource");
    CompileShaderFn compileShader=load<CompileShaderFn>("glCompileShader");
    GetShaderivFn getShader=load<GetShaderivFn>("glGetShaderiv");
    GetShaderInfoLogFn shaderLog=load<GetShaderInfoLogFn>("glGetShaderInfoLog");
    DeleteShaderFn deleteShader=load<DeleteShaderFn>("glDeleteShader");
    CreateProgramFn createProgram=load<CreateProgramFn>("glCreateProgram");
    AttachShaderFn attachShader=load<AttachShaderFn>("glAttachShader");
    LinkProgramFn linkProgram=load<LinkProgramFn>("glLinkProgram");
    GetProgramivFn getProgram=load<GetProgramivFn>("glGetProgramiv");
    GetProgramInfoLogFn programLog=load<GetProgramInfoLogFn>("glGetProgramInfoLog");
    DeleteProgramFn deleteProgram=load<DeleteProgramFn>("glDeleteProgram");
    UseProgramFn useProgram=load<UseProgramFn>("glUseProgram");
    GetUniformLocationFn uniformLocation=load<GetUniformLocationFn>("glGetUniformLocation");
    UniformMatrix4fvFn uniformMat4=load<UniformMatrix4fvFn>("glUniformMatrix4fv");
    Uniform3fFn uniform3f=load<Uniform3fFn>("glUniform3f");
    Uniform1iFn uniform1i=load<Uniform1iFn>("glUniform1i");
};

const char* vertexShader=R"GLSL(#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
uniform mat4 uViewProjection;
uniform mat4 uModel;
out vec3 vNormal;
void main() {
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
    vNormal = mat3(uModel) * aNormal;
}
)GLSL";
const char* fragmentShader=R"GLSL(#version 330 core
in vec3 vNormal;
uniform vec3 uColor;
uniform int uUnlit;
out vec4 FragColor;
void main() {
    float diffuse = max(dot(normalize(vNormal), normalize(vec3(0.3, 0.85, 0.55))), 0.0);
    float brightness = (uUnlit == 1) ? 1.0 : (0.35 + 0.65 * diffuse);
    FragColor = vec4(uColor * brightness, 1.0);
}
)GLSL";

// Six faces x two triangles, with position and normal per vertex.
std::vector<float> cubeVertices() {
    std::vector<float> v;
    v.reserve(36*6);
    constexpr std::array<std::array<float,3>,8> p{{
        {{-.5f,-.5f,-.5f}}, {{.5f,-.5f,-.5f}}, {{.5f,.5f,-.5f}}, {{-.5f,.5f,-.5f}},
        {{-.5f,-.5f,.5f}}, {{.5f,-.5f,.5f}}, {{.5f,.5f,.5f}}, {{-.5f,.5f,.5f}}
    }};
    struct Face { std::array<int,4> indices; Vec3 normal; };
    constexpr std::array<Face,6> faces{{
        {{{4,5,6,7}},{0,0,1}}, {{{1,0,3,2}},{0,0,-1}},
        {{{0,4,7,3}},{-1,0,0}}, {{{5,1,2,6}},{1,0,0}},
        {{{7,6,2,3}},{0,1,0}}, {{{0,1,5,4}},{0,-1,0}}
    }};
    for(const Face& face:faces) {
        for(int idx:{0,1,2,0,2,3}) {
            const auto& a=p[static_cast<std::size_t>(face.indices[static_cast<std::size_t>(idx)])];
            v.insert(v.end(),{a[0],a[1],a[2],face.normal.x,face.normal.y,face.normal.z});
        }
    }
    return v;
}
std::vector<float> gridVertices() {
    std::vector<float> v;
    v.reserve(25*4*6);
    auto vertex=[&](float x,float z) {v.insert(v.end(),{x,0.f,z,0.f,1.f,0.f});};
    for(int i=-12;i<=12;++i) {
        vertex(static_cast<float>(i),-12.f);vertex(static_cast<float>(i),12.f);
        vertex(-12.f,static_cast<float>(i));vertex(12.f,static_cast<float>(i));
    }
    return v;
}
} // namespace

struct Renderer::Impl {
    std::unique_ptr<Api> gl;
    GLuint shader{0}, cubeVAO{0}, cubeVBO{0}, gridVAO{0}, gridVBO{0};
    GLint uViewProjection{-1},uModel{-1},uColor{-1},uUnlit{-1};
    GLsizei gridVertexCount{0};
    bool initialized{false};

    GLuint compile(GLenum kind,const char* source) {
        const GLuint s=gl->createShader(kind);
        gl->shaderSource(s,1,&source,nullptr);
        gl->compileShader(s);
        GLint okay=0;gl->getShader(s,GL_COMPILE_STATUS,&okay);
        if(!okay) {
            GLint length=0;gl->getShader(s,GL_INFO_LOG_LENGTH,&length);
            std::string msg(static_cast<std::size_t>(length>0?length:1),'\0');
            gl->shaderLog(s,static_cast<GLsizei>(msg.size()),nullptr,msg.data());
            gl->deleteShader(s);throw std::runtime_error("Shader compilation: "+msg);
        }
        return s;
    }
    void upload(const std::vector<float>& vertices,GLuint& vao,GLuint& buffer) {
        gl->genVAO(1,&vao);gl->genBuffers(1,&buffer);
        gl->bindVAO(vao);gl->bindBuffer(GL_ARRAY_BUFFER,buffer);
        gl->bufferData(GL_ARRAY_BUFFER,static_cast<std::ptrdiff_t>(vertices.size()*sizeof(float)),vertices.data(),GL_STATIC_DRAW);
        gl->attribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);
        gl->enableAttrib(0);
        gl->attribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<const void*>(3*sizeof(float)));
        gl->enableAttrib(1);
        gl->bindVAO(0);
    }
    void init() {
        gl=std::make_unique<Api>();
        const GLuint vs=compile(GL_VERTEX_SHADER,vertexShader);
        GLuint fs=0;
        try {fs=compile(GL_FRAGMENT_SHADER,fragmentShader);} catch(...) {gl->deleteShader(vs);throw;}
        shader=gl->createProgram();
        gl->attachShader(shader,vs);gl->attachShader(shader,fs);
        gl->linkProgram(shader);
        gl->deleteShader(vs);gl->deleteShader(fs);
        GLint okay=0;gl->getProgram(shader,GL_LINK_STATUS,&okay);
        if(!okay) {
            GLint length=0;gl->getProgram(shader,GL_INFO_LOG_LENGTH,&length);
            std::string msg(static_cast<std::size_t>(length>0?length:1),'\0');
            gl->programLog(shader,static_cast<GLsizei>(msg.size()),nullptr,msg.data());
            throw std::runtime_error("Shader link: "+msg);
        }
        uViewProjection=gl->uniformLocation(shader,"uViewProjection");
        uModel=gl->uniformLocation(shader,"uModel");
        uColor=gl->uniformLocation(shader,"uColor");
        uUnlit=gl->uniformLocation(shader,"uUnlit");
        upload(cubeVertices(),cubeVAO,cubeVBO);
        auto grid=gridVertices();gridVertexCount=static_cast<GLsizei>(grid.size()/6);
        upload(grid,gridVAO,gridVBO);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glClearColor(0.035f,0.055f,0.09f,1.f);
        initialized=true;
    }
    void destroy() {
        if(!gl) return;
        if(cubeVBO)gl->deleteBuffers(1,&cubeVBO);
        if(gridVBO)gl->deleteBuffers(1,&gridVBO);
        if(cubeVAO)gl->deleteVAO(1,&cubeVAO);
        if(gridVAO)gl->deleteVAO(1,&gridVAO);
        if(shader)gl->deleteProgram(shader);
        cubeVBO=gridVBO=cubeVAO=gridVAO=shader=0;
        initialized=false;
        gl.reset();
    }
    void drawView(const Scene& scene,const Mat4& vp,int x,int w,int h,bool wireframe) {
        if(w<=0 || h<=0) return;
        glViewport(x,0,w,h);glScissor(x,0,w,h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl->useProgram(shader);
        gl->uniformMat4(uViewProjection,1,GL_FALSE,vp.data());
        const Mat4 identity=Mat4::identity();
        gl->uniformMat4(uModel,1,GL_FALSE,identity.data());
        gl->uniform3f(uColor,0.19f,0.31f,0.46f);
        gl->uniform1i(uUnlit,1);
        gl->bindVAO(gridVAO);
        glDrawArrays(GL_LINES,0,gridVertexCount);
        glPolygonMode(GL_FRONT_AND_BACK,wireframe?GL_LINE:GL_FILL);
        gl->uniform1i(uUnlit,0);
        gl->bindVAO(cubeVAO);
        for(const Entity& e:scene.entities()) {
            if(!e.mesh.visible) continue;
            Mat4 model=e.transform.matrix();
            gl->uniformMat4(uModel,1,GL_FALSE,model.data());
            gl->uniform3f(uColor,e.mesh.color.x,e.mesh.color.y,e.mesh.color.z);
            glDrawArrays(GL_TRIANGLES,0,36);
        }
        gl->bindVAO(0);
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    }
};

Renderer::Renderer():impl_(std::make_unique<Impl>()) {}
Renderer::~Renderer()=default;
bool Renderer::initialize(std::string& error) {
    try {impl_->init(); error.clear();return true;}
    catch(const std::exception& e) {error=e.what();impl_->destroy();return false;}
}
void Renderer::shutdown() {impl_->destroy();}
void Renderer::render(const Scene& scene,const Camera& camera,int w,int h,bool stereo,bool wireframe) {
    if(!impl_->initialized || w<=0 || h<=0) return;
    if(stereo) {
        const int leftWidth=w/2, rightWidth=w-leftWidth;
        const float offset=camera.interPupillaryDistance*0.5f;
        impl_->drawView(scene,camera.projection(static_cast<float>(leftWidth)/h,-offset)*camera.view(-offset),0,leftWidth,h,wireframe);
        impl_->drawView(scene,camera.projection(static_cast<float>(rightWidth)/h,offset)*camera.view(offset),leftWidth,rightWidth,h,wireframe);
    } else {
        impl_->drawView(scene,camera.projection(static_cast<float>(w)/h)*camera.view(),0,w,h,wireframe);
    }
}
} // namespace originxr
