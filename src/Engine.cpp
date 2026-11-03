#include "originxr/Engine.hpp"
#include "originxr/Renderer.hpp"
#include "originxr/SceneIO.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

namespace originxr {
Engine::Engine(EngineConfig config):config_(std::move(config)),stereo_(config_.stereoPreview) {}

void Engine::keyCallback(GLFWwindow* w,int key,int /*scancode*/,int action,int /*mods*/) {
    auto* self=static_cast<Engine*>(glfwGetWindowUserPointer(w));
    if(self)self->handleKey(key,action);
}
void Engine::cursorCallback(GLFWwindow* w,double x,double y) {
    auto* self=static_cast<Engine*>(glfwGetWindowUserPointer(w));
    if(self)self->handleCursor(x,y);
}
void Engine::handleKey(int key,int action) {
    if(action!=GLFW_PRESS) return;
    if(key==GLFW_KEY_F1) stereo_=!stereo_;
    if(key==GLFW_KEY_F2) wireframe_=!wireframe_;
    if(key==GLFW_KEY_P) paused_=!paused_;
    if(key==GLFW_KEY_F10 && glfwGetCurrentContext())
        glfwSetWindowShouldClose(glfwGetCurrentContext(),GLFW_TRUE);
    if(key==GLFW_KEY_F7) {
        Entity& e=scene_.createEntity("New Cube " + std::to_string(scene_.entities().size()+1));
        e.transform.position=camera_.position+camera_.forward()*2.f;
        e.spin.degreesPerSecond={0.f,30.f,0.f};
        std::cout<<"Created entity ID "<<e.id<<'\n';
    }
    if(key==GLFW_KEY_F8 && !scene_.entities().empty()) {
        EntityId id=scene_.entities().back().id;
        scene_.remove(id);
        std::cout<<"Removed entity ID "<<id<<'\n';
    }
    if(key==GLFW_KEY_R) camera_.reset();
    if(key==GLFW_KEY_F5) {
        std::string error;
        if(!loadScene(config_.scenePath,scene_,error)) std::cerr<<"Reload: "<<error<<'\n';
        else std::cout<<"Scene reloaded: "<<config_.scenePath<<'\n';
    }
    if(key==GLFW_KEY_F6) {
        std::string error;
        if(!saveScene(config_.scenePath,scene_,error)) std::cerr<<"Save: "<<error<<'\n';
        else std::cout<<"Scene saved: "<<config_.scenePath<<'\n';
    }
    if(key==GLFW_KEY_ESCAPE) {
        captured_=!captured_;
        firstMouse_=true;
        GLFWwindow* w=glfwGetCurrentContext();
        if(w) glfwSetInputMode(w,GLFW_CURSOR,captured_?GLFW_CURSOR_DISABLED:GLFW_CURSOR_NORMAL);
    }
}
void Engine::handleCursor(double x,double y) {
    if(!captured_)return;
    if(firstMouse_) {mouseX_=x;mouseY_=y;firstMouse_=false;return;}
    const double dx=x-mouseX_,dy=y-mouseY_;
    mouseX_=x;mouseY_=y;
    camera_.yaw+=static_cast<float>(dx*0.12);
    camera_.pitch=std::clamp(camera_.pitch-static_cast<float>(dy*0.12),-85.f,85.f);
}
void Engine::handleMovement(GLFWwindow* w,float dt) {
    Vec3 front=camera_.forward();front.y=0;front=normalized(front);
    Vec3 right=camera_.right();
    Vec3 move{};
    if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS)move+=front;
    if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS)move+=front*-1.f;
    if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS)move+=right;
    if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS)move+=right*-1.f;
    if(glfwGetKey(w,GLFW_KEY_E)==GLFW_PRESS)move.y+=1.f;
    if(glfwGetKey(w,GLFW_KEY_Q)==GLFW_PRESS)move.y-=1.f;
    const float speed=glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS?7.f:3.f;
    camera_.position+=normalized(move)*(speed*dt);
    camera_.position.y=std::max(camera_.position.y,0.15f);
}
void Engine::updateTitle(GLFWwindow* w,int fps) {
    std::ostringstream title;
    title<<config_.title<<" | "<<fps<<" FPS | "<<(stereo_?"Stereo preview":"Mono")
         <<(wireframe_?" | Wireframe":"")<<(paused_?" | Paused":"")
         <<" | "<<scene_.entities().size()<<" entities";
    glfwSetWindowTitle(w,title.str().c_str());
}
int Engine::run() {
    if(!glfwInit()) {std::cerr<<"Could not initialize GLFW\n";return 1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES,4);
    GLFWwindow* w=glfwCreateWindow(config_.width,config_.height,config_.title.c_str(),nullptr,nullptr);
    if(!w) {std::cerr<<"Could not create OpenGL 3.3 window\n";glfwTerminate();return 2;}
    glfwMakeContextCurrent(w);
    glfwSwapInterval(1);
    Renderer renderer;
    std::string error;
    if(!renderer.initialize(error)) {
        std::cerr<<"OpenGL initialization failed: "<<error<<'\n';
        renderer.shutdown();glfwDestroyWindow(w);glfwTerminate();return 3;
    }
    if(!config_.scenePath.empty()) {
        if(!loadScene(config_.scenePath,scene_,error))
            std::cerr<<"Scene not loaded (using existing scene): "<<error<<'\n';
    }
    glfwSetWindowUserPointer(w,this);
    glfwSetKeyCallback(w,&Engine::keyCallback);
    glfwSetCursorPosCallback(w,&Engine::cursorCallback);
    glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    std::cout<<"Controls: WASD move | mouse look | Q/E height | SHIFT speed | R reset\n"
             <<"F1 stereo | F2 wireframe | F5 load | F6 save | F7 add cube | F8 delete last | P pause | F10 exit | Esc mouse\n";
    double last=glfwGetTime(),fpsFrom=last;
    int frameCount=0;
    while(!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        const double now=glfwGetTime();
        const float dt=static_cast<float>(std::clamp(now-last,0.0,0.05));
        last=now;
        handleMovement(w,dt);
        if(!paused_) {
            scene_.update(dt);
            if(updateCallback_) updateCallback_(scene_,dt);
        }
        int fw=0,fh=0;glfwGetFramebufferSize(w,&fw,&fh);
        if(fw>0 && fh>0) {renderer.render(scene_,camera_,fw,fh,stereo_,wireframe_);glfwSwapBuffers(w);}
        ++frameCount;
        if(now-fpsFrom>=1.0) {
            updateTitle(w,static_cast<int>(frameCount/(now-fpsFrom)));
            fpsFrom=now;frameCount=0;
        }
    }
    renderer.shutdown();
    glfwDestroyWindow(w);
    glfwTerminate();
    return 0;
}
} // namespace originxr
