#pragma once
#include <memory>
#include <utility>
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/ecs/systems/InterpolationSystem.hpp>

struct BloodVignetteSystem : System {
    BloodVignetteSystem(Entity player, std::shared_ptr<SceneCamera> camera)
        : m_player(player),
          m_camera(std::move(camera)),
          m_shader(LoadShader(nullptr, "assets/shaders/blood_vignette.fs"))
    {
        if (m_shader.id == 0) {
            TraceLog(LOG_ERROR, "Could not load assets/shaders/blood_vignette.fs");
        }
    }

    ~BloodVignetteSystem() {
        if (m_shader.id != 0) UnloadShader(m_shader);
    }

    void update(Registry& r, float dt) override {
        m_elapsed += dt;
    }

    void draw(Registry& r) {
        if (m_shader.id == 0 || !r.has<Transform2D>(m_player))
            return;

        const float resolution[2] = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };
        const float intensity = 0.8f;
        const float time = m_elapsed;

        const int resolutionLoc = GetShaderLocation(m_shader, "uResolution");
        const int intensityLoc  = GetShaderLocation(m_shader, "uIntensity");
        const int timeLoc       = GetShaderLocation(m_shader, "uTime");

        SetShaderValue(m_shader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shader, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(m_shader);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
    }

private:
    Entity m_player;
    std::shared_ptr<SceneCamera> m_camera;
    Shader m_shader{};
    float m_elapsed = 0.f;
};
