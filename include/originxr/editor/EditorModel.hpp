#pragma once
#include "originxr/Camera.hpp"
#include "originxr/Scene.hpp"
#include <cstddef>
#include <vector>
namespace originxr::editor {
// Scene snapshots preserve entity IDs and hierarchy across undo and redo.
class UndoHistory {
public:
    explicit UndoHistory(std::size_t capacity=64):capacity_(capacity) {}
    void checkpoint(const Scene& scene);
    bool undo(Scene& scene);
    bool redo(Scene& scene);
    void clear();
    std::size_t undoCount() const { return undo_.size(); }
    std::size_t redoCount() const { return redo_.size(); }
private:
    std::size_t capacity_;
    std::vector<Scene> undo_, redo_;
};
struct Ray { Vec3 origin{},direction{}; };
Ray cameraRay(const Camera& camera,float pixelX,float pixelY,float width,float height);
// CPU ray-oriented-box picking. Uses unit cube bounds for OBJ meshes (approximation).
// Built-in plane uses an XZ thin slab.
EntityId pickEntity(const Scene& scene,const Ray& ray);
} // namespace originxr::editor
