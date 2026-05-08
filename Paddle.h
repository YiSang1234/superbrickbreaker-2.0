#pragma once
#include "raylib.h"

class Paddle {
private:
    Rectangle rect;
    float speed;
public:
    Paddle(float x, float y, float w, float h, float s);
    void Move(int screenW);
    void SetScale(float scale);
    Rectangle GetRect() const { return rect; }
    void Draw() const;
};