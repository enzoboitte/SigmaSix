#pragma once
#include <algorithm>
#include <vector>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/render/ShaderManager.hpp>
#include <SigmaSix/components/Components.hpp>

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
            if (r.has<ShaderComponent>(e)) drawWithShader(r.get<ShaderComponent>(e), *t, s);
            else DrawRectanglePro({t->x, t->y, s.w, s.h}, {s.w / 2, s.h / 2}, t->angle, s.color);
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
        float th = sc.uniforms.count("thickness") ? sc.uniforms["thickness"] : 3.f;
        float w = s.w + 2 * th, h = s.h + 2 * th;
        float size[2] = {w, h};
        float oc[4] = {1, 1, 1, 1};
        SetShaderValue(sh, GetShaderLocation(sh, "size"), size, SHADER_UNIFORM_VEC2);
        SetShaderValue(sh, GetShaderLocation(sh, "thickness"), &th, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, GetShaderLocation(sh, "outlineColor"), oc, SHADER_UNIFORM_VEC4);
        BeginShaderMode(sh);
        DrawTexturePro(white, {0, 0, 1, 1}, {t.x, t.y, w, h}, {w / 2, h / 2}, t.angle, s.color);
        EndShaderMode();
    }
};
