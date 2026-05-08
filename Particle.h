#pragma once
#include "raylib.h"

class Particle {
private:
    Vector2 pos, vel;
    Color color;
    float size, life, maxLife;
    bool active;
public:
    Particle(Vector2 p, Color c);
    void Update(float dt);
    void Draw() const;
    bool IsActive() const { return active; }
};