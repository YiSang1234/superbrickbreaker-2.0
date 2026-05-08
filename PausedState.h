// PausedState.h
#pragma once
#include "IState.h"
#include "Game.h"

class PausedState : public IState {
public:
    PausedState(Game* game);
    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;
    void OnExit() override;
};

// PausedState.cpp
#include "PausedState.h"

PausedState::PausedState(Game* game) : IState(game) {}

void PausedState::OnEnter() {
    // 进入暂停的初始化（可选）
}

void PausedState::Update(float dt) {
    // 恢复游戏
    if (IsKeyPressed(KEY_P)) {
        game->GetStateMachine()->SwitchState(GameStateType::PLAYING);
    }
}

void PausedState::Draw() {
    // 先绘制游戏画面，再叠加暂停文字
    game->DrawGamePlay();
    BeginDrawing();
    DrawText("PAUSED", game->GetScreenWidth()/2 - 100, game->GetScreenHeight()/2 - 30, 60, ORANGE);
    EndDrawing();
}

void PausedState::OnExit() {
    // 退出暂停的清理（可选）
}