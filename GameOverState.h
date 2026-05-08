// GameOverState.h
#pragma once
#include "IState.h"
#include "Game.h"

class GameOverState : public IState {
public:
    GameOverState(Game* game);
    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;
    void OnExit() override;
};

// GameOverState.cpp
#include "GameOverState.h"
#include <string>

GameOverState::GameOverState(Game* game) : IState(game) {}

void GameOverState::OnEnter() {
    // 进入游戏结束的初始化（可选）
}

void GameOverState::Update(float dt) {
    // 返回菜单
    if (IsKeyPressed(KEY_SPACE)) {
        game->GetStateMachine()->SwitchState(GameStateType::MENU);
    }
}

void GameOverState::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("GAME OVER", game->GetScreenWidth()/2 - 150, 200, 60, RED);
    DrawText(TextFormat("SCORE: %d", game->GetScore()), game->GetScreenWidth()/2 - 120, 300, 30, GOLD);
    DrawText("SPACE TO MENU", game->GetScreenWidth()/2 - 150, 400, 30, DARKGRAY);
    EndDrawing();
}

void GameOverState::OnExit() {
    // 退出游戏结束的清理（可选）
}