// MenuState.h
#pragma once
#include "IState.h"
#include "Game.h"

class MenuState : public IState {
private:
    int menuSelected;
    float menuBallSpeed;
    float menuPaddleSpeed;
    int menuLives;
    int menuBrickRows;
public:
    MenuState(Game* game);
    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;
    void OnExit() override;
};

// MenuState.cpp
#include "MenuState.h"
#include <string>
#include <algorithm>

MenuState::MenuState(Game* game) : IState(game) {
    // 初始化菜单默认值
    menuSelected = 0;
    menuBallSpeed = 4.0f;
    menuPaddleSpeed = 8.0f;
    menuLives = 3;
    menuBrickRows = 5;
}

void MenuState::OnEnter() {
    // 进入菜单时的初始化（如重置选中项）
    menuSelected = 0;
}

void MenuState::Update(float dt) {
    // 菜单交互逻辑
    if (IsKeyPressed(KEY_DOWN)) menuSelected = (menuSelected + 1) % 4;
    if (IsKeyPressed(KEY_UP)) menuSelected = (menuSelected - 1 + 4) % 4;

    // 调整选中项的值
    if (menuSelected == 0) {
        if (IsKeyPressed(KEY_RIGHT)) menuBallSpeed += 0.5f;
        if (IsKeyPressed(KEY_LEFT)) menuBallSpeed = std::max(2.0f, menuBallSpeed - 0.5f);
    }
    if (menuSelected == 1) {
        if (IsKeyPressed(KEY_RIGHT)) menuPaddleSpeed += 1;
        if (IsKeyPressed(KEY_LEFT)) menuPaddleSpeed = std::max(4.0f, menuPaddleSpeed - 1);
    }
    if (menuSelected == 2) {
        if (IsKeyPressed(KEY_RIGHT)) menuLives++;
        if (IsKeyPressed(KEY_LEFT)) menuLives = std::max(1, menuLives - 1);
    }
    if (menuSelected == 3) {
        if (IsKeyPressed(KEY_RIGHT)) menuBrickRows = std::min(8, menuBrickRows + 1);
        if (IsKeyPressed(KEY_LEFT)) menuBrickRows = std::max(3, menuBrickRows - 1);
    }

    // 空格开始游戏（将菜单配置同步到Game，并切换到PLAYING状态）
    if (IsKeyPressed(KEY_SPACE)) {
        // 同步菜单配置到Game（需在Game中添加setter方法）
        game->SetMenuConfig(menuBallSpeed, menuPaddleSpeed, menuLives, menuBrickRows);
        game->GetStateMachine()->SwitchState(GameStateType::PLAYING);
    }
}

void MenuState::Draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawText("BREAKOUT", game->GetScreenWidth()/2 - 160, 100, 60, BLUE);

    // 绘制菜单选项
    const char* options[] = {
        TextFormat("Ball Speed: %.1f", menuBallSpeed),
        TextFormat("Paddle Speed: %.0f", menuPaddleSpeed),
        TextFormat("Lives: %d", menuLives),
        TextFormat("Brick Rows: %d", menuBrickRows)
    };

    for (int i=0; i<4; i++) {
        Color color = (menuSelected == i) ? RED : DARKGRAY;
        DrawText(options[i], game->GetScreenWidth()/2 - 150, 220 + i*50, 30, color);
    }

    DrawText("[ UP / DOWN ] Select", game->GetScreenWidth()/2 - 180, 450, 20, DARKGRAY);
    DrawText("[ LEFT / RIGHT ] Adjust", game->GetScreenWidth()/2 - 180, 480, 20, DARKGRAY);
    DrawText("PRESS SPACE TO START", game->GetScreenWidth()/2 - 180, 530, 30, DARKGREEN);

    EndDrawing();
}

void MenuState::OnExit() {
    // 退出菜单时的清理（可选）
}