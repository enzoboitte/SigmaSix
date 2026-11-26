#pragma once
#include <memory>
#include <utility>
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/ecs/systems/InterpolationSystem.hpp>

struct FogOfWarSystem {
    FogOfWarSystem(Entity player, std::shared_ptr<SceneCamera> camera)
        : m_player(player),
          m_camera(std::move(camera)),
          m_shader(LoadShader(nullptr, "assets/shaders/fog_of_war.fs"))
    {
        if (m_shader.id == 0) {
            TraceLog(LOG_ERROR, "Could not load assets/shaders/fog_of_war.fs");
        }
    }

    ~FogOfWarSystem() {
        if (m_shader.id != 0) UnloadShader(m_shader);
    }

    void draw(Registry& r) {
        if (m_shader.id == 0 || !r.has<Transform2D>(m_player) || !r.has<FogOfWar>(m_player)) {
            return;
        }

        const Vector2 playerPosition = renderPosition(r, m_player);
        const auto& fog = r.get<FogOfWar>(m_player);
        const Vector2 playerScreenPosition =
            m_camera->worldToScreen(Vector3{playerPosition.x, playerPosition.y, 0.f});
        const float resolution[2] = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };
        const float fogColor[4] = {fog.color.x, fog.color.y, fog.color.z, fog.color.w};
        const int resolutionLocation = GetShaderLocation(m_shader, "uResolution");
        const int playerPositionLocation = GetShaderLocation(m_shader, "uPlayerScreenPosition");
        const int radiusLocation = GetShaderLocation(m_shader, "uVisibilityRadius");
        const int featherLocation = GetShaderLocation(m_shader, "uFeather");
        const int colorLocation = GetShaderLocation(m_shader, "uFogColor");

        SetShaderValue(m_shader, resolutionLocation, resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shader, playerPositionLocation, &playerScreenPosition, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shader, radiusLocation, &fog.visibilityRadius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_shader, featherLocation, &fog.feather, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_shader, colorLocation, fogColor, SHADER_UNIFORM_VEC4);

        BeginShaderMode(m_shader);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
    }

private:
    Entity m_player;
    std::shared_ptr<SceneCamera> m_camera;
    Shader m_shader{};
};
