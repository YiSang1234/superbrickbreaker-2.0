#include "Ball.h"
#include <cmath>
#include <algorithm>

inline float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

Ball::Ball(Vector2 p, Vector2 v, float r) : pos(p), vel(v), radius(r) {}
void Ball::Reset(Vector2 np, Vector2 nv) { pos = np; vel = nv; }
void Ball::Move(float m) { pos.x += vel.x * m; pos.y += vel.y * m; }

void Ball::BounceEdge(int w, int h) {
    if (pos.x - radius <= 0 || pos.x + radius >= w) {
        vel.x *= -1;
        if (pos.x < radius) pos.x = radius;
        if (pos.x > w - radius) pos.x = w - radius;
    }
    if (pos.y - radius <= 0) {
        vel.y *= -1;
        pos.y = radius;
    }
}

bool Ball::CheckPaddleCollision(Rectangle r) {
    if (!CheckCollisionCircleRec(pos, radius, r) || vel.y < 0) return false;
    vel.y = -fabs(vel.y);
    float hit = (pos.x - (r.x + r.width/2)) / (r.width/2);
    vel.x = hit * 5.0f;
    pos.y = r.y - radius - 1;
    return true;
}

bool Ball::CheckBrickCollision(Rectangle r) {
    if (!CheckCollisionCircleRec(pos, radius, r)) return false;

    float closestX = Clamp(pos.x, r.x, r.x + r.width);
    float closestY = Clamp(pos.y, r.y, r.y + r.height);
    float dx = pos.x - closestX;
    float dy = pos.y - closestY;
    float distSq = dx * dx + dy * dy;

    if (distSq < 0.0001f) {
        if (fabsf(vel.y) > fabsf(vel.x)) {
            if (vel.y > 0) { pos.y = r.y - radius; vel.y *= -1; }
            else           { pos.y = r.y + r.height + radius; vel.y *= -1; }
        } else {
            if (vel.x > 0) { pos.x = r.x - radius; vel.x *= -1; }
            else           { pos.x = r.x + r.width + radius; vel.x *= -1; }
        }
        return true;
    }

    float dist = sqrtf(distSq);
    float overlap = radius - dist;
    float nx = dx / dist;
    float ny = dy / dist;

    pos.x += nx * overlap;
    pos.y += ny * overlap;

    float dot = vel.x * nx + vel.y * ny;
    if (dot < 0) {
        vel.x -= 2 * dot * nx;
        vel.y -= 2 * dot * ny;
    }
    return true;
}

bool Ball::CheckBottomDeath(int h) {
    return pos.y + radius >= h;
}

void Ball::Draw() const {
    DrawCircleV(pos, radius, RED);
}

void Ball::DrawAsClone() const {
    DrawCircleV(pos, radius, SKYBLUE);
}