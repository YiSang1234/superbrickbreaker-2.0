#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "json.hpp"  
#include "PowerUpEffect.h"

using json = nlohmann::json;

static Color StringToColor(const std::string& name) {
    if (name == "RED") return RED;
    if (name == "ORANGE") return ORANGE;
    if (name == "YELLOW") return YELLOW;
    if (name == "GREEN") return GREEN;
    if (name == "BLUE") return BLUE;
    if (name == "PURPLE") return PURPLE;
    if (name == "SKYBLUE") return SKYBLUE;
    if (name == "PINK") return PINK;
    return WHITE;
}

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

    for (int i = 1; i <= 3; ++i) {
    archiveSlotPath[i] = "saves/save" + std::to_string(i) + ".json";
    archiveInfos[i].exists = false;
}
selectedSlot = 1;

    saveFilePath = "saves/savegame.json";
    errorMessageTimer = 0.0f;
    // 确保 saves 目录存在（程序启动时创建）
    system("mkdir saves 2>nul");   // Windows, 忽略错误
    // Linux/macOS: system("mkdir -p saves");
}

Game::~Game() {
    UnloadAssets();
    CloseAudioDevice();
}

void Game::DrawArchiveSelect() {
    ClearBackground(DARKGRAY);
    
    DrawText("SELECT SAVE SLOT", screenWidth/2 - 150, 80, 40, YELLOW);
    
    for (int i = 1; i <= 3; ++i) {
        int y = 200 + (i-1) * 80;
        Color color = (selectedSlot == i) ? SKYBLUE : WHITE;
        
        std::string info = GetArchiveInfoText(i);
        DrawText(info.c_str(), screenWidth/2 - 200, y, 30, color);
        
        // 绘制选择框
        if (selectedSlot == i) {
            DrawRectangleLines(screenWidth/2 - 220, y - 5, 440, 40, SKYBLUE);
        }
    }
    
    if (currentArchiveMode == ARCHIVE_SAVE) {
        DrawText("Press ENTER to SAVE current progress to selected slot", 
                 screenWidth/2 - 300, 500, 20, LIGHTGRAY);
    } else {
        DrawText("Press ENTER to LOAD selected slot", 
                 screenWidth/2 - 200, 500, 20, LIGHTGRAY);
    }
    DrawText("Press R or ESC to cancel", screenWidth/2 - 150, 550, 20, LIGHTGRAY);
}

void Game::OpenArchiveLoadMode() {
    currentArchiveMode = ARCHIVE_LOAD;
    RefreshArchiveInfos();
    selectedSlot = 1;
    currentScreen = ARCHIVE_SELECT;
}


void Game::OpenArchiveSaveMode() {

    if (currentScreen == PLAYING) {
        wasPausedBeforeArchive = isPaused; // 需要添加成员变量
        isPaused = true;
    }
    currentArchiveMode = ARCHIVE_SAVE;
    RefreshArchiveInfos();
    selectedSlot = 1;
    currentScreen = ARCHIVE_SELECT;
}

void Game::UpdateArchiveSelect() {
    // 上下键选择槽位
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
        selectedSlot = (selectedSlot % 3) + 1;
    }
    
    // 确认键：执行当前模式的操作
    if (IsKeyPressed(KEY_ENTER)) {
        if (currentArchiveMode == ARCHIVE_SAVE) {
            SaveGameToSlot(selectedSlot);
        } else if (currentArchiveMode == ARCHIVE_LOAD) {
            LoadGameFromSlot(selectedSlot);
            // 加载后已经设置了 currentScreen = PLAYING，直接返回，不需要额外处理
            return;
        }
        // 保存完成后，返回之前的屏幕并恢复暂停状态
        if (wasPausedBeforeArchive && currentScreen == ARCHIVE_SELECT) {
            currentScreen = PLAYING;
            isPaused = false;
        } else {
            currentScreen = MENU;
        }
    }
    
    // 取消（按 R 或 ESC）
    if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ESCAPE)) {
        if (wasPausedBeforeArchive && currentScreen == ARCHIVE_SELECT) {
            currentScreen = PLAYING;
            isPaused = false;
        } else {
            currentScreen = MENU;
        }
    }

}

void Game::RefreshArchiveInfos() {
    for (int i = 1; i <= 3; ++i) {
        std::ifstream file(archiveSlotPath[i]);
        if (!file.is_open()) {
            archiveInfos[i].exists = false;
            continue;
        }
        json save;
        try {
            file >> save;
            archiveInfos[i].exists = true;
            archiveInfos[i].level = save.value("current_level", 1);
            archiveInfos[i].score = save.value("score", 0);
            archiveInfos[i].lives = save.value("player_life", 3);
            // 可选读取时间戳，这里不额外处理
        } catch (...) {
            archiveInfos[i].exists = false;
        }
    }
}


std::string Game::GetArchiveInfoText(int slot) {
    if (!archiveInfos[slot].exists)
        return "Slot " + std::to_string(slot) + " : [EMPTY]";
    else
        return "Slot " + std::to_string(slot) + " : Level " + std::to_string(archiveInfos[slot].level) +
               "  Score " + std::to_string(archiveInfos[slot].score) +
               "  Lives " + std::to_string(archiveInfos[slot].lives);
}
void Game::LoadGameFromSlot(int slot) {
    std::ifstream file(archiveSlotPath[slot]);
    if (!file.is_open()) {
        ShowErrorMessage("No save file in slot " + std::to_string(slot));
        return;
    }
    
    json save;
    try {
        file >> save;
    } catch (...) {
        ShowErrorMessage("Save file corrupted.");
        return;
    }
    
    // 恢复基本数据
    score = save.value("score", 0);
    playerLife = save.value("player_life", startLives);
    currentLevel = save.value("current_level", 1);
    unlockedLevel = save.value("unlocked_level", 1);
    highScore = save.value("high_score", highScore);
    gameTime = save.value("game_time", 0.0f);
    ballServed = save.value("ball_served", false);
    
    paddleLongerTime = save["powerup"].value("paddle_longer", 0.0f);
    slowBallTime = save["powerup"].value("slow_ball", 0.0f);
    
    // 加载关卡配置
    LoadLevelConfig(currentLevel);
    InitBricksFromConfig();
    
    // 恢复砖块状态
    if (save.contains("brick_states") && save["brick_states"].is_array()) {
        auto& states = save["brick_states"];
        if (states.size() == bricks.size()) {
            for (size_t i = 0; i < bricks.size(); ++i) {
                bricks[i].SetActive(states[i] == 1);
            }
        }
    }
    
    // 恢复主球
    if (save.contains("main_ball")) {
        float px = save["main_ball"]["position"]["x"];
        float py = save["main_ball"]["position"]["y"];
        float vx = save["main_ball"]["velocity"]["x"];
        float vy = save["main_ball"]["velocity"]["y"];
        ball.Reset({px, py}, {vx, vy});
    } else {
        // 降级处理
        ballServed = false;
    }
    
    // 恢复多球
    multiBalls.clear();
    if (save.contains("multi_balls") && save["multi_balls"].is_array()) {
        for (auto& mbJson : save["multi_balls"]) {
            Vector2 pos = {mbJson["x"], mbJson["y"]};
            Vector2 vel = {mbJson["vx"], mbJson["vy"]};
            multiBalls.emplace_back(pos, vel, ballRadius);
        }
    }
    
    // 恢复挡板位置
    if (save.contains("paddle_x")) {
        paddle.SetX(save["paddle_x"]);
    }
    
    paddle.SetScale(paddleLongerTime > 0 ? 1.6f : 1.0f);
    
    showLevelText = true;
    levelStartTimer = 1.5f;
    isPaused = false;
    currentScreen = PLAYING;  // 直接进入游戏
    
    ShowErrorMessage("Loaded slot " + std::to_string(slot));

    if (playerLife <= 0) {
        playerLife = 1;   // 至少给一条命
    }
    if (gameTime >= maxGameTime) {
        gameTime = maxGameTime - 1.0f;   // 避免超时
    }
    
    // 确保球已发球或未发球状态合理
    if (ballServed) {
        // 如果球超出边界，重置球的位置到挡板上方
        Vector2 ballPos = ball.GetPosition();
        if (ballPos.y + ballRadius > screenHeight) {
            ballServed = false;
        }
    }
    
    // 重置其他可能影响 Game Over 的标志
    showLevelText = true;
    levelStartTimer = 1.5f;
    isPaused = false;
    currentScreen = PLAYING;   // 强制设置为游戏中
    
    // 如果有必要，重置游戏计时器（防止时间耗尽）
    gameTime = 0.0f;
}

void Game::SaveGameToSlot(int slot) {
    json save;
    // 基础信息
    save["score"] = score;
    save["player_life"] = playerLife;
    save["current_level"] = currentLevel;
    save["unlocked_level"] = unlockedLevel;
    save["high_score"] = highScore;
    save["game_time"] = gameTime;
    save["ball_served"] = ballServed;
    
    // 道具剩余时间
    save["powerup"]["paddle_longer"] = paddleLongerTime;
    save["powerup"]["slow_ball"] = slowBallTime;
    
    // 主球状态
    save["main_ball"]["position"]["x"] = ball.GetPosition().x;
    save["main_ball"]["position"]["y"] = ball.GetPosition().y;
    save["main_ball"]["velocity"]["x"] = ball.GetVelocity().x;
    save["main_ball"]["velocity"]["y"] = ball.GetVelocity().y;
    
    // 多球状态
    json multiBallsJson = json::array();
    for (auto& mb : multiBalls) {
        json mbObj;
        mbObj["x"] = mb.GetPosition().x;
        mbObj["y"] = mb.GetPosition().y;
        mbObj["vx"] = mb.GetVelocity().x;
        mbObj["vy"] = mb.GetVelocity().y;
        multiBallsJson.push_back(mbObj);
    }
    save["multi_balls"] = multiBallsJson;
    
    // 挡板位置
    save["paddle_x"] = paddle.GetRect().x;
    
    // 砖块状态
    std::vector<int> brickStates;
    for (auto& b : bricks) {
        brickStates.push_back(b.IsActive() ? 1 : 0);
    }
    save["brick_states"] = brickStates;
    
    // 写入文件
    std::ofstream file(archiveSlotPath[slot]);
    if (file.is_open()) {
        file << save.dump(4);
        ShowErrorMessage("Game saved to slot " + std::to_string(slot));
    } else {
        ShowErrorMessage("Failed to save slot " + std::to_string(slot));
    }
    
    RefreshArchiveInfos(); // 更新显示
}

bool Game::LoadGameFromFile() {
    std::ifstream file(saveFilePath);
    if (!file.is_open()) return false;
    
    json save;
    try {
        file >> save;
    } catch (...) {
        ShowErrorMessage("Save file corrupted.");
        return false;
    }
    
    // 恢复基本数据
    score = save.value("score", 0);
    playerLife = save.value("player_life", startLives);
    currentLevel = save.value("current_level", 1);
    unlockedLevel = save.value("unlocked_level", 1);
    highScore = save.value("high_score", highScore);
    gameTime = save.value("game_time", 0.0f);
    ballServed = save.value("ball_served", false);
    
    // 恢复道具时间
    paddleLongerTime = save["powerup"].value("paddle_longer", 0.0f);
    slowBallTime = save["powerup"].value("slow_ball", 0.0f);
    
    // 加载当前关卡配置
    LoadLevelConfig(currentLevel);
    InitBricksFromConfig();
    
    // 恢复砖块状态
    if (save.contains("brick_states") && save["brick_states"].is_array()) {
        auto& states = save["brick_states"];
        if (states.size() == bricks.size()) {
            for (size_t i = 0; i < bricks.size(); ++i) {
                bricks[i].SetActive(states[i] == 1);
            }
        }
    }
    
    // 恢复主球状态
    if (save.contains("main_ball")) {
        float px = save["main_ball"]["position"]["x"];
        float py = save["main_ball"]["position"]["y"];
        float vx = save["main_ball"]["velocity"]["x"];
        float vy = save["main_ball"]["velocity"]["y"];
        ball.Reset({px, py}, {vx, vy});
    } else {
        // 如果没有，则放在挡板中间
        ball.Reset({paddle.GetRect().x + paddle.GetRect().width/2, paddle.GetRect().y - 10}, {0,0});
        ballServed = false;
    }
    
    // 恢复多球
    multiBalls.clear();
    if (save.contains("multi_balls") && save["multi_balls"].is_array()) {
        for (auto& mbJson : save["multi_balls"]) {
            Vector2 pos = {mbJson["x"], mbJson["y"]};
            Vector2 vel = {mbJson["vx"], mbJson["vy"]};
            multiBalls.emplace_back(pos, vel, ballRadius);
        }
    }
    
    // 恢复挡板位置
    if (save.contains("paddle_x")) {
        float px = save["paddle_x"];
        Rectangle rect = paddle.GetRect();
        rect.x = px;
        // 注意：Paddle 没有直接设置位置的方法，我们可以通过重新构造或添加 SetX 方法
        // 这里简单用反射？更简单：添加 Paddle::SetX(float x) 方法
        paddle.SetX(px);
    }
    
    // 应用道具视觉效果
    paddle.SetScale(paddleLongerTime > 0 ? 1.6f : 1.0f);
    
    // 重置其他标志
    showLevelText = true;
    levelStartTimer = 1.5f;
    isPaused = false;
    currentScreen = PLAYING;
    
    return true;
}

void Game::SaveGameToFile() {
    json save;
    // 基础信息
    save["score"] = score;
    save["player_life"] = playerLife;
    save["current_level"] = currentLevel;
    save["unlocked_level"] = unlockedLevel;
    save["high_score"] = highScore;
    save["game_time"] = gameTime;
    save["ball_served"] = ballServed;
    
    // 道具剩余时间
    save["powerup"]["paddle_longer"] = paddleLongerTime;
    save["powerup"]["slow_ball"] = slowBallTime;
    
    // 主球状态
    save["main_ball"]["position"]["x"] = ball.GetPosition().x;
    save["main_ball"]["position"]["y"] = ball.GetPosition().y;
    save["main_ball"]["velocity"]["x"] = ball.GetVelocity().x;   // 需要添加 GetVelocity 方法
    save["main_ball"]["velocity"]["y"] = ball.GetVelocity().y;
    
    // 多球状态
    json multiBallsJson = json::array();
    for (auto& mb : multiBalls) {
        json mbObj;
        mbObj["x"] = mb.GetPosition().x;
        mbObj["y"] = mb.GetPosition().y;
        mbObj["vx"] = mb.GetVelocity().x;
        mbObj["vy"] = mb.GetVelocity().y;
        multiBallsJson.push_back(mbObj);
    }
    save["multi_balls"] = multiBallsJson;
    
    // 挡板位置（可选，其实可由 ballServed 后重新计算，但保留）
    save["paddle_x"] = paddle.GetRect().x;
    
    // 砖块状态
    std::vector<int> brickStates;
    for (auto& b : bricks) {
        brickStates.push_back(b.IsActive() ? 1 : 0);
    }
    save["brick_states"] = brickStates;
    
    std::ofstream file(saveFilePath);
    if (file.is_open()) {
        file << save.dump(4);
        ShowErrorMessage("Game saved.");
    } else {
        ShowErrorMessage("Failed to save game!");
    }
}

void Game::DrawEditor() {
    ClearBackground(DARKGRAY);
    
    float startX = (screenWidth - (brickCols * (brickW + brickSpace))) / 2.0f;
    for (int r = 0; r < brickRows; ++r) {
        for (int c = 0; c < brickCols; ++c) {
            if (editLayout[r][c] == 0) continue;
            float x = startX + c * (brickW + brickSpace);
            float y = 60 + r * (brickH + brickSpace);
            DrawRectangleRec({x, y, brickW, brickH}, editSelectedColor);
            DrawRectangleLinesEx({x, y, brickW, brickH}, 1, WHITE);
        }
    }
    
    // 鼠标高亮
    int row = (int)((editMousePos.y - 60) / (brickH + brickSpace));
    int col = (int)((editMousePos.x - startX) / (brickW + brickSpace));
    if (row >= 0 && row < brickRows && col >= 0 && col < brickCols) {
        float x = startX + col * (brickW + brickSpace);
        float y = 60 + row * (brickH + brickSpace);
        DrawRectangleLinesEx({x, y, brickW, brickH}, 3, YELLOW);
    }
    
    // UI 文字 - 使用 TextFormat
    DrawText(TextFormat("EDIT MODE - Level %d", editingLevel), 20, 20, 30, WHITE);
    DrawText("Left Click: Add Brick | Right Click: Remove Brick", 20, 60, 20, LIGHTGRAY);
    DrawText("Press S to Save | ESC to Exit", 20, 90, 20, LIGHTGRAY);
    
    int brickCount = 0;
    for (auto& row : editLayout)
        for (int v : row) if (v) brickCount++;
    DrawText(TextFormat("Bricks: %d", brickCount), screenWidth - 150, 20, 20, YELLOW);
}

void Game::SaveEditedLevel() {
    // 加载原关卡配置（保留速度、颜色等元数据）
    std::string filename = "levels/level_" + std::to_string(editingLevel) + ".json";
    json config;
    
    std::ifstream inFile(filename);
    if (inFile.is_open()) {
        try {
            inFile >> config;
        } catch (...) {
            config = json::object(); // 空对象
        }
        inFile.close();
    }
    
    // 更新布局
    config["layout"] = editLayout;
    config["brick_rows"] = brickRows;
    config["brick_cols"] = brickCols;
    config["brick_width"] = brickW;
    config["brick_height"] = brickH;
    config["brick_spacing"] = brickSpace;
    
    // 如果原配置没有 ball_speed 等字段，设置默认值
    if (!config.contains("ball_speed_x")) config["ball_speed_x"] = 4.0f;
    if (!config.contains("ball_speed_y")) config["ball_speed_y"] = -4.0f;
    if (!config.contains("row_colors")) {
        // 根据行数生成默认颜色
        std::vector<std::string> colors = {"RED","ORANGE","YELLOW","GREEN","BLUE","PURPLE"};
        json colorsArr;
        for (int i = 0; i < brickRows; ++i) {
            colorsArr.push_back(colors[i % colors.size()]);
        }
        config["row_colors"] = colorsArr;
    }
    if (!config.contains("row_scores")) {
        json scoresArr;
        for (int i = 0; i < brickRows; ++i) {
            scoresArr.push_back(10);
        }
        config["row_scores"] = scoresArr;
    }
    
    // 写入文件
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << config.dump(4);
    } else {
        ShowErrorMessage("Cannot save level file: " + filename);
    }
}

void Game::UpdateEditor(float dt) {
    // 获取鼠标位置
    editMousePos = GetMousePosition();
    
    // 计算鼠标所在的砖块行列
    float startX = (screenWidth - (brickCols * (brickW + brickSpace))) / 2.0f;
    int col = (int)((editMousePos.x - startX) / (brickW + brickSpace));
    int row = (int)((editMousePos.y - 60) / (brickH + brickSpace)); // 砖块起始Y=60
    
    // 左键添加砖块，右键删除砖块
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (row >= 0 && row < brickRows && col >= 0 && col < brickCols) {
            editLayout[row][col] = 1;   // 添加砖块
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (row >= 0 && row < brickRows && col >= 0 && col < brickCols) {
            editLayout[row][col] = 0;   // 删除砖块
        }
    }
    
    // 保存按键（例如 F5 或 S 键）
    if (IsKeyPressed(KEY_S)) {
        SaveEditedLevel();
        ShowErrorMessage("Level " + std::to_string(editingLevel) + " saved.");
    }
    
    // 退出编辑模式（U 返回主菜单）
    if (IsKeyPressed(KEY_U)) {
        currentScreen = MENU;
    }
}

void Game::LoadLevelForEdit(int level) {
    // 先尝试加载关卡配置（复用 LoadLevelConfig，但不应用到游戏变量）
    std::string filename = "levels/level_" + std::to_string(level) + ".json";
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        // 如果文件不存在，创建一个默认的 10x5 全满布局
        editLayout.assign(5, std::vector<int>(10, 1));
        brickRows = 5;
        brickCols = 10;
        brickW = 70;
        brickH = 25;
        brickSpace = 5;
        return;
    }
    
    json config;
    try {
        file >> config;
    } catch (...) {
        editLayout.assign(5, std::vector<int>(10, 1));
        return;
    }
    
    // 读取布局
    if (config.contains("layout") && config["layout"].is_array()) {
        editLayout.clear();
        for (auto& row : config["layout"]) {
            std::vector<int> rowData;
            for (auto& cell : row) {
                rowData.push_back(cell.get<int>());
            }
            editLayout.push_back(rowData);
        }
    } else {
        // 默认全满
        int rows = config.value("brick_rows", 5);
        int cols = config.value("brick_cols", 10);
        editLayout.assign(rows, std::vector<int>(cols, 1));
    }
    
    // 读取砖块尺寸信息（用于绘制）
    brickRows = (int)editLayout.size();
    brickCols = (int)editLayout[0].size();
    brickW = config.value("brick_width", 70.0f);
    brickH = config.value("brick_height", 25.0f);
    brickSpace = config.value("brick_spacing", 5.0f);
}

void Game::EnterEditMode(int level) {
    editingLevel = level;
    LoadLevelForEdit(level);               // 加载关卡布局到 editLayout
    editSelectedColor = RED;               // 默认红色砖块
    editShowGrid = true;
    currentScreen = EDITOR;
}

void Game::CheckForSaveFile() {
    std::ifstream file(saveFilePath);
    hasSaveFile = file.is_open();
}

void Game::AskContinuePrompt() {
    if (!hasSaveFile) return;
    
    const char* msg = "Previous save found. Press Y to continue, N for new game.";
    DrawText(msg, screenWidth/2 - MeasureText(msg, 20)/2, screenHeight/2 + 50, 20, YELLOW);
    
    if (IsKeyPressed(KEY_Y)) {
        if (LoadGameFromFile()) {
            currentScreen = PLAYING;
            hasSaveFile = false;   // 避免重复询问
        }
    }
    if (IsKeyPressed(KEY_N)) {
        remove(saveFilePath.c_str());
        hasSaveFile = false;
    }
}

bool Game::LoadLevelConfig(int level) {
    std::string filename = "levels/level_" + std::to_string(level) + ".json";
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        ShowErrorMessage("Missing level file: " + filename);
        ApplyDefaultLevelConfig(level);
        return false;
    }
    
    json config;
    try {
        file >> config;
    } catch (json::parse_error& e) {
        ShowErrorMessage(std::string("JSON parse error: ") + e.what());
        ApplyDefaultLevelConfig(level);
        return false;
    }
    
    // 读取基础参数
    currentLevelBallSpeedX = config.value("ball_speed_x", 4.0f);
    currentLevelBallSpeedY = config.value("ball_speed_y", -4.0f);
    currentLevelConfigRows = config.value("brick_rows", 5);
    currentLevelConfigCols = config.value("brick_cols", 10);
    currentLevelBrickW = config.value("brick_width", 70.0f);
    currentLevelBrickH = config.value("brick_height", 25.0f);
    currentLevelBrickSpace = config.value("brick_spacing", 5.0f);
    
    // 读取布局
    currentLevelLayout.clear();
    if (config.contains("layout") && config["layout"].is_array()) {
        for (auto& row : config["layout"]) {
            std::vector<int> rowData;
            for (auto& cell : row) {
                rowData.push_back(cell.get<int>());
            }
            currentLevelLayout.push_back(rowData);
        }
        if (!currentLevelLayout.empty()) {
            currentLevelConfigRows = (int)currentLevelLayout.size();
            currentLevelConfigCols = (int)currentLevelLayout[0].size();
        }
    } else {
        // 没有 layout，生成全满
        currentLevelLayout.assign(currentLevelConfigRows, 
            std::vector<int>(currentLevelConfigCols, 1));
    }
    
    // 读取每行颜色
    currentLevelRowColors.clear();
    if (config.contains("row_colors") && config["row_colors"].is_array()) {
        for (auto& colStr : config["row_colors"]) {
            currentLevelRowColors.push_back(StringToColor(colStr.get<std::string>()));
        }
    }
    while (currentLevelRowColors.size() < (size_t)currentLevelConfigRows) {
        currentLevelRowColors.push_back(RED);
    }
    
    // 读取每行得分
    currentLevelRowScores.clear();
    if (config.contains("row_scores") && config["row_scores"].is_array()) {
        for (auto& sc : config["row_scores"]) {
            currentLevelRowScores.push_back(sc.get<int>());
        }
    }
    while (currentLevelRowScores.size() < (size_t)currentLevelConfigRows) {
        currentLevelRowScores.push_back(10);
    }
    
    // 应用到游戏全局变量
    ballSpeedX = currentLevelBallSpeedX;
    ballSpeedY = currentLevelBallSpeedY;
    brickRows = currentLevelConfigRows;
    brickCols = currentLevelConfigCols;
    brickW = currentLevelBrickW;
    brickH = currentLevelBrickH;
    brickSpace = currentLevelBrickSpace;
    
    return true;
}

void Game::ApplyDefaultLevelConfig(int level) {
    ShowErrorMessage("Using default level configuration.");
    // 默认递推逻辑
    brickRows = 5 + (level-1);
    if (brickRows > 8) brickRows = 8;
    brickCols = 10;
    brickW = 70;
    brickH = 25;
    brickSpace = 5;
    ballSpeedX = 4.0f + (level-1)*0.5f;
    ballSpeedY = -4.0f - (level-1)*0.5f;
    
    // 生成全满布局
    currentLevelLayout.clear();
    for (int r = 0; r < brickRows; ++r) {
        currentLevelLayout.push_back(std::vector<int>(brickCols, 1));
    }
    // 默认颜色/得分
    currentLevelRowColors.clear();
    currentLevelRowScores.clear();
    Color defaultColors[] = {RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE, SKYBLUE, PINK};
    for (int r = 0; r < brickRows; ++r) {
        currentLevelRowColors.push_back(defaultColors[r % 8]);
        currentLevelRowScores.push_back(10);
    }
}

void Game::InitBricksFromConfig() {
    bricks.clear();
    bricks.reserve(brickCols * brickRows);
    
    float startX = (screenWidth - (brickCols * (brickW + brickSpace))) / 2.0f;
    
    for (int r = 0; r < brickRows; ++r) {
        if (r >= (int)currentLevelLayout.size()) break;
        auto& rowLayout = currentLevelLayout[r];
        for (int c = 0; c < brickCols; ++c) {
            if (c >= (int)rowLayout.size()) break;
            if (rowLayout[c] == 0) continue;
            
            float x = startX + c * (brickW + brickSpace);
            float y = 60 + r * (brickH + brickSpace);
            Color col = (r < (int)currentLevelRowColors.size()) ? currentLevelRowColors[r] : RED;
            int scoreVal = (r < (int)currentLevelRowScores.size()) ? currentLevelRowScores[r] : 10;
            bricks.emplace_back(x, y, brickW, brickH, col, scoreVal);
        }
    }
}


void Game::ShowErrorMessage(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
    errorMessage = msg;
    errorMessageTimer = 3.0f;
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
    bricks.reserve(brickCols * brickRows);
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
    LoadLevelConfig(currentLevel);      // 从 JSON 加载当前关卡配置
    InitBricksFromConfig();
    
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
    SaveUnlockedLevel();                 // 保存解锁进度（可选）
    currentLevel++;
    
    // 尝试加载下一关
    if (!LoadLevelConfig(currentLevel)) {
        // 如果没有下一关，游戏胜利
        currentScreen = GAME_WIN;
        return;
    }
    InitBricksFromConfig();
    
    // 奖励一条命
    playerLife++;
    ballServed = false;
    showLevelText = true;
    levelStartTimer = 1.5f;
    // 重置道具效果
    paddleLongerTime = 0;
    slowBallTime = 0;
    paddle.SetScale(1.0f);
    multiBalls.clear();
    powerUps.clear();
}

void Game::SpawnParticles(Vector2 pos, Color c, int count) {
    for (int i = 0; i < count; i++)
        particles.emplace_back(pos, c);
}

void Game::UpdateParticles(float dt) {
    for (auto& p : particles) p.Update(dt);
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return !p.IsActive(); }),
                    particles.end());
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
        if (pu.GetPosition().y > screenHeight + 20)
            pu.Deactivate();
        if (pu.CheckCollision(paddle.GetRect())) {
            ApplyPowerUp(pu.GetType());
            pu.Deactivate();
            Vector2 center = { paddle.GetRect().x + paddle.GetRect().width/2,
                   paddle.GetRect().y + paddle.GetRect().height/2 };
            SpawnParticles(center, WHITE, 12);
        }
    }
    powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
                                  [](const PowerUp& pu) { return !pu.IsActive(); }),
                   powerUps.end());
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

// ===== 异步加载 =====
void Game::StartAsyncLoad() {
    if (loadingTexture) return;
    loadingTexture = true;
    loadComplete = false;
    loadEffectApplied = false;

    loadFuture = std::async(std::launch::async, [this]() {
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

// ===== 游戏更新（按键修复） =====
void Game::UpdateGamePlay(float dt) {
    if (currentScreen != PLAYING) return;

    if (IsKeyPressed(KEY_U)) {
        currentScreen = PAUSED;
        isPaused = true;
        return;
    }

    if (IsKeyPressed(KEY_S)) {
        OpenArchiveSaveMode();   // 这个函数会设置 isPaused = true 并切换屏幕
        return;
    }

    if (IsKeyPressed(KEY_P)) {
        isPaused = !isPaused;
        if (isPaused) return;
    }

    if (!isPaused) {
        if (IsKeyPressed(KEY_L)) {
            currentScreen = LEVEL_SCORE;
            return;
        }
        if (IsKeyPressed(KEY_K)) {
            StartAsyncLoad();
        }
    }

    if (isPaused) return;

    gameTime += dt;
    CheckAsyncLoad();

    if (showLevelText) {
        levelStartTimer -= dt;
        if (levelStartTimer <= 0) showLevelText = false;
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
                Vector2 cen = { b.GetRect().x + b.GetRect().width / 2,
                                b.GetRect().y + b.GetRect().height / 2 };
                SpawnParticles(cen, b.GetColor(), 10);
                SpawnPowerUp(cen);
            }
        }

        if (ball.CheckBottomDeath(screenHeight)) {
            playerLife--;
            ballServed = false;
        }
    }

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

    multiBalls.erase(std::remove_if(multiBalls.begin(), multiBalls.end(),
                                    [this](const Ball& b) {
                                        return b.GetPosition().y - 10 > screenHeight;
                                    }),
                     multiBalls.end());

    int act = 0;
for (auto& b : bricks) if (b.IsActive()) act++;
if (act == 0) {
    NextLevel();   // 自动加载下一关（内部会检测是否胜利）
    return;        // 本次更新结束，避免继续处理
}
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
    if (IsKeyPressed(KEY_S)) {
    OpenArchiveLoadMode();
}
    if (IsKeyPressed(KEY_E)) {
        StartEditMode();      // 进入编辑模式
    }
}

void Game::UpdatePause() {
    if (IsKeyPressed(KEY_P)) {
        currentScreen = PLAYING;
        isPaused = false;
    }
    if (IsKeyPressed(KEY_U)) {
        currentScreen = MENU;
        isPaused = false;
    }
    if (IsKeyPressed(KEY_R)) {
        ResetGame();
        isPaused = false;
    }
}

void Game::UpdateLevelScore() {
    if (IsKeyPressed(KEY_L) || IsKeyPressed(KEY_U)) {
        currentScreen = PLAYING;
        isPaused = false;
    }
    if (IsKeyPressed(KEY_ENTER)) GotoLevel(currentLevel);
    if (IsKeyPressed(KEY_RIGHT) && currentLevel < unlockedLevel) currentLevel++;
    if (IsKeyPressed(KEY_LEFT) && currentLevel > 1) currentLevel--;
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
    DrawText("[S] SAVE GAME", 20, 520, 20, LIGHTGRAY);
    DrawText("[E] EDITOR", 20, 560, 20, LIGHTGRAY);
}

void Game::DrawPauseMenu() {
    DrawGamePlay();
    DrawText("PAUSED", 330, 200, 60, ORANGE);
    DrawText("[P] RESUME", 330, 300, 30, WHITE);
    DrawText("[R] RESTART", 330, 340, 30, WHITE);
    DrawText("[U] MENU", 330, 380, 30, WHITE);
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

    DrawText("[P] PAUSE  [L] MENU  [K] LOAD  [ESC] MENU", 20, 550, 20, LIGHTGRAY);

    if (loadingTexture && !loadComplete)
        DrawText("Loading...", screenWidth/2 - 100, screenHeight/2 - 20, 40, WHITE);
    if (errorMessageTimer > 0) {
    DrawText(errorMessage.c_str(), 20, screenHeight - 40, 20, RED);
    errorMessageTimer -= GetFrameTime();
}
}

void Game::Run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateMusicStream(bgMusic);
        
        BeginDrawing();   // 外层统一开始渲染

        if (introState == 0) {
            DrawIntroAnimation();
        } else {
            switch (currentScreen) {
                case MENU:
                    UpdateMenu();
                    DrawMenu();               // 内部已包含 ClearBackground
                    AskContinuePrompt();       // 仅绘制文字，不操作 Begin/End
                    break;
                case PLAYING:
                    UpdateGamePlay(dt);
                    DrawGamePlay();
                    break;
                case PAUSED:
                    UpdatePause();
                    DrawPauseMenu();           // 内部会先调用 DrawGamePlay()
                    break;
                case LEVEL_SCORE:
                    UpdateLevelScore();
                    DrawLevelScoreScreen();
                    break;
                case GAME_OVER:
    ClearBackground(BLACK);
    DrawText("GAME OVER", screenWidth/2 - 100, screenHeight/2 - 30, 60, RED);
    DrawText("PRESS SPACE TO MENU", screenWidth/2 - 150, screenHeight/2 + 50, 30, LIGHTGRAY);
    if (IsKeyPressed(KEY_SPACE)) currentScreen = MENU;
    if (IsKeyPressed(KEY_SPACE)) currentScreen = MENU;
    if (IsKeyPressed(KEY_S)) OpenArchiveSaveMode();
    break;

               case GAME_WIN:
    ClearBackground(BLACK);
    DrawText("YOU WIN!", screenWidth/2 - 80, screenHeight/2 - 30, 60, GOLD);
    DrawText("PRESS SPACE TO MENU", screenWidth/2 - 150, screenHeight/2 + 50, 30, LIGHTGRAY);
    if (IsKeyPressed(KEY_SPACE)) currentScreen = MENU;
    if (IsKeyPressed(KEY_S)) OpenArchiveSaveMode();
    break;
               case ARCHIVE_SELECT:
    UpdateArchiveSelect();
    DrawArchiveSelect();
    break;

              case EDITOR:
    UpdateEditor(dt);
    DrawEditor();
    break;
            }
        }

        // 检查游戏结束（仅当在 PLAYING 状态时）
        if (currentScreen == PLAYING && IsGameOver()) {
            currentScreen = GAME_OVER;
        }

        EndDrawing();     // 外层统一结束渲染
    }
    CloseWindow();
}