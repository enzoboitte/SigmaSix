#pragma once
#include <box2d/box2d.h>
#include <cmath>
#include <unordered_map>
#include <utility>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/ecs/systems/CollisionSystem.hpp>

struct PhysicsSystem : System {
    static constexpr float pixelsPerMeter = 100.f;

    explicit PhysicsSystem(std::shared_ptr<CollisionSystem> collisions)
        : m_collisions(std::move(collisions)),
          m_world(b2Vec2(0.f, 9.8f))
    {
        m_world.SetContactListener(m_collisions.get());
    }

    void fixedUpdate(Registry& r, float dt) override {
        for (auto it = m_bodies.begin(); it != m_bodies.end();) {
            if (!r.has<Collider>(it->first) || !r.has<Transform2D>(it->first)) {
                m_world.DestroyBody(it->second);
                it = m_bodies.erase(it);
            } else {
                ++it;
            }
        }

        r.each<Collider>([&](Entity e, Collider& collider) {
            if (!r.has<Transform2D>(e) || m_bodies.count(e)) return;

            auto& transform = r.get<Transform2D>(e);
            const bool hasRigidbody = r.has<Rigidbody>(e);
            const Rigidbody rb = hasRigidbody ? r.get<Rigidbody>(e) : Rigidbody{};

            b2BodyDef bodyDef;
            bodyDef.type = hasRigidbody
                ? (rb.isKinematic ? b2_staticBody : b2_dynamicBody)
                : b2_staticBody;
            bodyDef.position.Set(
                transform.x / pixelsPerMeter,
                transform.y / pixelsPerMeter
            );
            bodyDef.angle = transform.angle * DEG2RAD;
            bodyDef.gravityScale = rb.gravityScale;
            bodyDef.userData.pointer = static_cast<uintptr_t>(e);

            b2Body* body = m_world.CreateBody(&bodyDef);

            b2PolygonShape shape;
            const float halfWidth = collider.w / (2.f * pixelsPerMeter);
            const float halfHeight = collider.h / (2.f * pixelsPerMeter);
            shape.SetAsBox(
                halfWidth,
                halfHeight,
                b2Vec2(collider.offsetX / pixelsPerMeter,
                       collider.offsetY / pixelsPerMeter),
                0.f
            );

            b2FixtureDef fixtureDef;
            fixtureDef.shape = &shape;
            fixtureDef.isSensor = collider.isTrigger;
            if (bodyDef.type == b2_dynamicBody && halfWidth > 0.f && halfHeight > 0.f) {
                fixtureDef.density = rb.mass / (4.f * halfWidth * halfHeight);
            }
            body->CreateFixture(&fixtureDef);
            m_bodies.emplace(e, body);
            r.add<PreviousTransform2D>(e, PreviousTransform2D{transform.x, transform.y});
        });

        for (const auto& [e, body] : m_bodies) {
            if (r.has<Transform2D>(e)) {
                const auto& t = r.get<Transform2D>(e);
                r.add<PreviousTransform2D>(e, PreviousTransform2D{t.x, t.y});

                const float angle = t.angle * DEG2RAD;
                if (std::abs(body->GetAngle() - angle) > 1e-5f) {
                    body->SetTransform(body->GetPosition(), angle);
                }
            }

            if (!r.has<Rigidbody>(e) || body->GetType() != b2_dynamicBody) continue;

            auto& rb = r.get<Rigidbody>(e);
            body->SetLinearVelocity(
                b2Vec2(rb.vx / pixelsPerMeter, rb.vy / pixelsPerMeter)
            );
            body->SetGravityScale(rb.gravityScale);
            body->SetLinearDamping(rb.drag);
        }

        m_world.Step(dt, 8, 3);

        for (const auto& [e, body] : m_bodies) {
            if (!r.has<Transform2D>(e)) continue;

            const b2Vec2 position = body->GetPosition();
            auto& transform = r.get<Transform2D>(e);
            transform.x = position.x * pixelsPerMeter;
            transform.y = position.y * pixelsPerMeter;

            if (r.has<Rigidbody>(e) && body->GetType() == b2_dynamicBody) {
                const b2Vec2 velocity = body->GetLinearVelocity();
                auto& rb = r.get<Rigidbody>(e);
                rb.vx = velocity.x * pixelsPerMeter;
                rb.vy = velocity.y * pixelsPerMeter;
            }
        }
    }

private:
    std::shared_ptr<CollisionSystem> m_collisions;
    b2World m_world;
    std::unordered_map<Entity, b2Body*> m_bodies;
};
