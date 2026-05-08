#include "Particle.h"
#include <cmath>
#include <cstdlib>

Particle::Particle(Vector2 p, Color c) : pos(p), color(c), active(true) {
    life = maxLife = 1.0f;
    size = (rand()%8 + 4)/10.0f;
    float ang = (rand()%360) * DEG2RAD;
    float sp = rand()%50 + 30;
    vel.x = cosf(ang)*sp;
    vel.y = sinf(ang)*sp;
}

void Particle::Update(float dt) {
    if (!active) return;
    life -= dt;
    if (life <= 0) { active = false; return; }
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    vel.y += 100 * dt;
}

void Particle::Draw() const {
    if (!active) return;
    Color c = color;
    c.a = (unsigned char)(255 * (life/maxLife));
    DrawCircleV(pos, size, c);
}