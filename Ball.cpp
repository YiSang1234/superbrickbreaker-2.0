#include "Ball.h"
#include <cmath>

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

// ✅ 优化砖块碰撞：100%回弹，绝不失灵
bool Ball::CheckBrickCollision(Rectangle r) {
    if (!CheckCollisionCircleRec(pos, radius, r)) return false;

    float overlapLeft = (pos.x + radius) - r.x;
    float overlapRight = (r.x + r.width) - (pos.x - radius);
    float overlapTop = (pos.y + radius) - r.y;
    float overlapBottom = (r.y + r.height) - (pos.y - radius);

    float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
    float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

    if (minOverlapX < minOverlapY) {
        vel.x *= -1;
    } else {
        vel.y *= -1;
    }
    return true;
}

bool Ball::CheckBottomDeath(int h) {
    return pos.y + radius >= h;
}

void Ball::Draw() const {
    DrawCircleV(pos, radius, RED);
}

// ✅ 新增：分身球用蓝色
void Ball::DrawAsClone() const {
    DrawCircleV(pos, radius, SKYBLUE);
}