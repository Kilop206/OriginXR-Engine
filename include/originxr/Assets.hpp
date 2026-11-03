#pragma once
#include "originxr/Math.hpp"
#include <cstdint>
#include <memory>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>

namespace originxr {
struct Vertex {
    Vec3 position{};
    Vec3 normal{0.f,1.f,0.f};
    float u{0.f},v{0.f};
};
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    bool valid() const;
};
struct ImageData {
    int width{0},height{0};
    std::vector<std::uint8_t> rgb;
    bool valid() const;
};

// AssetManager never touches a graphics API. Paths and built-in IDs are cache keys.
// Resources are immutable after registration to keep the GPU upload cache coherent.
class AssetManager {
public:
    AssetManager();
    void setRoot(std::string root) { root_=std::move(root); }
    const MeshData* mesh(const std::string& id, std::string& error);
    const ImageData* image(const std::string& id, std::string& error);
    bool registerMesh(std::string id, MeshData data, std::string& error);
    bool registerImage(std::string id, ImageData data, std::string& error);
    std::size_t meshCount() const { return meshes_.size(); }
    std::size_t imageCount() const { return images_.size(); }
private:
    std::string root_;
    std::unordered_map<std::string, std::shared_ptr<const MeshData>> meshes_;
    std::unordered_map<std::string, std::shared_ptr<const ImageData>> images_;
};

bool loadObj(const std::string& path, MeshData& result, std::string& error);
bool loadPpm(const std::string& path, ImageData& result, std::string& error);
MeshData createCubeMesh();
MeshData createPlaneMesh();
} // namespace originxr
