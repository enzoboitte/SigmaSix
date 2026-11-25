#pragma once
#include <SigmaSix/entity/GameObject.hpp>

// Joueur (carré vert) déplacé aux flèches / ZQSD.
class Player : public GameObject {
public:
    float speed = 200.f;
    Player(float x, float y) : GameObject(x, y, 32, 32, GREEN) {}

    void update(float dt) override {
        float dx = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
        float dy = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) - (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
        x += dx * speed * dt;
        y += dy * speed * dt;
    }
};
