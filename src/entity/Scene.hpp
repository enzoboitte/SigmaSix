#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include <SigmaSix/entity/GameObject.hpp>

// Contient les objets et appelle leurs update / fixedUpdate / draw.
class Scene {
public:
    // Crée un objet : scene.spawn<Player>(100, 100);
    template <typename T, typename... Args>
    T& spawn(Args&&... args) {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *obj;
        m_pending.push_back(std::move(obj));
        return ref;
    }

    void update(float dt) {
        flush();
        for (auto& o : m_objects) if (o->active) o->update(dt);
        m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(),
                            [](auto& o) { return o->destroyed; }),
                        m_objects.end());
    }

    // Accumule le temps et appelle fixedUpdate à pas fixe.
    void fixedUpdate(float frameDt, float step = 1.f / 50.f) {
        m_acc += frameDt;
        while (m_acc >= step) {
            for (auto& o : m_objects) if (o->active) o->fixedUpdate(step);
            m_acc -= step;
        }
    }

    void draw() {
        for (auto& o : m_objects) if (o->active) o->draw();
    }

private:
    void flush() {
        for (auto& o : m_pending) {
            o->start();
            m_objects.push_back(std::move(o));
        }
        m_pending.clear();
    }

    std::vector<std::unique_ptr<GameObject>> m_objects, m_pending;
    float m_acc = 0;
};
