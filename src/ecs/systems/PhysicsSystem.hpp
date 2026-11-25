#pragma once
#include <algorithm>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>

// Intégration simple : gravité, frottement, position.
struct PhysicsSystem : System {
    static constexpr float gravity = 980.f;
    void fixedUpdate(Registry& r, float dt) override {
        r.each<Rigidbody>([&](Entity e, Rigidbody& rb) {
            if (rb.isKinematic || !r.has<Transform2D>(e)) return;
            rb.vy += gravity * rb.gravityScale * dt;
            float k = std::max(0.f, 1.f - rb.drag * dt);
            rb.vx *= k; rb.vy *= k;
            auto& t = r.get<Transform2D>(e);
            t.x += rb.vx * dt; t.y += rb.vy * dt;
        });
    }
};
