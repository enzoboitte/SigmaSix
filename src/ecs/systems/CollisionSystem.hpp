#pragma once
#include <box2d/box2d.h>
#include <vector>
#include <SigmaSix/core/System.hpp>

struct CollisionEvent {
    Entity a;
    Entity b;
    bool began;
};

struct CollisionSystem : System, b2ContactListener {
    void BeginContact(b2Contact* contact) override {
        record(contact, true);
    }

    void EndContact(b2Contact* contact) override {
        record(contact, false);
    }

    void fixedUpdate(Registry&, float) override {
        m_events.swap(m_pendingEvents);
        m_pendingEvents.clear();
    }

    const std::vector<CollisionEvent>& events() const { return m_events; }

private:
    void record(b2Contact* contact, bool began) {
        const Entity a = static_cast<Entity>(
            contact->GetFixtureA()->GetBody()->GetUserData().pointer
        );
        const Entity b = static_cast<Entity>(
            contact->GetFixtureB()->GetBody()->GetUserData().pointer
        );
        m_pendingEvents.push_back({a, b, began});
    }

    std::vector<CollisionEvent> m_pendingEvents;
    std::vector<CollisionEvent> m_events;
};