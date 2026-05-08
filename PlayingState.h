// PlayingState.h
#pragma once
#include "IState.h"
#include "Game.h"

class PlayingState : public IState {
public:
    PlayingState(Game* game);
    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;
    void OnExit() override;
};

// PlayingState.cpp
#include "PlayingState.h"
#include <string>

PlayingState::PlayingState(Game* game) : IState(game) {}

void PlayingState::OnEnter() {
    // 进入游戏时重置游戏状态
    game->ResetGame();
}

void PlayingState::Update(float dt) {
    // 暂停逻辑
    if (IsKeyPressed(KEY_P)) {
        game->GetStateMachine()->SwitchState(GameStateType::PAUSED);
        return;
    }

    // 游戏核心逻辑（原Game::UpdatePlaying）
    if (!game->IsPaused()) { // 需给Game类添加IsPaused()方法
        game->UpdateGamePlay(dt);
    }

    // 检测游戏结束条件
    if (game->IsGameOver()) {
        game->GetStateMachine()->SwitchState(GameStateType::GAME_OVER);
    }
}

void PlayingState::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // 绘制游戏元素（原Game::DrawPlaying）
    game->DrawGamePlay();

    EndDrawing();
}

void PlayingState::OnExit() {
    // 退出游戏时的清理（可选）
}