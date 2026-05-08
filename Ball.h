#pragma once
#include "raylib.h"

class Ball {
private:
    Vector2 pos;
    Vector2 vel;
    float radius;
public:
    Ball(Vector2 p, Vector2 v, float r);
    void Reset(Vector2 np, Vector2 nv);
    void Move(float speedMul = 1.0f);
    void BounceEdge(int w, int h);
    bool CheckPaddleCollision(Rectangle pad);
    bool CheckBrickCollision(Rectangle brk);
    bool CheckBottomDeath(int h);
    void Draw() const;
    void DrawAsClone() const;
    Vector2 GetPosition() const { return pos; }
};