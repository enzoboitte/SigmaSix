#pragma once
#include <raylib.h>

// Classe mère de tout objet de la scène (façon Unity : GameObject/MonoBehaviour).
// Surcharge start/update/fixedUpdate/draw dans tes classes filles.
class GameObject {
public:
    float x = 0, y = 0, angle = 0;
    float w = 32, h = 32;
    Color color = WHITE;
    bool active = true;
    bool destroyed = false;

    GameObject(float x, float y, float w, float h, Color c)
        : x(x), y(y), w(w), h(h), color(c) {}
    virtual ~GameObject() = default;

    virtual void start() {}                    // une fois, à l'ajout dans la scène
    virtual void update(float /*dt*/) {}       // chaque frame
    virtual void fixedUpdate(float /*dt*/) {}  // pas de temps fixe (physique)
    virtual void draw() {                      // rendu par défaut : rectangle centré
        DrawRectanglePro(Rectangle{x, y, w, h}, Vector2{w / 2, h / 2}, angle, color);
    }

    void destroy() { destroyed = true; }
};
