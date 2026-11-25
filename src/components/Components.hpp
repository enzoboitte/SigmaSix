#pragma once
#include <raylib.h>

class b2Body;

struct Transform { float x = 0, y = 0, angle = 0; };
struct Sprite    { Color color = WHITE; float w = 32, h = 32; };
struct Physics   { b2Body* body = nullptr; };
