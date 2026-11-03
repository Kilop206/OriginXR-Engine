#pragma once
#include "originxr/Scene.hpp"
#include <string>

namespace originxr {
// Plain-text, versioned .oxscene files. No 3rd party JSON dependency.
// loadScene leaves the original Scene untouched on parse errors.
bool loadScene(const std::string& filename, Scene& scene, std::string& error);
bool saveScene(const std::string& filename, const Scene& scene, std::string& error);
} // namespace originxr
