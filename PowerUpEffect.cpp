#include "PowerUpEffect.h"
#include "Game.h"
#include "Ball.h"

void PaddleLongerEffect::Apply(Game& game) {
    game.SetPaddleLonger(true);
}

void MultiBallEffect::Apply(Game& game) {
    game.SpawnMultiBalls();
}

void SlowBallEffect::Apply(Game& game) {
    game.SetSlowBall(true);
}

PowerUpEffect* PowerUpEffectFactory::Create(PowerUpType type) {
    switch (type) {
        case PADDLE_LONGER: return new PaddleLongerEffect();
        case MULTI_BALL:    return new MultiBallEffect();
        case SLOW_BALL:     return new SlowBallEffect();
        default:            return nullptr;
    }
}