#pragma once
#include "originxr/Camera.hpp"
#include "originxr/Scene.hpp"
#include <functional>
#include <string>
#include <utility>

struct GLFWwindow;
namespace originxr {
struct EngineConfig {
    int width{1280}, height{720};
    std::string title{"OriginXR Engine | Sandbox"};
    std::string scenePath{"scenes/demo.oxscene"};
    bool stereoPreview{false};
};
class Engine {
public:
    explicit Engine(EngineConfig config={});
    int run(); // blocking application loop
    Scene& scene() {return scene_;}
    const Scene& scene() const {return scene_;}
    Camera& camera() {return camera_;}
    // Register application-specific behaviour without modifying engine internals.
    void onUpdate(std::function<void(Scene&,float)> callback) {updateCallback_=std::move(callback);}
private:
    static void keyCallback(GLFWwindow*,int,int,int,int);
    static void cursorCallback(GLFWwindow*,double,double);
    void handleKey(int key,int action);
    void handleCursor(double x,double y);
    void handleMovement(GLFWwindow* window,float dt);
    void updateTitle(GLFWwindow* window,int fps);
    EngineConfig config_;
    Scene scene_;
    Camera camera_;
    bool stereo_{false},wireframe_{false},paused_{false};
    std::function<void(Scene&,float)> updateCallback_;
    bool captured_{true},firstMouse_{true};
    double mouseX_{0.0},mouseY_{0.0};
};
} // namespace originxr
