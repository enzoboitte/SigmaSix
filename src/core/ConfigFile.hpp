#pragma once
#include "raylib.h"
#include <SigmaYaml/Yaml.hpp>
#include <string>

// Valeurs par défaut = utilisées si le fichier ou la clé manque.
struct SigmaConfigWindow
{
    int         width  = 800;
    int         height = 600;
    std::string title  = "Sigmasix";
    int         fps    = 60;
};

struct SigmaConfigAudio
{
    int volume = 80; // 0..100
};

struct SigmaConfig
{
    SigmaConfigWindow window;
    SigmaConfigAudio  audio;
};

// Charge un .yml chiffré ou en clair (détecté par yaml::loadFile) dans un SigmaConfig.
class ConfigFile {
public:
    explicit ConfigFile(const std::string& path)
    {
        yaml::Node doc;
        ok = yaml::loadFile(path, doc, &error);
        if (!ok) TraceLog(LOG_WARNING, "Config: %s", error.c_str());
        
        const yaml::Node& w = doc["Window"];
        data.window.width  = static_cast<int>(w["Width"].asInt(data.window.width));
        data.window.height = static_cast<int>(w["Height"].asInt(data.window.height));
        data.window.title  = w["Title"].asString(data.window.title);
        data.window.fps    = static_cast<int>(w["Fps"].asInt(data.window.fps));
        data.audio.volume  = static_cast<int>(doc["Audio"]["Volume"].asInt(data.audio.volume));
    }

    SigmaConfig data;
    bool        ok;
    std::string error;
};
