#pragma once
#include <memory>
#include <utility>
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/ecs/systems/InterpolationSystem.hpp>

static constexpr int MAX_WALLS = 64;

struct WallShadowSystem {
    WallShadowSystem(Entity player, std::shared_ptr<SceneCamera> camera)
        : m_player(player),
          m_camera(std::move(camera)),
          m_shader(LoadShader(nullptr, "assets/shaders/wall_shadows.fs"))
    {
        if (m_shader.id == 0) {
            TraceLog(LOG_ERROR, "Could not load assets/shaders/wall_shadows.fs");
        }
    }

    ~WallShadowSystem() {
        if (m_shader.id != 0) UnloadShader(m_shader);
    }

    void draw(Registry& r) {
        if (m_shader.id == 0 || !r.has<Transform2D>(m_player))
            return;

        const Vector2 playerPos = renderPosition(r, m_player);
        const Vector2 playerScreen = m_camera->worldToScreen({playerPos.x, playerPos.y, 0.f});

        float walls[MAX_WALLS * 4];
        float angles[MAX_WALLS];
        int count = 0;
        
        r.each<Wall>([&](Entity e, Wall&) 
        {
            if (count >= MAX_WALLS || !r.has<Transform2D>(e) || !r.has<Collider>(e)) return;
            const auto& t = r.get<Transform2D>(e);
            const auto& c = r.get<Collider>(e);
            const Vector2 pos = renderPosition(r, e);
            const Vector2 screen = m_camera->worldToScreen({pos.x + c.offsetX, pos.y + c.offsetY, 0.f});

            walls[count * 4 + 0] = screen.x;
            walls[count * 4 + 1] = screen.y;
            walls[count * 4 + 2] = c.w * 0.5f * m_camera->zoom;
            walls[count * 4 + 3] = c.h * 0.5f * m_camera->zoom;
            angles[count] = t.angle * DEG2RAD;
            ++count;
        });

        const float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
        const float shadowColor[4] = {0.f, 0.f, 0.f, 1.f};

        SetShaderValue(m_shader, GetShaderLocation(m_shader, "uResolution"), resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shader, GetShaderLocation(m_shader, "uPlayerScreenPosition"), &playerScreen, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shader, GetShaderLocation(m_shader, "uShadowColor"), shadowColor, SHADER_UNIFORM_VEC4);
        SetShaderValue(m_shader, GetShaderLocation(m_shader, "uWallCount"), &count, SHADER_UNIFORM_INT);
        if (count > 0) {
            SetShaderValueV(m_shader, GetShaderLocation(m_shader, "uWalls"), walls, SHADER_UNIFORM_VEC4, count);
            SetShaderValueV(m_shader, GetShaderLocation(m_shader, "uWallAngles"), angles, SHADER_UNIFORM_FLOAT, count);
        }

        BeginShaderMode(m_shader);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
    }

private:
    Entity m_player;
    std::shared_ptr<SceneCamera> m_camera;
    Shader m_shader{};
};
