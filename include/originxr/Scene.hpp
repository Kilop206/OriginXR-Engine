#pragma once
#include "originxr/Math.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace originxr {

using EntityId = std::uint32_t;

struct Transform {
    Vec3 position{};
    Vec3 rotation{};   // Euler angles in degrees
    Vec3 scale{1.f, 1.f, 1.f};
    Mat4 matrix() const;
};

struct MeshComponent {
    std::string asset{"builtin:cube"};  // ID in AssetManager or path to OBJ
    bool visible{true};
};

struct MaterialComponent {
    Vec3 tint{0.4f,0.7f,1.0f};
    std::string texture;  // empty or path to PPM image
    bool unlit{false};
};

struct SpinComponent {
    Vec3 degreesPerSecond{}; // zero means stationary
};

struct Entity {
    EntityId id{0};
    EntityId parent{0}; // 0 = scene root, local transform relative to parent
    std::string name;
    Transform transform;
    MeshComponent mesh;
    MaterialComponent material;
    SpinComponent spin;
};

class Scene {
public:
    Entity& createEntity(std::string name);
    // Used by versioned scene persistence. Returns nullptr for invalid/duplicate IDs.
    Entity* restoreEntity(EntityId id, std::string name);
    bool setParent(EntityId child, EntityId parent);
    bool validHierarchy() const;
    Mat4 worldMatrix(EntityId id) const;
    Entity* find(EntityId id);
    const Entity* find(EntityId id) const;
    bool remove(EntityId id);
    void clear();
    void update(float dt);
    std::vector<Entity>& entities() { return entities_; }
    const std::vector<Entity>& entities() const { return entities_; }
private:
    std::vector<Entity> entities_;
    EntityId nextId_{1};
};

} // namespace originxr
