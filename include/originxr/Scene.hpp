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
    Vec3 color{0.4f,0.7f,1.0f};
    bool visible{true};
};

struct SpinComponent {
    Vec3 degreesPerSecond{}; // zero means stationary
};

struct Entity {
    EntityId id{0};
    std::string name;
    Transform transform;
    MeshComponent mesh;
    SpinComponent spin;
};

class Scene {
public:
    Entity& createEntity(std::string name);
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
