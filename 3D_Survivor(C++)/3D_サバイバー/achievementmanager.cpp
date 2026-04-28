/*
	実績の全体管理：achievementmanager.h

	2026/01/13	hibiki sakuma
*/
#include "achievementmanager.h"
#include "direct3d.h"
#include <fstream>
#include <sstream>
#include <SpriteFont.h>
#include <SpriteBatch.h>
#include <windows.h> // MultiByteToWideCharを使うために必要

// 変数宣言
// スマートポインタで安全に管理
static std::unique_ptr<DirectX::SpriteBatch> g_FontBatch;
static std::unique_ptr<DirectX::SpriteFont>  g_Font;

// 内部関数
std::wstring Utf8ToWstring(const std::string& src) {
    if (src.empty()) return L"";

    // 1. 文字列の前後にある改行コードや空白を掃除する（トリミング）
    std::string cleanSrc = src;
    cleanSrc.erase(cleanSrc.find_last_not_of(" \n\r\t") + 1);
    cleanSrc.erase(0, cleanSrc.find_first_not_of(" \n\r\t"));

    // 2. BOM対策（以前のコードと同様）
    const char* pszSrc = cleanSrc.c_str();
    if (cleanSrc.size() >= 3 && (unsigned char)pszSrc[0] == 0xEF && (unsigned char)pszSrc[1] == 0xBB && (unsigned char)pszSrc[2] == 0xBF) {
        pszSrc += 3;
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, pszSrc, -1, nullptr, 0);
    if (size <= 0) return L"";

    std::wstring dest(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, pszSrc, -1, &dest[0], size);
    dest.resize(size - 1);
    return dest;
}

void AchievementManager::Save(){
    // 実行ファイルと同じ階層に "achievements.dat" を作成
    std::ofstream ofs("resuce/data/achievements.dat");
    if (!ofs) return; // ファイルが開けない

    for (const auto& ach : m_achievements) {
        // ID, 解除フラグ を一行ずつ書き出す
        // 例: KILL_100,1
        ofs << ach.id << "," << (ach.isUnlocked ? 1 : 0) << "\n";
    }
}

void AchievementManager::Load(){
    std::ifstream ifs("resuce/data/achievements.dat");
    if (!ifs) return; // ファイルが開けない

    std::string line;
    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        std::string id, unlockedStr;

        if (std::getline(ss, id, ',') && std::getline(ss, unlockedStr, ',')) {
            // m_achievementsの中から、ファイルから読んだIDと同じものを探す
            auto it = std::find_if(m_achievements.begin(), m_achievements.end(),
                [&](const Achievement& a) { return a.id == id; });

            if (it != m_achievements.end()) {
                it->isUnlocked = (unlockedStr == "1");
            }
        }
    }
}

void AchievementManager::LoadMasterData(){
    std::ifstream ifs("resuce/data/achievement_master.csv");
    if (!ifs) return;

    std::string line;
    std::getline(ifs, line); // ヘッダー読み飛ばし

    while (std::getline(ifs, line)) {
        if (line.empty()) continue; // 空行対策

        std::stringstream ss(line);
        std::string id, titleStr, type, targetStr;

        std::getline(ss, id, ',');
        std::getline(ss, titleStr, ',');
        std::getline(ss, type, ',');
        std::getline(ss, targetStr, ',');

        if (id.empty() || targetStr.empty()) continue;

        int targetValue = std::stoi(targetStr); // ターゲット実数
        std::wstring title = Utf8ToWstring(titleStr); // 実績名
        std::unique_ptr<AchievementCondition> cond; 

        // 条件判定クラスの生成
        if (type == "KILL") {
            // IDの中身を見てイベント名を決める
            std::string eventName = "ENEMY_KILLED"; // デフォルト（総数）
            if      (id.find("T1") != std::string::npos) eventName = "ENEMY_KILLED_T1";
            else if (id.find("T2") != std::string::npos) eventName = "ENEMY_KILLED_T2";
            else if (id.find("T3") != std::string::npos) eventName = "ENEMY_KILLED_T3";

            cond = std::make_unique<KillCondition>(eventName, targetValue);
        } else if (type == "TIME") {
            cond = std::make_unique<TimeCondition>(targetValue);
        } else if (type == "PLAYER") {
            cond = std::make_unique<ClearPlayerCondition>(targetValue);
        }
        
        if (id.empty()) continue; // 空ガード

        if (cond) {
            AddAchievement(id, title, std::move(cond));
        }
    }
}

void AchievementManager::LoadSaveData() {
    Load(); // 以前作成したLoad()を呼び出す
}

void AchievementManager::Update(double elapsed_time) {
    if (m_notifications.empty()) return;

    // 先頭の通知を取り出す
    auto& notify = m_notifications.front();

    // タイマーを減らす
    notify.displayTimer -= (float)elapsed_time;

    // 0秒になったら削除（次の通知があれば自動的にそれが先頭になる）
    if (notify.displayTimer <= 0.0f) {
        m_notifications.erase(m_notifications.begin());
    }
}

void AchievementManager::DrawNotification(){
    /*
    if (m_notifications.empty()) return;

    // 現在表示すべき通知
    const auto& notify = m_notifications.front();

    // 描画位置の計算（右下に配置）
    float x = (float)Direct3D_GetBackBufferWidth() * 0.8f;
    float y = (float)Direct3D_GetBackBufferHeight() * 0.8f;

    // 2. 文字を描画（SpriteFontを使用）
    // achieve.cpp で成功した「分離して描画」の形をとります
    if (g_Font && g_FontBatch) {
        g_FontBatch->Begin();

        // 縁取り（黒）
        XMVECTOR black = DirectX::Colors::Black;
        g_Font->DrawString(g_FontBatch.get(), L"【実績解除！】", XMFLOAT2(x + 1, y + 1), black);
        g_Font->DrawString(g_FontBatch.get(), notify.title.c_str(), XMFLOAT2(x + 1, y + 41), black);

        // 本文
        g_Font->DrawString(g_FontBatch.get(), L"【実績解除！】", XMFLOAT2(x, y), DirectX::Colors::Gold);
        g_Font->DrawString(g_FontBatch.get(), notify.title.c_str(), XMFLOAT2(x, y + 40), DirectX::Colors::White);

        g_FontBatch->End();
    }

    */
}

void AchievementManager::DrawNewlyUnlocked(float startX, float startY){
    if (m_newlyUnlocked.empty()) return;

    g_FontBatch->Begin();
    float y = startY;
    g_Font->DrawString(g_FontBatch.get(), L"【今回獲得した実績】", XMFLOAT2(startX, y), Colors::Gold);

    y += 40.0f;
    for (auto* ach : m_newlyUnlocked) {
        g_Font->DrawString(g_FontBatch.get(), ach->title.c_str(), XMFLOAT2(startX + 20, y), Colors::White);
        y += 30.0f;
    }
    g_FontBatch->End();
}
