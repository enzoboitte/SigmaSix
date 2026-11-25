#pragma once
#include <SigmaSix/prefabs/Entity.hpp>
#include <SigmaSix/prefabs/Components/PlayerController.hpp>

// Joueur (carré vert) : Entity + PlayerController (le déplacement est dans le composant).
class Player : public Entity {
public:
    Player(float x, float y) : Entity(x, y, 32, 32, GREEN) {
        addScript<PlayerController>();
    }
};
