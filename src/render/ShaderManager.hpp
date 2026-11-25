#pragma once
// SigmaSix/render/ShaderManager.hpp
#include "raylib.h"
#include <string>
#include <unordered_map>

class ShaderManager {
public:
    Shader& load(const std::string& name, const char* vs, const char* fs) {
        auto it = shaders.find(name);
        if (it != shaders.end()) return it->second;
        return shaders[name] = LoadShader(vs, fs);
    }
    Shader& get(const std::string& name) { return shaders.at(name); }
    ~ShaderManager() { for (auto& [_, s] : shaders) UnloadShader(s); }
private:
    std::unordered_map<std::string, Shader> shaders;
};