#pragma once
#include "raylib.h"

class Brick {
private:
    Rectangle rect;
    Color color;
    bool active;
    int score;
public:
    Brick() = default;
    Brick(float x, float y, float w, float h, Color c, int s);
    bool IsActive() const;
    void SetActive(bool a);
    Rectangle GetRect() const;
    int GetScoreValue() const;
    Color GetColor() const { return color; }
    void SetColor(Color c) { color = c; }
    void Draw() const;
};