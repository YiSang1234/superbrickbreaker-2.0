#pragma once
#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include "Particle.h"
#include <vector>
#include <future>
#include <mutex>
#include <string>

enum GameScreen { MENU, PLAYING, PAUSED, GAME_OVER, LEVEL_SCORE, GAME_WIN, EDITOR, ARCHIVE_SELECT };
class Game {
private:
    GameScreen currentScreen;
    int screenWidth, screenHeight;
    float ballSpeedX, ballSpeedY;
    float ballRadius;
    float paddleW, paddleH, paddleSpeed;
    int brickRows, brickCols;
    float brickW, brickH, brickSpace;
    int startLives, maxGameTime;

    int currentLevel, unlockedLevel, highScore;
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
    int playerLife, score;
    float gameTime;

    float paddleLongerTime, slowBallTime;
    const float POWERUP_DURATION;

    Music bgMusic;
    Sound hitSound, powerUpSound, gameOverSound;
    Texture2D bgTexture, logoTexture;
    int introState;
    float introTimer, logoAlpha;

    // 异步加载（保留原有）
    std::future<void> loadFuture;
    std::mutex loadMutex;
    bool loadingTexture = false;
    bool loadComplete = false;
    bool loadEffectApplied = false;

    // ===== 新增 JSON 相关 =====
    bool hasSaveFile;               // 是否存在存档
    std::string saveFilePath;
    std::string errorMessage;       // 错误提示文字
    float errorMessageTimer;        // 错误提示显示时间

    // 当前关卡的配置（从 JSON 读取）
    int currentLevelConfigRows;
    int currentLevelConfigCols;
    float currentLevelBallSpeedX;
    float currentLevelBallSpeedY;
    float currentLevelBrickW;
    float currentLevelBrickH;
    float currentLevelBrickSpace;
    std::vector<std::vector<int>> currentLevelLayout;
    std::vector<Color> currentLevelRowColors;
    std::vector<int> currentLevelRowScores;

    // 私有方法
    void LoadConfig();                     // 加载默认配置（原有）
    void InitBricks();                     // 改为调用 InitBricksFromConfig
    void LoadHighScore();
    void SaveHighScore();
    void LoadUnlockedLevel();
    void SaveUnlockedLevel();
    void StartAsyncLoad();
    void CheckAsyncLoad();

    // 新增 JSON 方法
    bool LoadLevelConfig(int level);                // 从 JSON 加载关卡
    void ApplyDefaultLevelConfig(int level);        // 回退默认配置
    void InitBricksFromConfig();                    // 根据配置生成砖块
    void ShowErrorMessage(const std::string& msg);  // 显示错误

    int editingLevel;                      // 正在编辑的关卡号
    std::vector<std::vector<int>> editLayout;   // 编辑中的砖块布局（0空1砖）
    Color editSelectedColor;               // 当前选择的砖块颜色（可扩展）
    Vector2 editMousePos;                  // 鼠标位置（用于绘制高亮）
    bool editShowGrid;                     // 是否显示网格

    // 编辑模式方法
    void EnterEditMode(int level);         // 进入编辑模式
    void UpdateEditor(float dt);           // 编辑模式更新逻辑
    void DrawEditor();                     // 绘制编辑界面
    void SaveEditedLevel();                // 保存编辑后的关卡
    void LoadLevelForEdit(int level);      // 加载关卡配置到编辑缓冲区

    int selectedSlot;                      // 当前选中的存档槽位 (1-3)
    std::string archiveSlotPath[4];        // 存档文件路径，索引1-3
    struct ArchiveInfo {
        bool exists;
        int level;
        int score;
        int lives;
        std::string timestamp;             // 可选
    } archiveInfos[4];

    // 存档界面来源：用于区分是从游戏内保存还是从菜单加载
    enum ArchiveMode { ARCHIVE_SAVE, ARCHIVE_LOAD };
    ArchiveMode currentArchiveMode;

    // 私有方法
    void UpdateArchiveSelect();            // 存档选择界面更新
    void DrawArchiveSelect();              // 绘制存档选择界面
    void RefreshArchiveInfos();            // 刷新三个存档的信息
    void SaveGameToSlot(int slot);         // 保存当前游戏到指定槽位
    void LoadGameFromSlot(int slot);       // 从指定槽位加载游戏
    std::string GetArchiveInfoText(int slot); 

    bool wasPausedBeforeArchive;  

    
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
    bool IsPaused() const { return isPaused; }

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

    // ===== 存档/读档接口 =====
    void CheckForSaveFile();           // 检测存档文件
    void AskContinuePrompt();          // 询问是否继续（在菜单中调用）
    void SaveGameToFile();             // 保存游戏
    bool LoadGameFromFile();           // 加载游戏

    // Getter（供其他类使用）
    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }
    int GetScore() const { return score; }
    int GetHighScore() const { return highScore; }
    int GetCurrentLevel() const { return currentLevel; }

    void StartEditMode() { 
        if (currentScreen == MENU) {
            editingLevel = unlockedLevel;  // 默认编辑当前已解锁的最大关卡
            EnterEditMode(editingLevel);
        }
    }
    void OpenArchiveSaveMode();    // 从游戏内按S调用（保存模式）
    void OpenArchiveLoadMode();    // 从主菜单按S调用（加载
};