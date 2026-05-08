#pragma once
#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include "Particle.h"
#include <vector>

#include <future>  // 🔑 std::async
#include <mutex>    // 🔑 std::mutex
// Game.h 公共方法区

enum GameScreen { MENU, PLAYING, PAUSED, GAME_OVER, LEVEL_SCORE };

class Game {
private:
    GameScreen currentScreen;
    int screenWidth;
    int screenHeight;
    float ballSpeedX, ballSpeedY;
    float ballRadius;
    float paddleW, paddleH, paddleSpeed;
    int brickRows, brickCols;
    float brickW, brickH, brickSpace;
    int startLives, maxGameTime;

    int currentLevel;
    int unlockedLevel;
    int highScore;
    float levelStartTimer;
    bool showLevelText;
    bool isPaused;

    Ball ball;
    Paddle paddle;
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerUps;
    std::vector<Particle> particles;
    std::vector<Ball> multiBalls;

    bool ballServed;
    int playerLife;
    int score;
    float gameTime;

    float paddleLongerTime;
    float slowBallTime;
    const float POWERUP_DURATION;

    // Audio & Visual
    Music bgMusic;
    Sound hitSound;
    Sound powerUpSound;
    Sound gameOverSound;
    Texture2D bgTexture;
    Texture2D logoTexture;
    int introState;
    float introTimer;
    float logoAlpha;

    void LoadConfig();
    void InitBricks();
    void LoadHighScore();
    void SaveHighScore();
    void LoadUnlockedLevel();
    void SaveUnlockedLevel();

    std::future<void> loadFuture;
    std::mutex loadMutex;
    bool loadingTexture = false;      // 是否正在加载
    bool loadComplete = false;        // 加载是否完成（由线程设置，主线程读取）
    bool loadEffectApplied = false;   // 效果是否已应用

    void StartAsyncLoad();            // 启动异步加载
    void CheckAsyncLoad(); 

public:
    Game();
    ~Game();
    void Run();
    void ResetGame();
    void NextLevel();
    void GotoLevel(int level);

    void UpdateGamePlay(float dt);
    void DrawGamePlay();
    bool IsGameOver();

    void DrawMenu();
    void DrawPauseMenu();
    void DrawLevelScoreScreen();
    void UpdateMenu();
    void UpdatePause();
    void UpdateLevelScore();

    void SpawnPowerUp(Vector2 pos);
    void UpdatePowerUps(float dt);
    void DrawPowerUps() const;
    void ApplyPowerUp(PowerUpType type);
    void UpdatePowerUpTimers(float dt);
    void DrawPowerUpUI() const;

    void SpawnParticles(Vector2 pos, Color c, int count = 8);
    void UpdateParticles(float dt);
    void DrawParticles() const;

    void LoadAssets();
    void UnloadAssets();
    void DrawIntroAnimation();

    void SetPaddleLonger(bool enable);
    void SetSlowBall(bool enable);
    void SpawnMultiBalls();

    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }
    int GetScore() const { return score; }
    int GetHighScore() const { return highScore; }
    int GetCurrentLevel() const { return currentLevel; }
    bool IsPaused() const { return isPaused; }
};