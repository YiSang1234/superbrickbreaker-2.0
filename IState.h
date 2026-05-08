#pragma once
#include "raylib.h"

// 前向声明Game类，避免循环包含
class Game;

class IState {
protected:
    Game* game; // 持有游戏实例指针，用于访问游戏数据/方法
public:
    IState(Game* gameInstance) : game(gameInstance) {}
    virtual ~IState() = default;

    // 状态进入时调用（仅一次）
    virtual void OnEnter() = 0;
    // 状态帧更新
    virtual void Update(float dt) = 0;
    // 状态绘制
    virtual void Draw() = 0;
    // 状态退出时调用（仅一次）
    virtual void OnExit() = 0;
};