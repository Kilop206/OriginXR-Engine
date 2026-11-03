#include "originxr/SceneIO.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace originxr {
namespace {
bool readVec(std::istream& in, Vec3& v) { return static_cast<bool>(in >> v.x >> v.y >> v.z); }
void writeVec(std::ostream& out, const Vec3& v) { out << ' ' << v.x << ' ' << v.y << ' ' << v.z; }
}
bool loadScene(const std::string& path, Scene& output, std::string& error) {
    std::ifstream file(path);
    if (!file) { error="Cannot open scene: "+path; return false; }
    Scene parsed;
    std::string line;
    std::size_t row=0;
    bool headerSeen=false;
    while (std::getline(file,line)) {
        ++row;
        const auto first = line.find_first_not_of(" \t\r");
        if(first==std::string::npos || line[first]=='#') continue;
        std::istringstream in(line);
        std::string marker;
        in>>marker;
        if(!headerSeen) {
            int version=0;
            if(marker!="originxr_scene" || !(in>>version) || version!=1) {
                error="Unsupported scene header at line "+std::to_string(row);return false;
            }
            headerSeen=true;continue;
        }
        if(marker!="entity") {error="Unknown token at line "+std::to_string(row);return false;}
        std::string name;
        Vec3 pos,rot,scale,color,spin;
        int visible=1;
        if(!(in>>std::quoted(name)) || !readVec(in,pos) || !readVec(in,rot) ||
           !readVec(in,scale) || !readVec(in,color) || !readVec(in,spin) || !(in>>visible)
           || (visible!=0 && visible!=1)) {
            error="Invalid entity at line "+std::to_string(row);return false;
        }
        std::string extra;
        if(in>>extra) {error="Extra fields at line "+std::to_string(row);return false;}
        Entity& e=parsed.createEntity(name);
        e.transform.position=pos;e.transform.rotation=rot;e.transform.scale=scale;
        e.mesh.color=color;e.spin.degreesPerSecond=spin;e.mesh.visible=visible!=0;
    }
    if(!headerSeen) { error="Scene has no header";return false; }
    output=std::move(parsed);
    error.clear();return true;
}
bool saveScene(const std::string& path, const Scene& scene, std::string& error) {
    std::ofstream file(path);
    if(!file) {error="Cannot save scene: "+path;return false;}
    file<<"# OriginXR scene, units in meters, angles in degrees\noriginxr_scene 1\n";
    file<<std::setprecision(9);
    for(const Entity& e:scene.entities()) {
        file<<"entity "<<std::quoted(e.name);
        writeVec(file,e.transform.position);writeVec(file,e.transform.rotation);
        writeVec(file,e.transform.scale);writeVec(file,e.mesh.color);
        writeVec(file,e.spin.degreesPerSecond);
        file<<' '<<(e.mesh.visible?1:0)<<'\n';
    }
    if(!file) {error="Write error: "+path;return false;}
    error.clear();return true;
}
} // namespace originxr
