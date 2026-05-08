#include "Brick.h"

Brick::Brick(float x, float y, float w, float h, Color c, int s)
    : rect{x,y,w,h}, color(c), active(true), score(s) {}

bool Brick::IsActive() const { return active; }
void Brick::SetActive(bool a) { active = a; }
Rectangle Brick::GetRect() const { return rect; }
int Brick::GetScoreValue() const { return score; }

void Brick::Draw() const {
    if (!active) return;
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 1, BLACK);
}