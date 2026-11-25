#pragma once
#include <SigmaSix/entity/GameObject.hpp>

// Ennemi (carré rouge). Ajoute ton IA dans update/fixedUpdate.
class Enemy : public GameObject {
public:
    Enemy(float x, float y) : GameObject(x, y, 32, 32, RED) {}
};
