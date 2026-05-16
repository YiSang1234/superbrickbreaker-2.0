#pragma once
#include "raylib.h"

enum PowerUpType { PADDLE_LONGER, MULTI_BALL, SLOW_BALL };

class PowerUp {
private:
    Vector2 pos;
    float speed;
    float size;
    PowerUpType type;
    Color color;
    bool active;
public:
    PowerUp(Vector2 p, PowerUpType t);
    void Update();
    void Draw() const;
    bool CheckCollision(Rectangle r) const;
    void Deactivate() { active = false; }
    bool IsActive() const { return active; }
    PowerUpType GetType() const { return type; }
    Vector2 GetPosition() const { return pos; }
};