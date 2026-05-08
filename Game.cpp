#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>
#include "PowerUpEffect.h"
#include <chrono>   // sleep_for
#include <thread>   // this_thread

Game::Game()
    : ball({400.0f, 530.0f}, {0.0f, 0.0f}, 10.0f),
      paddle(350.0f, 550.0f, 100.0f, 20.0f, 8.0f),
      POWERUP_DURATION(8.0f)
{
    srand((unsigned)time(NULL));
    LoadConfig();
    LoadHighScore();
    LoadUnlockedLevel();

    currentScreen = MENU;
    currentLevel = 1;
    isPaused = false;
    levelStartTimer = 1.5f;
    showLevelText = true;
    paddleLongerTime = 0;
    slowBallTime = 0;

    introState = 0;
    introTimer = 0;
    logoAlpha = 0;

    ballServed = false;
    playerLife = startLives;
    score = 0;
    gameTime = 0.0f;

    InitBricks();
    InitWindow(screenWidth, screenHeight, "Breakout | Complete Edition");
    InitAudioDevice();
    LoadAssets();
    SetTargetFPS(60);
}

Game::~Game() {
    UnloadAssets();
    CloseAudioDevice();
}

void Game::SetPaddleLonger(bool enable) {
    paddleLongerTime = enable ? POWERUP_DURATION : 0;
    paddle.SetScale(enable ? 1.6f : 1.0f);
}

void Game::SetSlowBall(bool enable) {
    slowBallTime = enable ? POWERUP_DURATION : 0;
}

void Game::SpawnMultiBalls() {
    Vector2 p = ball.GetPosition();
    multiBalls.emplace_back(p, Vector2{-3, -5}, ballRadius);
    multiBalls.emplace_back(p, Vector2{3, -5}, ballRadius);
}

void Game::LoadAssets() {
    bgTexture = LoadTexture("bg.png");
    logoTexture = LoadTexture("logo.png");
    bgMusic = LoadMusicStream("bg_music.wav");
    hitSound = LoadSound("hit.wav");
    powerUpSound = LoadSound("powerup.wav");
    gameOverSound = LoadSound("gameover.wav");
    PlayMusicStream(bgMusic);
    SetMusicVolume(bgMusic, 0.4f);
}

void Game::UnloadAssets() {
    UnloadTexture(bgTexture);
    UnloadTexture(logoTexture);
    UnloadMusicStream(bgMusic);
    UnloadSound(hitSound);
    UnloadSound(powerUpSound);
    UnloadSound(gameOverSound);
}

void Game::LoadUnlockedLevel() {
    FILE* f = fopen("unlocked.txt", "r");
    if (f) { fscanf(f, "%d", &unlockedLevel); fclose(f); }
    else unlockedLevel = 1;
}

void Game::SaveUnlockedLevel() {
    if (currentLevel >= unlockedLevel) {
        unlockedLevel = currentLevel + 1;
        FILE* f = fopen("unlocked.txt", "w");
        if (f) { fprintf(f, "%d", unlockedLevel); fclose(f); }
    }
}

void Game::LoadConfig() {
    screenWidth = 800;
    screenHeight = 600;
    ballSpeedX = 4.0f;
    ballSpeedY = -4.0f;
    ballRadius = 10.0f;
    paddleW = 100.0f;
    paddleH = 20.0f;
    paddleSpeed = 8.0f;
    brickRows = 5;
    brickCols = 10;
    brickW = 70.0f;
    brickH = 25.0f;
    brickSpace = 5.0f;
    startLives = 3;
    maxGameTime = 120;
}

void Game::LoadHighScore() {
    FILE* f = fopen("highscore.txt", "r");
    if (f) { fscanf(f, "%d", &highScore); fclose(f); }
    else highScore = 0;
}

void Game::SaveHighScore() {
    if (score > highScore) {
        highScore = score;
        FILE* f = fopen("highscore.txt", "w");
        if (f) { fprintf(f, "%d", highScore); fclose(f); }
    }
}

void Game::InitBricks() {
    bricks.clear();
    float startX = (screenWidth - (brickCols * (brickW + brickSpace))) / 2.0f;
    Color cs[] = {RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE};
    for (int r = 0; r < brickRows; r++) {
        for (int col = 0; col < brickCols; col++) {
            float x = startX + col * (brickW + brickSpace);
            float y = 60 + r * (brickH + brickSpace);
            bricks.emplace_back(x, y, brickW, brickH, cs[r % 6], 10);
        }
    }
}

void Game::ResetGame() {
    playerLife = startLives;
    score = 0;
    gameTime = 0;
    ballServed = false;
    showLevelText = true;
    levelStartTimer = 1.5f;
    paddleLongerTime = 0;
    slowBallTime = 0;
    paddle.SetScale(1.0f);
    multiBalls.clear();
    powerUps.clear();
    particles.clear();
    currentScreen = PLAYING;
    isPaused = false;
    InitBricks();
}

void Game::GotoLevel(int level) {
    currentLevel = level;
    ballSpeedX = 4.0f + (level-1)*0.5f;
    ballSpeedY = -4.0f - (level-1)*0.5f;
    brickRows = 5 + (level-1);
    if (brickRows > 8) brickRows = 8;
    ResetGame();
}

void Game::NextLevel() {
    SaveUnlockedLevel();
    currentLevel++;
    levelStartTimer = 1.5f;
    showLevelText = true;
    playerLife++;
    ballSpeedX += 0.5f;
    ballSpeedY -= 0.5f;
    if (brickRows < 8) brickRows++;
    InitBricks();
    ballServed = false;
}

void Game::SpawnParticles(Vector2 pos, Color c, int count) {
    for (int i = 0; i < count; i++)
        particles.emplace_back(pos, c);
}

void Game::UpdateParticles(float dt) {
    for (auto& p : particles) p.Update(dt);
}

void Game::DrawParticles() const {
    for (const auto& p : particles) p.Draw();
}

void Game::SpawnPowerUp(Vector2 pos) {
    if (rand() % 100 < 30)
        powerUps.emplace_back(pos, (PowerUpType)(rand() % 3));
}

void Game::UpdatePowerUps(float dt) {
    for (auto& pu : powerUps) {
        if (!pu.IsActive()) continue;
        pu.Update();
        if (pu.CheckCollision(paddle.GetRect())) {
            ApplyPowerUp(pu.GetType());
            pu.Deactivate();
            SpawnParticles({paddle.GetRect().x + paddle.GetRect().width/2, paddle.GetRect().y + paddle.GetRect().height/2}, WHITE, 12);
        }
    }
}

void Game::DrawPowerUps() const {
    for (const auto& pu : powerUps) pu.Draw();
}

void Game::ApplyPowerUp(PowerUpType type) {
    PlaySound(powerUpSound);

    auto* effect = PowerUpEffectFactory::Create(type);
    if (effect) {
        effect->Apply(*this);
        delete effect;
    }
}

void Game::UpdatePowerUpTimers(float dt) {
    if (paddleLongerTime > 0) {
        paddleLongerTime -= dt;
        if (paddleLongerTime <= 0) paddle.SetScale(1.0f);
    }
    if (slowBallTime > 0) slowBallTime -= dt;
}

void Game::DrawPowerUpUI() const {
    if (paddleLongerTime > 0) DrawText(TextFormat("LONGER: %.1fs", paddleLongerTime), 20, 35, 20, ORANGE);
    if (slowBallTime > 0) DrawText(TextFormat("SLOW: %.1fs", slowBallTime), 20, 60, 20, SKYBLUE);
}

// ==============================
// ✅ 完全修复：暂停 + L 键菜单
// ==============================
void Game::UpdateGamePlay(float dt) {
    // 第一步：全局开关 —— 只要不是游戏中，ALL STOP
    if (currentScreen != PLAYING) {
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentScreen = PAUSED;
        return;
    }
    if (IsKeyPressed(KEY_K)) {
        StartAsyncLoad();
    }

    // 第二步：按键系统
    if (IsKeyPressed(KEY_P)) {
        isPaused = !isPaused;
        if (isPaused) return;
    }
    if (!isPaused) {
        // 进入选关界面
        if (IsKeyPressed(KEY_L)) {
            currentScreen = LEVEL_SCORE;
            return;
        }
        if (IsKeyPressed(KEY_K)) {
            StartAsyncLoad();
        }
    }
    if (isPaused) return;

    if (IsKeyPressed(KEY_L)) {
        currentScreen = LEVEL_SCORE;
        return;
    }

    CheckAsyncLoad();
    // 第三步：暂停开关 —— 开启 = 整个世界冻结
    if (isPaused) {
        return;
    }

    // 下面是正常游戏逻辑
    gameTime += dt;

    if (showLevelText) {
        levelStartTimer -= dt;
        if (levelStartTimer <= 0) {
            showLevelText = false;
        }
        return;
    }

    paddle.Move(screenWidth);
    UpdatePowerUps(dt);
    UpdatePowerUpTimers(dt);
    UpdateParticles(dt);

    if (!ballServed) {
        float bx = paddle.GetRect().x + paddle.GetRect().width / 2;
        float by = paddle.GetRect().y - 10;
        ball.Reset({ bx, by }, { 0, 0 });

        if (IsKeyPressed(KEY_SPACE)) {
            ball.Reset({ bx, by }, { ballSpeedX, ballSpeedY });
            ballServed = true;
        }
    } else {
        float mul = (slowBallTime > 0) ? 0.7f : 1.0f;
        ball.Move(mul);
        ball.BounceEdge(screenWidth, screenHeight);

        if (ball.CheckPaddleCollision(paddle.GetRect())) {
            SpawnParticles(ball.GetPosition(), SKYBLUE, 6);
            PlaySound(hitSound);
        }

        for (auto& b : bricks) {
            if (b.IsActive() && ball.CheckBrickCollision(b.GetRect())) {
                b.SetActive(false);
                PlaySound(hitSound);
                score += b.GetScoreValue();
                Vector2 cen = { b.GetRect().x + b.GetRect().width / 2, b.GetRect().y + b.GetRect().height / 2 };
                SpawnParticles(cen, b.GetColor(), 10);
                SpawnPowerUp(cen);
            }
        }

        if (ball.CheckBottomDeath(screenHeight)) {
            playerLife--;
            ballServed = false;
        }
    }

    // 多球更新（只有不暂停时才会运行）
    for (auto& mb : multiBalls) {
        float mul = (slowBallTime > 0) ? 0.7f : 1.0f;
        mb.Move(mul);
        mb.BounceEdge(screenWidth, screenHeight);
        mb.CheckPaddleCollision(paddle.GetRect());

        for (auto& b : bricks) {
            if (b.IsActive() && mb.CheckBrickCollision(b.GetRect())) {
                b.SetActive(false);
            }
        }
    }

    int act = 0;
    for (auto& b : bricks) {
        if (b.IsActive()) act++;
    }

    if (act == 0) {
        NextLevel();
    }
    CheckAsyncLoad();
}
bool Game::IsGameOver() {
    if (playerLife <= 0 || gameTime >= maxGameTime) {
        PlaySound(gameOverSound);
        SaveHighScore();
        return true;
    }
    return false;
}

void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_SPACE)) ResetGame();
    if (IsKeyPressed(KEY_L)) currentScreen = LEVEL_SCORE;
}

void Game::UpdatePause() {
    if (IsKeyPressed(KEY_P)) {
        currentScreen = PLAYING;
        isPaused = false;   // 恢复时强制解除暂停
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentScreen = MENU;
        isPaused = false;   // 回主菜单也解除暂停
    }
    if (IsKeyPressed(KEY_R)) {
        ResetGame();
        isPaused = false;
    }
}
void Game::UpdateLevelScore() {
    if (IsKeyPressed(KEY_L) || IsKeyPressed(KEY_ESCAPE)) {
        currentScreen = PLAYING;
        isPaused = false;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        GotoLevel(currentLevel);
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        if (currentLevel < unlockedLevel) currentLevel++;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        if (currentLevel > 1) currentLevel--;
    }
}

void Game::DrawIntroAnimation() {
    ClearBackground(BLACK);
    introTimer += GetFrameTime();
    if (introTimer < 1.0f) logoAlpha = introTimer;
    else if (introTimer < 3.0f) logoAlpha = 1.0f;
    else if (introTimer < 4.0f) logoAlpha = 4.0f - introTimer;
    else { introState = 1; return; }

    DrawTexturePro(logoTexture,
        {0,0,(float)logoTexture.width,(float)logoTexture.height},
        {(float)screenWidth/2-200, (float)screenHeight/2-150,400,300},
        {0,0}, 0,
        {255,255,255,(unsigned char)(logoAlpha*255)});
}

void Game::DrawMenu() {
    ClearBackground(DARKBLUE);
    DrawText("BREAKOUT", 260, 80, 60, YELLOW);
    DrawText(TextFormat("HIGH SCORE: %d", highScore), 290, 170, 30, GREEN);
    DrawText("[SPACE] START GAME", 250, 300, 35, WHITE);
    DrawText("[L] LEVELS & SCORE", 250, 360, 35, LIGHTGRAY);
    DrawText("[<- ->] MOVE PADDLE", 250, 420, 25, GRAY);
}

void Game::DrawPauseMenu() {
    DrawGamePlay();
    DrawText("PAUSED", 330, 200, 60, ORANGE);
    DrawText("[P] RESUME", 330, 300, 30, WHITE);
    DrawText("[R] RESTART", 330, 340, 30, WHITE);
    DrawText("[ESC] MENU", 330, 380, 30, WHITE);
}

void Game::DrawLevelScoreScreen() {
    ClearBackground(BLACK);
    DrawText("LEVEL & SCORE BOARD", 180, 80, 50, YELLOW);
    DrawText(TextFormat("UNLOCKED: %d", unlockedLevel), 320, 160, 30, GREEN);
    DrawText(TextFormat("CURRENT: %d", currentLevel), 320, 200, 30, SKYBLUE);
    DrawText(TextFormat("HIGH SCORE: %d", highScore), 280, 260, 35, GREEN);

    DrawText("[<- ->] SELECT LEVEL", 250, 350, 30, WHITE);
    DrawText("[ENTER] START LEVEL", 250, 390, 30, WHITE);
    DrawText("[L/ESC] BACK", 250, 430, 30, GRAY);
}

void Game::DrawGamePlay() {
    if (bgTexture.id != 0) DrawTexture(bgTexture, 0, 0, WHITE);
    
    else {
        Color bg;
        switch (currentLevel % 6) {
            case 0: bg = DARKGRAY; break;
            case 1: bg = DARKBLUE; break;
            case 2: bg = DARKPURPLE; break;
            case 3: bg = DARKGREEN; break;
            default: bg = DARKBLUE;
        }
        ClearBackground(bg);
    
    if (loadingTexture && !loadComplete) {
        DrawText("Loading...", screenWidth / 2 - 100, screenHeight / 2 - 20, 40, WHITE);
    }

    ball.Draw();
    paddle.Draw();
    for (auto& b : bricks) b.Draw();
    for (auto& mb : multiBalls) mb.DrawAsClone();

    DrawPowerUps();
    DrawParticles();
    DrawPowerUpUI();

    DrawText(TextFormat("LIFE: %d", playerLife), 20, 10, 20, RED);
    DrawText(TextFormat("SCORE: %d", score), screenWidth/2 - 60, 10, 20, YELLOW);
    DrawText(TextFormat("BEST: %d", highScore), screenWidth - 140, 10, 20, GREEN);
    DrawText(TextFormat("LEVEL: %d", currentLevel), 20, 85, 20, YELLOW);

    if (showLevelText) {
        const char* txt = TextFormat("LEVEL %d", currentLevel);
        DrawText(txt, screenWidth/2 - MeasureText(txt,60)/2, screenHeight/2-30, 60, YELLOW);
    }

    if (!ballServed && !showLevelText)
        DrawText("PRESS SPACE TO START", screenWidth/2 - 160, 400, 30, SKYBLUE);

    if (isPaused)
        DrawText("PAUSED", 340, 200, 50, ORANGE);

    DrawText("[P] PAUSE  [L] MENU  [ESC] MENU", 20, 550, 20, LIGHTGRAY);
}
if (loadingTexture && !loadComplete) {
    DrawText("Loading...", screenWidth / 2 - 100, screenHeight / 2 - 20, 40, WHITE);
}
}

void Game::Run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateMusicStream(bgMusic);

        BeginDrawing();

        if (introState == 0) {
            DrawIntroAnimation();
            EndDrawing();
            continue;
        }

        switch (currentScreen) {
            case MENU: 
                UpdateMenu(); 
                DrawMenu(); 
                break;
            case PLAYING: 
                UpdateGamePlay(dt); 
                DrawGamePlay(); 
                break;
            case PAUSED: 
                UpdatePause(); 
                DrawPauseMenu(); 
                break;
            case LEVEL_SCORE: 
                UpdateLevelScore(); 
                DrawLevelScoreScreen(); 
                break;
            case GAME_OVER:
                ClearBackground(BLACK);
                DrawText("GAME OVER", 260, 200, 60, RED);
                DrawText(TextFormat("SCORE: %d", score), 320, 300, 30, YELLOW);
                DrawText(TextFormat("BEST: %d", highScore), 320, 340, 30, GREEN);
                DrawText("PRESS SPACE FOR MENU", 240, 440, 30, LIGHTGRAY);
                if (IsKeyPressed(KEY_SPACE)) currentScreen = MENU;
                break;
        }

        if (currentScreen == PLAYING && IsGameOver())
            currentScreen = GAME_OVER;

        EndDrawing();
    }
    CloseWindow();
}
void Game::StartAsyncLoad() {
    if (loadingTexture) return;   // 防止重复
    loadingTexture = true;
    loadComplete = false;
    loadEffectApplied = false;

    loadFuture = std::async(std::launch::async, [this]() {
        // 模拟耗时加载（3秒）
        std::this_thread::sleep_for(std::chrono::seconds(3));
        {
            std::lock_guard<std::mutex> lock(loadMutex);
            loadComplete = true;
        }
    });
}

void Game::CheckAsyncLoad() {
    if (!loadingTexture) return;

    bool done = false;
    {
        std::lock_guard<std::mutex> lock(loadMutex);
        done = loadComplete;
    }

    if (done && !loadEffectApplied) {
        // 效果：把存活砖块变成随机亮色
        for (auto& b : bricks) {
            if (b.IsActive()) {
                b.SetColor(Color{
                    (unsigned char)(rand() % 200 + 55),
                    (unsigned char)(rand() % 200 + 55),
                    (unsigned char)(rand() % 200 + 55),
                    255
                });
            }
        }
        loadEffectApplied = true;
        loadingTexture = false;
    }
}