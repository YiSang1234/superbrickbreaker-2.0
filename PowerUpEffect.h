#pragma once
#include "raylib.h"
#include "Game.h"

class Game;

// 抽象道具效果
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(Game& game) = 0;
};

// 加长挡板
class PaddleLongerEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override;
};

// 多球
class MultiBallEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override;
};

// 减速球
class SlowBallEffect : public PowerUpEffect {
public:
    void Apply(Game& game) override;
};

// 工厂
class PowerUpEffectFactory {
public:
    static PowerUpEffect* Create(PowerUpType type);
};