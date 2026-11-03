#pragma once
#include "originxr/Assets.hpp"
#include "originxr/Camera.hpp"
#include "originxr/RenderView.hpp"
#include "originxr/Scene.hpp"
#include <memory>
#include <string>
#include <vector>
namespace originxr {
// OpenGL 3.3 backend. No GLFW/OpenGL in the public headers or core library.
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    bool initialize(std::string& error);
    void shutdown(); // while the OpenGL context is still current
    void render(const Scene& scene, const Camera& camera,
                int framebufferWidth, int framebufferHeight,
                AssetManager& assets,bool sideBySideStereo, bool wireframe);
    // Render one or more externally supplied views. Views can be fed by a future OpenXR backend.
    void renderViews(const Scene& scene,const std::vector<RenderView>& views,
                     AssetManager& assets,bool wireframe=false);
    const RenderStats& stats() const {return stats_;}
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    RenderStats stats_{};
};
} // namespace originxr
