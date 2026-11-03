#pragma once
#include "originxr/Camera.hpp"
#include "originxr/Scene.hpp"
#include <memory>
#include <string>

namespace originxr {
// OpenGL 3.3 renderer. Owns GPU resources only after initialize().
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    bool initialize(std::string& error);
    void shutdown(); // must be called while the OpenGL context is still current
    void render(const Scene& scene, const Camera& camera,
                int framebufferWidth, int framebufferHeight,
                bool sideBySideStereo, bool wireframe);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace originxr
