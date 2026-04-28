/*
	実績の全体管理：achievementmanager.h

	2026/01/13	hibiki sakuma
*/
#ifndef ACHIEVEMENTMANAGWE_H
#define ACHIEVEMENTMANAGWE_H

#include <iostream>
#include <memory> // std::unique_ptr を使う
#include <string>
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;

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

// ポップアップ用構造体
struct AchievementNotify {
    std::wstring title;
    float displayTimer{}; // 表示されている残り時間
    bool isActive{};      // 現在表示中か
};

class AchievementManager {
private:
    AchievementManager() = default;

    // 1. 全実績を保持するリスト（実体）
    // unique_ptrを含む構造体なので、vectorがメモリ管理を自動で行ってくれます
    std::vector<Achievement> m_achievements; // 本命　ずっと記憶

    // 2. 今回解除した実績を一時的に指し示すリスト（ポインタ）
    // 実体は m_achievements にあるので、こちらはポインタで管理します
    std::vector<Achievement*> m_newlyUnlocked; // 付箋　今回のみ

    std::vector<AchievementNotify> m_notifications; // 通知待ちリスト

    void Save();
    void Load();
    // 外部から個別に呼ばれる必要がないものは隠す
    void LoadMasterData();
    void LoadSaveData();
public:
    // ゲームの初期化処理からこれを一度だけ呼ぶ
    void Initialize() {
        LoadMasterData(); // 内部でマスターを読み込む
        LoadSaveData();   // 内部でセーブ状況を反映する
    }

    void Update(double elapsed_time); // 通知のタイマー管理用
    void DrawNotification();      // 画面右上に描画する用
    void DrawNewlyUnlocked(float startX, float startY); // result画面での表示

    void ClearNewlyUnlocked() {
        m_newlyUnlocked.clear();
    }

    static AchievementManager& Instance() {
        static AchievementManager instance;
        return instance;
    }

    // イベント通知
    void OnNotify(const std::string& eventName, int value = 1) {
        for (auto& ach : m_achievements) {
            if (ach.isUnlocked) continue;

            if (ach.condition && ach.condition->IsClear(eventName, value)) {
                ach.isUnlocked = true;
                m_newlyUnlocked.push_back(&ach);

                // 通知リストに登録
                AchievementNotify notify;
                notify.title = ach.title;
                notify.displayTimer = 3.0f; // 3秒間
                notify.isActive = true;
                m_notifications.push_back(notify);

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

#endif // !ACHIEVEMENTMANAGWE_H