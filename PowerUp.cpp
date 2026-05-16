#include "PowerUp.h"

PowerUp::PowerUp(Vector2 p, PowerUpType t) : pos(p), type(t), active(true) {
    speed = 120;
    size = 18;
    switch(type) {
        case PADDLE_LONGER: color = ORANGE; break;
        case MULTI_BALL:    color = LIME;   break;
        case SLOW_BALL:     color = SKYBLUE; break;
        default:            color = GRAY;
    }
}

void PowerUp::Update() {
    if (!active) return;
    pos.y += speed * GetFrameTime();
}

void PowerUp::Draw() const {
    if (!active) return;
    DrawCircleV(pos, size, color);
    DrawCircleLines(pos.x, pos.y, size, WHITE);
}

bool PowerUp::CheckCollision(Rectangle r) const {
    return active && CheckCollisionCircleRec(pos, size, r);
}