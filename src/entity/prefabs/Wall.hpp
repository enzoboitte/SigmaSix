#pragma once
#include <SigmaSix/entity/GameObject.hpp>

// Mur (rectangle gris) de taille w x h, centré en (x, y).
class Wall : public GameObject {
public:
    Wall(float x, float y, float w, float h) : GameObject(x, y, w, h, GRAY) {}
};
