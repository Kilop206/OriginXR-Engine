#include "originxr/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
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
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_RGB
#define GL_RGB 0x1907
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_UNPACK_ALIGNMENT
#define GL_UNPACK_ALIGNMENT 0x0CF5
#endif

namespace originxr {
namespace {
using GenVertexArraysFn=void(APIENTRY*)(GLsizei,GLuint*);
using BindVertexArrayFn=void(APIENTRY*)(GLuint);
using DeleteVertexArraysFn=void(APIENTRY*)(GLsizei,const GLuint*);
using GenBuffersFn=void(APIENTRY*)(GLsizei,GLuint*);
using BindBufferFn=void(APIENTRY*)(GLenum,GLuint);
using BufferDataFn=void(APIENTRY*)(GLenum,std::ptrdiff_t,const void*,GLenum);
using DeleteBuffersFn=void(APIENTRY*)(GLsizei,const GLuint*);
using VertexAttribPointerFn=void(APIENTRY*)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
using EnableVertexAttribArrayFn=void(APIENTRY*)(GLuint);
using VertexAttribDivisorFn=void(APIENTRY*)(GLuint,GLuint);
using DrawElementsInstancedFn=void(APIENTRY*)(GLenum,GLsizei,GLenum,const void*,GLsizei);
using CreateShaderFn=GLuint(APIENTRY*)(GLenum);
using ShaderSourceFn=void(APIENTRY*)(GLuint,GLsizei,const char* const*,const GLint*);
using CompileShaderFn=void(APIENTRY*)(GLuint);
using GetShaderivFn=void(APIENTRY*)(GLuint,GLenum,GLint*);
using GetShaderInfoLogFn=void(APIENTRY*)(GLuint,GLsizei,GLsizei*,char*);
using DeleteShaderFn=void(APIENTRY*)(GLuint);
using CreateProgramFn=GLuint(APIENTRY*)();
using AttachShaderFn=void(APIENTRY*)(GLuint,GLuint);
using LinkProgramFn=void(APIENTRY*)(GLuint);
using GetProgramivFn=void(APIENTRY*)(GLuint,GLenum,GLint*);
using GetProgramInfoLogFn=void(APIENTRY*)(GLuint,GLsizei,GLsizei*,char*);
using DeleteProgramFn=void(APIENTRY*)(GLuint);
using UseProgramFn=void(APIENTRY*)(GLuint);
using GetUniformLocationFn=GLint(APIENTRY*)(GLuint,const char*);
using UniformMatrix4fvFn=void(APIENTRY*)(GLint,GLsizei,GLboolean,const GLfloat*);
using Uniform3fFn=void(APIENTRY*)(GLint,GLfloat,GLfloat,GLfloat);
using Uniform1iFn=void(APIENTRY*)(GLint,GLint);
using ActiveTextureFn=void(APIENTRY*)(GLenum);
using GenTexturesFn=void(APIENTRY*)(GLsizei,GLuint*);
using DeleteTexturesFn=void(APIENTRY*)(GLsizei,const GLuint*);
using BindTextureFn=void(APIENTRY*)(GLenum,GLuint);
using TexImage2DFn=void(APIENTRY*)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
using TexParameteriFn=void(APIENTRY*)(GLenum,GLenum,GLint);
using PixelStoreiFn=void(APIENTRY*)(GLenum,GLint);
template<typename T> T load(const char* name) {
    auto proc=glfwGetProcAddress(name);
    if(!proc)throw std::runtime_error(std::string("Missing OpenGL function: ")+name);
    return reinterpret_cast<T>(proc);
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
    VertexAttribDivisorFn attribDivisor=load<VertexAttribDivisorFn>("glVertexAttribDivisor");
    DrawElementsInstancedFn drawInstanced=load<DrawElementsInstancedFn>("glDrawElementsInstanced");
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
    ActiveTextureFn activeTexture=load<ActiveTextureFn>("glActiveTexture");
    GenTexturesFn genTextures=load<GenTexturesFn>("glGenTextures");
    DeleteTexturesFn deleteTextures=load<DeleteTexturesFn>("glDeleteTextures");
    BindTextureFn bindTexture=load<BindTextureFn>("glBindTexture");
    TexImage2DFn texImage2D=load<TexImage2DFn>("glTexImage2D");
    TexParameteriFn texParameteri=load<TexParameteriFn>("glTexParameteri");
    PixelStoreiFn pixelStorei=load<PixelStoreiFn>("glPixelStorei");
};
const char* vertexSource=R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aModel0;
layout(location=4) in vec4 aModel1;
layout(location=5) in vec4 aModel2;
layout(location=6) in vec4 aModel3;
uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform int uInstanced;
out vec3 vNormal;
out vec2 vUV;
void main() {
    mat4 model=(uInstanced==1) ? mat4(aModel0,aModel1,aModel2,aModel3) : uModel;
    gl_Position=uViewProjection*model*vec4(aPosition,1.0);
    vNormal=mat3(transpose(inverse(model)))*aNormal;
    vUV=aUV;
}
)GLSL";
const char* fragmentSource=R"GLSL(#version 330 core
in vec3 vNormal;
in vec2 vUV;
uniform vec3 uTint;
uniform sampler2D uSampler;
uniform int uUseTexture;
uniform int uUnlit;
out vec4 fragColor;
void main() {
    vec3 color=uTint;
    if(uUseTexture==1)color*=texture(uSampler,vUV).rgb;
    float diffuse=max(dot(normalize(vNormal),normalize(vec3(.3,.85,.55))),0.0);
    float light=(uUnlit==1) ? 1.0 : 0.35+0.65*diffuse;
    fragColor=vec4(color*light,1.0);
}
)GLSL";
struct BatchKey {
    std::string mesh,texture;
    Vec3 tint;
    bool unlit;
    bool operator<(const BatchKey& r) const {
        return std::tie(mesh,texture,tint.x,tint.y,tint.z,unlit)<
               std::tie(r.mesh,r.texture,r.tint.x,r.tint.y,r.tint.z,r.unlit);
    }
};
struct GpuMesh {GLuint vao{0},vbo{0},ebo{0},instances{0};GLsizei indexCount{0};};
} // namespace
struct Renderer::Impl {
    std::unique_ptr<Api> gl;
    GLuint shader{0},gridVAO{0},gridVBO{0};
    GLsizei gridVertexCount{0};
    GLint uVP{-1},uModel{-1},uTint{-1},uUnlit{-1},uInstanced{-1},uUseTexture{-1},uSampler{-1};
    std::unordered_map<std::string,GpuMesh> gpuMeshes;
    std::unordered_map<std::string,GLuint> gpuTextures;
    std::set<std::string> missingAssets;
    bool initialized{false};
    GLuint compile(GLenum kind,const char* source) {
        GLuint id=gl->createShader(kind);
        gl->shaderSource(id,1,&source,nullptr);gl->compileShader(id);
        GLint success=0;gl->getShader(id,GL_COMPILE_STATUS,&success);
        if(!success) {
            GLint length=0;gl->getShader(id,GL_INFO_LOG_LENGTH,&length);
            std::string msg(static_cast<std::size_t>(length>0?length:1),'\0');
            gl->shaderLog(id,static_cast<GLsizei>(msg.size()),nullptr,msg.data());
            gl->deleteShader(id);throw std::runtime_error("GLSL compile: "+msg);
        }
        return id;
    }
    void init() {
        gl=std::make_unique<Api>();
        GLuint vs=compile(GL_VERTEX_SHADER,vertexSource);
        GLuint fs=0;
        try {fs=compile(GL_FRAGMENT_SHADER,fragmentSource);}catch(...){gl->deleteShader(vs);throw;}
        shader=gl->createProgram();gl->attachShader(shader,vs);gl->attachShader(shader,fs);
        gl->linkProgram(shader);gl->deleteShader(vs);gl->deleteShader(fs);
        GLint success=0;gl->getProgram(shader,GL_LINK_STATUS,&success);
        if(!success) {
            GLint length=0;gl->getProgram(shader,GL_INFO_LOG_LENGTH,&length);
            std::string msg(static_cast<std::size_t>(length>0?length:1),'\0');
            gl->programLog(shader,static_cast<GLsizei>(msg.size()),nullptr,msg.data());
            throw std::runtime_error("GLSL link: "+msg);
        }
        uVP=gl->uniformLocation(shader,"uViewProjection");
        uModel=gl->uniformLocation(shader,"uModel");
        uTint=gl->uniformLocation(shader,"uTint");
        uUnlit=gl->uniformLocation(shader,"uUnlit");
        uInstanced=gl->uniformLocation(shader,"uInstanced");
        uUseTexture=gl->uniformLocation(shader,"uUseTexture");
        uSampler=gl->uniformLocation(shader,"uSampler");
        gl->genVAO(1,&gridVAO);gl->genBuffers(1,&gridVBO);
        std::vector<Vertex> grid;
        grid.reserve(100);
        for(int i=-12;i<=12;++i) {
            float x=static_cast<float>(i);
            grid.push_back({{x,0,-12},{0,1,0},0,0});grid.push_back({{x,0,12},{0,1,0},0,0});
            grid.push_back({{-12,0,x},{0,1,0},0,0});grid.push_back({{12,0,x},{0,1,0},0,0});
        }
        gridVertexCount=static_cast<GLsizei>(grid.size());
        gl->bindVAO(gridVAO);
        gl->bindBuffer(GL_ARRAY_BUFFER,gridVBO);
        gl->bufferData(GL_ARRAY_BUFFER,static_cast<std::ptrdiff_t>(grid.size()*sizeof(Vertex)),grid.data(),GL_STATIC_DRAW);
        gl->attribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,position)));gl->enableAttrib(0);
        gl->attribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,normal)));gl->enableAttrib(1);
        gl->attribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,u)));gl->enableAttrib(2);
        gl->bindVAO(0);
        glEnable(GL_DEPTH_TEST);glEnable(GL_SCISSOR_TEST);
        glClearColor(.035f,.055f,.09f,1.f);
        initialized=true;
    }
    void destroy() {
        if(!gl)return;
        for(auto& item:gpuMeshes) {
            auto& mesh=item.second;
            if(mesh.vbo)gl->deleteBuffers(1,&mesh.vbo);
            if(mesh.ebo)gl->deleteBuffers(1,&mesh.ebo);
            if(mesh.instances)gl->deleteBuffers(1,&mesh.instances);
            if(mesh.vao)gl->deleteVAO(1,&mesh.vao);
        }
        gpuMeshes.clear();
        for(auto& item:gpuTextures) if(item.second)gl->deleteTextures(1,&item.second);
        gpuTextures.clear();
        if(gridVBO)gl->deleteBuffers(1,&gridVBO);
        if(gridVAO)gl->deleteVAO(1,&gridVAO);
        if(shader)gl->deleteProgram(shader);
        gridVBO=gridVAO=shader=0;
        initialized=false;gl.reset();
    }
    GpuMesh* getMesh(const std::string& id,AssetManager& assets) {
        auto existing=gpuMeshes.find(id);
        if(existing!=gpuMeshes.end())return &existing->second;
        std::string error;
        auto* data=assets.mesh(id,error);
        if(!data) {if(missingAssets.insert(id).second)std::cerr<<"[OriginXR] "<<error<<'\n';return nullptr;}
        GpuMesh gpu;
        gpu.indexCount=static_cast<GLsizei>(data->indices.size());
        gl->genVAO(1,&gpu.vao);gl->genBuffers(1,&gpu.vbo);
        gl->genBuffers(1,&gpu.ebo);gl->genBuffers(1,&gpu.instances);
        gl->bindVAO(gpu.vao);
        gl->bindBuffer(GL_ARRAY_BUFFER,gpu.vbo);
        gl->bufferData(GL_ARRAY_BUFFER,static_cast<std::ptrdiff_t>(data->vertices.size()*sizeof(Vertex)),data->vertices.data(),GL_STATIC_DRAW);
        gl->bindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu.ebo);
        gl->bufferData(GL_ELEMENT_ARRAY_BUFFER,static_cast<std::ptrdiff_t>(data->indices.size()*sizeof(std::uint32_t)),data->indices.data(),GL_STATIC_DRAW);
        gl->attribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,position)));gl->enableAttrib(0);
        gl->attribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,normal)));gl->enableAttrib(1);
        gl->attribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,u)));gl->enableAttrib(2);
        gl->bindBuffer(GL_ARRAY_BUFFER,gpu.instances);
        for(GLuint col=0;col<4;++col) {
            gl->attribPointer(3+col,4,GL_FLOAT,GL_FALSE,sizeof(Mat4),reinterpret_cast<const void*>(sizeof(float)*4*col));
            gl->enableAttrib(3+col);gl->attribDivisor(3+col,1);
        }
        gl->bindVAO(0);
        return &gpuMeshes.emplace(id,gpu).first->second;
    }
    GLuint getTexture(const std::string& id,AssetManager& assets) {
        if(id.empty())return 0;
        auto existing=gpuTextures.find(id);
        if(existing!=gpuTextures.end())return existing->second;
        std::string error;
        auto* image=assets.image(id,error);
        if(!image) {if(missingAssets.insert(id).second)std::cerr<<"[OriginXR] "<<error<<'\n';return 0;}
        GLuint tex=0;gl->genTextures(1,&tex);gl->bindTexture(GL_TEXTURE_2D,tex);
        gl->pixelStorei(GL_UNPACK_ALIGNMENT,1);
        gl->texParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        gl->texParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        gl->texImage2D(GL_TEXTURE_2D,0,GL_RGB,image->width,image->height,0,GL_RGB,GL_UNSIGNED_BYTE,image->rgb.data());
        gl->bindTexture(GL_TEXTURE_2D,0);
        gpuTextures.emplace(id,tex);
        return tex;
    }
    void draw(const Scene& scene,const RenderView& view,AssetManager& assets,bool wireframe,RenderStats& stats) {
        if(view.width<=0 || view.height<=0)return;
        glViewport(view.x,view.y,view.width,view.height);
        glScissor(view.x,view.y,view.width,view.height);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        gl->useProgram(shader);
        gl->uniformMat4(uVP,1,GL_FALSE,view.viewProjection.data());
        gl->uniform1i(uSampler,0);
        gl->uniform1i(uInstanced,0);
        gl->uniform1i(uUseTexture,0);
        gl->uniform1i(uUnlit,1);
        gl->uniform3f(uTint,.19f,.31f,.46f);
        const auto identity=Mat4::identity();
        gl->uniformMat4(uModel,1,GL_FALSE,identity.data());
        gl->bindVAO(gridVAO);
        glDrawArrays(GL_LINES,0,gridVertexCount);
        ++stats.drawCalls;
        std::map<BatchKey,std::vector<Mat4>> batches;
        for(const auto& e:scene.entities()) {
            if(!e.mesh.visible)continue;
            BatchKey key{e.mesh.asset,e.material.texture,e.material.tint,e.material.unlit};
            batches[std::move(key)].push_back(e.transform.matrix());
        }
        glPolygonMode(GL_FRONT_AND_BACK,wireframe?GL_LINE:GL_FILL);
        gl->uniform1i(uInstanced,1);
        for(const auto& batch:batches) {
            GpuMesh* mesh=getMesh(batch.first.mesh,assets);
            if(!mesh)continue;
            GLuint texture=getTexture(batch.first.texture,assets);
            gl->activeTexture(GL_TEXTURE0);
            gl->bindTexture(GL_TEXTURE_2D,texture);
            gl->uniform1i(uUseTexture,texture?1:0);
            gl->uniform1i(uUnlit,batch.first.unlit?1:0);
            gl->uniform3f(uTint,batch.first.tint.x,batch.first.tint.y,batch.first.tint.z);
            gl->bindVAO(mesh->vao);
            gl->bindBuffer(GL_ARRAY_BUFFER,mesh->instances);
            gl->bufferData(GL_ARRAY_BUFFER,static_cast<std::ptrdiff_t>(batch.second.size()*sizeof(Mat4)),batch.second.data(),GL_DYNAMIC_DRAW);
            gl->drawInstanced(GL_TRIANGLES,mesh->indexCount,GL_UNSIGNED_INT,nullptr,static_cast<GLsizei>(batch.second.size()));
            ++stats.drawCalls;
            stats.instances+=static_cast<unsigned>(batch.second.size());
            stats.triangles+=static_cast<unsigned>(mesh->indexCount/3)*static_cast<unsigned>(batch.second.size());
        }
        gl->bindVAO(0);gl->bindTexture(GL_TEXTURE_2D,0);
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    }
};
Renderer::Renderer():impl_(std::make_unique<Impl>()){}
Renderer::~Renderer()=default;
bool Renderer::initialize(std::string& error) {
    try {impl_->init();error.clear();return true;}
    catch(const std::exception& e) {error=e.what();impl_->destroy();return false;}
}
void Renderer::shutdown(){impl_->destroy();}
void Renderer::renderViews(const Scene& scene,const std::vector<RenderView>& views,AssetManager& assets,bool wireframe) {
    stats_={};if(!impl_->initialized)return;
    const auto start=std::chrono::steady_clock::now();
    for(const auto& view:views)impl_->draw(scene,view,assets,wireframe,stats_);
    stats_.cpuMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
void Renderer::render(const Scene& scene,const Camera& camera,int w,int h,AssetManager& assets,bool stereo,bool wireframe) {
    if(w<=0||h<=0)return;
    std::vector<RenderView> views;
    if(stereo) {
        const int leftWidth=w/2;
        const int rightWidth=w-leftWidth;
        const float ipd=camera.interPupillaryDistance*.5f;
        views.push_back({camera.projection(static_cast<float>(leftWidth)/h,-ipd)*camera.view(-ipd),0,0,leftWidth,h});
        views.push_back({camera.projection(static_cast<float>(rightWidth)/h,ipd)*camera.view(ipd),leftWidth,0,rightWidth,h});
    } else {
        views.push_back({camera.projection(static_cast<float>(w)/h)*camera.view(),0,0,w,h});
    }
    renderViews(scene,views,assets,wireframe);
}
} // namespace originxr
