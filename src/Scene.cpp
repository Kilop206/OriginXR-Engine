#include "originxr/Scene.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace originxr {
Mat4 Transform::matrix() const {
    return Mat4::translation(position) * Mat4::rotationY(rotation.y)
        * Mat4::rotationX(rotation.x) * Mat4::rotationZ(rotation.z)
        * Mat4::scaling(scale);
}
Entity& Scene::createEntity(std::string name) {
    if (nextId_ == 0) throw std::overflow_error("Entity ID overflow");
    Entity e;
    e.id=nextId_++;
    e.name=std::move(name);
    entities_.push_back(e);
    return entities_.back();
}
Entity* Scene::find(EntityId id) {
    auto it=std::find_if(entities_.begin(),entities_.end(),[id](const Entity& e){return e.id==id;});
    return it==entities_.end()?nullptr:&*it;
}
const Entity* Scene::find(EntityId id) const {
    auto it=std::find_if(entities_.begin(),entities_.end(),[id](const Entity& e){return e.id==id;});
    return it==entities_.end()?nullptr:&*it;
}
bool Scene::remove(EntityId id) {
    auto it=std::remove_if(entities_.begin(),entities_.end(),[id](const Entity& e){return e.id==id;});
    if(it==entities_.end()) return false;
    entities_.erase(it, entities_.end());
    return true;
}
void Scene::clear() { entities_.clear(); nextId_=1; }
void Scene::update(float dt) {
    dt=std::clamp(dt,0.f,0.05f);
    for(auto& e:entities_) e.transform.rotation += e.spin.degreesPerSecond*dt;
}
}
