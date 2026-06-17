/*
	実績の全体管理：achievementmanager.h

	2026/01/13	hibiki sakuma
*/
#include "achievementmanager.h"
#include <fstream>
#include <sstream>
#include <windows.h> 

// 内部関数
std::wstring Utf8ToWstring(const std::string& src) {
    if (src.empty()) return L"";

    // トリミング
    std::string cleanSrc = src;
    cleanSrc.erase(cleanSrc.find_last_not_of(" \n\r\t") + 1);
    cleanSrc.erase(0, cleanSrc.find_first_not_of(" \n\r\t"));

    // BOM対策
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
    std::ofstream ofs("resource/data/achievements.dat");
    if (!ofs) return; // ファイルが開けない

    for (const auto& ach : m_achievements) {
        ofs << ach.id << "," << (ach.isUnlocked ? 1 : 0) << "\n";
    }
}

void AchievementManager::Load(){
    std::ifstream ifs("resource/data/achievements.dat");
    if (!ifs) return; // ファイルが開けない

    std::string line;
    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        std::string id, unlockedStr;

        if (std::getline(ss, id, ',') && std::getline(ss, unlockedStr, ',')) {
            auto it = std::find_if(m_achievements.begin(), m_achievements.end(),
                [&](const Achievement& a) { return a.id == id; });

            if (it != m_achievements.end()) {
                it->isUnlocked = (unlockedStr == "1");
            }
        }
    }
}

void AchievementManager::LoadMasterData(){
    std::ifstream ifs("resource/data/achievement_master.csv");
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