#pragma once
#include <raylib.h>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>

// Position d'affichage interpolée si disponible, sinon position physique.
inline Vector2 renderPosition(Registry& r, Entity e) {
    if (r.has<RenderTransform2D>(e)) {
        const auto& rt = r.get<RenderTransform2D>(e);
        return {rt.x, rt.y};
    }
    const auto& t = r.get<Transform2D>(e);
    return {t.x, t.y};
}

// Lisse l'affichage entre deux pas physiques (physique 50 Hz, rendu 60 FPS).
// alpha = acc / step, fourni par la boucle principale.
struct InterpolationSystem : System {
    float alpha = 1.f;

    void update(Registry& r, float) override {
        r.each<Transform2D>([&](Entity e, Transform2D& t) {
            RenderTransform2D rt{t.x, t.y, t.angle};
            if (r.has<PreviousTransform2D>(e)) {
                const auto& p = r.get<PreviousTransform2D>(e);
                rt.x = p.x + (t.x - p.x) * alpha;
                rt.y = p.y + (t.y - p.y) * alpha;
            }
            r.add<RenderTransform2D>(e, rt);
        });
    }
};
