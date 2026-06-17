/*
	実績の全体管理：achievementmanager.h

	2026/01/13	hibiki sakuma
*/
#ifndef ACHIEVEMENTMANAGER_H
#define ACHIEVEMENTMANAGER_H

#include <memory> // std::unique_ptr を使う
#include <string>
#include <vector>

// 判定の抽象クラス
class AchievementCondition {
public:
    virtual ~AchievementCondition() = default;
    // 解除条件を満たしたかチェックする関数
    virtual bool IsClear(const std::string& eventName, int value) = 0;
};

// 撃破数判定：累計加算していく
class KillCondition : public AchievementCondition {
    std::string targetEventName; // 例: "ENEMY_KILLED_T1"
    int targetCount;
    int current = 0;
public:
    // コンストラクタでイベント名（タイプ）を指定できるようにする
    KillCondition(std::string event, int t) : targetEventName(event), targetCount(t) {}

    bool IsClear(const std::string& eventName, int value) override {
        if (eventName == targetEventName) {
            current += value;
            return current >= targetCount;
        }
        return false;
    }
};

// 生存時間判定：送られてきた値が目標を超えているか
class TimeCondition : public AchievementCondition {
    int targetSeconds;
public:
    TimeCondition(int sec) : targetSeconds(sec) {}
    bool IsClear(const std::string& eventName, int value) override {
        if (eventName == "SESSION_TIME") {
            return value >= targetSeconds;
        }
        return false;
    }
};

// クリアキャラクター判定：どのプレイヤーがクリアしたか
class ClearPlayerCondition : public AchievementCondition {
    int targetPlayerId;
public:
    ClearPlayerCondition(int id) : targetPlayerId(id) {}
    bool IsClear(const std::string& eventName, int value) override {
        if (eventName == "CLEAR_PLAYER") {
            return (value == targetPlayerId); // ターゲットと対象のIDが同じなら
        }
        return false;
    }
};

// 実績情報のデータ構造体
struct Achievement {
    std::string id;
    std::wstring title;
    // 判定ロジックを「戦略（Strategy）」として保持
    std::unique_ptr<AchievementCondition> condition;
    bool isUnlocked = false;
};

class AchievementManager {
private:
    AchievementManager() = default;

    // 全実績の状態（解除状況を含む）を保持する
    std::vector<Achievement> m_achievements; 

    void Save();
    void Load();
    // 外部から個別に呼ばれる必要がないものは隠す
    void LoadMasterData();
public:
    // ゲームの初期化処理からこれを一度だけ呼ぶ
    void Initialize() {
        LoadMasterData(); // 内部でマスターを読み込む
        Load();   // 内部でセーブ状況を反映する
    }

    static AchievementManager& Instance() {
        static AchievementManager instance;
        return instance;
    }

    // イベント通知
    void OnNotify(const std::string& eventName, int value = 1) {

        for (auto& ach : m_achievements)
        {
            if (ach.isUnlocked) continue;

            if (ach.condition && ach.condition->IsClear(eventName, value))
            {
                ach.isUnlocked = true;
                Save(); // セーブ
            }
        }

    }

    // 実績の追加用（初期化時などに呼ぶ）
    void AddAchievement(std::string id, std::wstring title, std::unique_ptr<AchievementCondition> cond) {
        // 直接 vector の末尾で生成することで、メモリの移動回数を減らす
        m_achievements.emplace_back(Achievement{ id, title, std::move(cond), false });
    }

    // 実績リスト全体への参照を返す（読み取り専用）
    const std::vector<Achievement>& GetAll() const {
        return m_achievements;
    }
};

#endif // !ACHIEVEMENTMANAGER_H