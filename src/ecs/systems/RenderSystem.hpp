#pragma once
#include <algorithm>
#include <type_traits>
#include <vector>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/render/ShaderManager.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/ecs/systems/InterpolationSystem.hpp>

// Rendu des Sprite, trié par z (ordre stable via l'id d'entité).
struct RenderSystem : System {
    void draw(Registry& r) override {
        std::vector<std::pair<Entity, Transform2D*>> order;
        
        r.each<Sprite>([&](Entity e, Sprite&) {
            if (r.has<Transform2D>(e)) order.emplace_back(e, &r.get<Transform2D>(e));
        });
        std::sort(order.begin(), order.end(), [](auto& a, auto& b) {
            return a.second->z != b.second->z ? a.second->z < b.second->z : a.first < b.first;
        });
        for (auto& [e, t] : order) {
            auto& s = r.get<Sprite>(e);
            Transform2D drawT = *t;
            const Vector2 pos = renderPosition(r, e);
            drawT.x = pos.x;
            drawT.y = pos.y;
            if (r.has<ShaderComponent>(e)) drawWithShader(r.get<ShaderComponent>(e), drawT, s);
            else DrawRectanglePro({drawT.x, drawT.y, s.w, s.h}, {s.w / 2, s.h / 2}, drawT.angle, s.color);
        }
    }

private:
    ShaderManager shaders;
    Texture2D white{};

    // Quad agrandi de `thickness` px ; le shader outline colore la bordure.
    void drawWithShader(ShaderComponent& sc, Transform2D& t, Sprite& s) {
        if (white.id == 0) {
            Image img = GenImageColor(1, 1, WHITE);
            white = LoadTextureFromImage(img);
            UnloadImage(img);
        }
        Shader& sh = shaders.load(sc.name, nullptr, ("assets/shaders/" + sc.name + ".fs").c_str());
        const bool isOutline = sc.name == "outline";
        float thickness = 3.f;
        if (auto it = sc.uniforms.find("thickness"); it != sc.uniforms.end()) {
            if (const auto* value = std::get_if<float>(&it->second)) thickness = *value;
        }

        const float width = s.w + (isOutline ? 2.f * thickness : 0.f);
        const float height = s.h + (isOutline ? 2.f * thickness : 0.f);
        if (isOutline) {
            const float size[2] = {width, height};
            const float outlineColor[4] = {1.f, 1.f, 1.f, 1.f};
            SetShaderValue(sh, GetShaderLocation(sh, "size"), size, SHADER_UNIFORM_VEC2);
            SetShaderValue(sh, GetShaderLocation(sh, "thickness"), &thickness, SHADER_UNIFORM_FLOAT);
            SetShaderValue(sh, GetShaderLocation(sh, "outlineColor"), outlineColor, SHADER_UNIFORM_VEC4);
        }

        for (const auto& [name, value] : sc.uniforms) {
            const int location = GetShaderLocation(sh, name.c_str());
            if (location < 0) continue;

            std::visit([&](const auto& uniform) {
                using T = std::decay_t<decltype(uniform)>;
                if constexpr (std::is_same_v<T, float>) {
                    SetShaderValue(sh, location, &uniform, SHADER_UNIFORM_FLOAT);
                } else if constexpr (std::is_same_v<T, Vector2>) {
                    SetShaderValue(sh, location, &uniform, SHADER_UNIFORM_VEC2);
                } else if constexpr (std::is_same_v<T, Vector4>) {
                    SetShaderValue(sh, location, &uniform, SHADER_UNIFORM_VEC4);
                }
            }, value);
        }

        BeginShaderMode(sh);
        DrawTexturePro(white, {0, 0, 1, 1}, {t.x, t.y, width, height},
                       {width / 2, height / 2}, t.angle, s.color);
        EndShaderMode();
    }
};
