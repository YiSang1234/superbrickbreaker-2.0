#include "Paddle.h"

Paddle::Paddle(float x, float y, float w, float h, float s) : speed(s) {
    rect = {x, y, w, h};
}

void Paddle::Move(int w) {
    if (IsKeyDown(KEY_LEFT)) rect.x -= speed;
    if (IsKeyDown(KEY_RIGHT)) rect.x += speed;
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.width > w) rect.x = w - rect.width;
}

void Paddle::SetScale(float scale) {
    float old = rect.width;
    rect.width = 100 * scale;
    rect.x -= (rect.width - old) * 0.5f;
}

void Paddle::Draw() const {
    DrawRectangleRec(rect, SKYBLUE);
    DrawRectangleLinesEx(rect, 1, DARKBLUE);
}