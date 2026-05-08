#pragma once
#include "IState.h"
#include <unordered_map>
#include <string>
#include <stdexcept>

enum class GameStateType {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class StateMachine {
private:
    std::unordered_map<GameStateType, IState*> states; // 状态注册表
    IState* currentState = nullptr;                    // 当前激活状态
    Game* game;                                        // 游戏实例指针

public:
    StateMachine(Game* gameInstance) : game(gameInstance) {}
    ~StateMachine() {
        // 释放所有注册的状态
        for (auto& pair : states) {
            delete pair.second;
        }
    }

    // 注册状态（接管内存所有权）
    void RegisterState(GameStateType type, IState* state) {
        states[type] = state;
    }

    // 切换状态（自动调用旧状态Exit、新状态Enter）
    void SwitchState(GameStateType newStateType) {
        if (currentState != nullptr) {
            currentState->OnExit();
        }

        auto it = states.find(newStateType);
        if (it == states.end()) {
            throw std::runtime_error("State not registered!");
        }

        currentState = it->second;
        currentState->OnEnter();
    }

    // 调度当前状态的更新
    void UpdateCurrentState(float dt) {
        if (currentState != nullptr) {
            currentState->Update(dt);
        }
    }

    // 调度当前状态的绘制
    void DrawCurrentState() {
        if (currentState != nullptr) {
            currentState->Draw();
        }
    }

    // 获取当前状态（可选）
    IState* GetCurrentState() const {
        return currentState;
    }
};