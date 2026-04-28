/*
	敵全体の制御：enemymanager.h

	2026/01/10	hibiki sakuma
*/
#include "enemymanager.h"
#include "player.h"
#include "model.h"
#include "meshfield.h"
#include <vector>
#include <algorithm>
#include "achievementmanager.h"
#include <DirectXCollision.h> // 視錐台判定に必要
#include "player_camera.h"
using namespace DirectX;
// 定数宣言
// エネミーの数関係
static constexpr int MAX_T1 = 500;  // T1の最大数
static constexpr int MAX_T2 = 500;  // T2の最大数
static constexpr int MAX_T3 = 500;  // T3の最大数
static constexpr int TOTAL_MAX = MAX_T1 + MAX_T2 + MAX_T3;
static constexpr int MAX_INSTANCES = 500; // 一度に描画できる最大数

// スポーン関係
static constexpr int SPAWN_MAX = 5; // 一度に出現する最大敵数
static constexpr double SEC60 = 60.0; // 60秒
static constexpr double COUNTUP_TIME = 20.0; // X秒ごとにエネミーの上限を増やす
static constexpr int START_ENEMY_LIMIT = 10; // 最初のエネミー上限
static constexpr double ENEMY_LIMIT_UP = 5.0; // 時間経過で増える上限の値
static constexpr float SPAWN_DISTANCE = 10.0f; // プレイヤーからの出現距離
static constexpr int ABSOLUTE_MAX = 1500; // 配列の物理的な限界サイズ
static constexpr float CAMERA_FRONT_DISTANCE = 15.0f; // カメラを使用した手前のスポーン位置
static constexpr float CAMERA_PALM_DISTANCE = 100.0f; // カメラを使用した手奥のスポーン位置
static constexpr float DESSPAWNDIST = 100.0f; // プレイヤーからのデスポーン距離
static constexpr float SPAWN_T1_PERCENT = 30;
static constexpr float SPAWN_T2_PERCENT = SPAWN_T1_PERCENT + 50;

// 内部変数（staticで隠蔽）
static Enemy_T1 g_PoolT1[MAX_T1];
static Enemy_T2 g_PoolT2[MAX_T2];
static Enemy_T3 g_PoolT3[MAX_T3];
static double g_SpawnTimer = 0.0;
static double g_GameTime = 0.0; // 経過時間（難易度調整用）
static int g_CurrentMaxEnemies = 10; // 最初の同時存在上限（例：10体）
static int g_FrameCounter = 0; // 毎フレーム加算用

void EnemyManager::Initialize() {
    // 各クラスのモデルロード
    Enemy_T1::LoadModel();
    Enemy_T2::LoadModel(); 
    Enemy_T3::LoadModel();
    // 全て非アクティブ化
    for (int i = 0; i < MAX_T1; i++) g_PoolT1[i].Deactivate();
    for (int i = 0; i < MAX_T2; i++) g_PoolT2[i].Deactivate();
    for (int i = 0; i < MAX_T3; i++) g_PoolT3[i].Deactivate();
    // 動的変数の初期化
    g_SpawnTimer = 0.0;
    g_GameTime = 0.0;
}

void EnemyManager::Finalize() {
    Enemy_T1::UnloadModel();
    Enemy_T2::UnloadModel();
    Enemy_T3::UnloadModel();
}

void EnemyManager::Update(double elapsed_time) {
    g_GameTime += elapsed_time;

    // 1. 同時存在上限の更新
    g_CurrentMaxEnemies = START_ENEMY_LIMIT + (int)(g_GameTime / COUNTUP_TIME * ENEMY_LIMIT_UP);
    if (g_CurrentMaxEnemies > TOTAL_MAX) g_CurrentMaxEnemies = TOTAL_MAX;

    // 2. カメラの視錐台（見える空間）を作成
    XMMATRIX view = XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
    XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetPerspectiveMatrix());

    // カメラのプロジェクション行列から視錐台基本形を作成
    BoundingFrustum frustum(proj);
    // ビュー行列の逆行列を使って、カメラの位置・向きに合わせる
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    frustum.Transform(frustum, invView);
    // これにより、画面の少し外側までエネミーの存在を許容します
    frustum.Origin.x *= 1.0f; // 中心はそのまま
    int activeCount = 0;

    // 各プールの更新と「画面外消去」
    // T1
    for (int i = 0; i < MAX_T1; i++) {
        if (!g_PoolT1[i].IsActive()) continue;
    
        // --- 1. 画面外判定（デスポーン） ---
        XMVECTOR posV = XMLoadFloat3(&g_PoolT1[i].GetPosition());
        XMVECTOR playerPosV = XMLoadFloat3(&GetPlayer()->GetPosition());
        float distSq = XMVectorGetX(XMVector3LengthSq(posV - playerPosV));

        if (frustum.Contains(posV) == DISJOINT && distSq > (DESSPAWNDIST * DESSPAWNDIST)) {
            g_PoolT1[i].Deactivate();
            continue;
        }

        // --- 2. 通常更新 ---
        g_PoolT1[i].Update(elapsed_time);
    
        // --- 3. 撃破判定（実績加算） ---
        if (g_PoolT1[i].GetHP() <= 0) {
            // 実績システムにタイプ番号を渡す
            AchievementManager::Instance().OnNotify("ENEMY_KILLED_T1", 1);
            g_PoolT1[i].Deactivate(); // 撃破して消去
            activeCount--;        // 生存数を減らす（即座にスポーンさせるため）
        }
        else {
            activeCount++;
        }
    }

    // T2
    for (int i = 0; i < MAX_T2; i++) {
        if (!g_PoolT2[i].IsActive()) continue;

        // --- 1. 画面外判定（デスポーン） ---
        XMVECTOR posV = XMLoadFloat3(&g_PoolT2[i].GetPosition());
        XMVECTOR playerPosV = XMLoadFloat3(&GetPlayer()->GetPosition());
        float distSq = XMVectorGetX(XMVector3LengthSq(posV - playerPosV));
        
        if (frustum.Contains(posV) == DISJOINT && distSq > (DESSPAWNDIST * DESSPAWNDIST)) {
            g_PoolT2[i].Deactivate();
            continue;
        }

        // --- 2. 通常更新 ---
        g_PoolT2[i].Update(elapsed_time);

        // --- 3. 撃破判定（実績加算） ---
        if (g_PoolT2[i].GetHP() <= 0) {
            // 実績システムにタイプ番号を渡す
            AchievementManager::Instance().OnNotify("ENEMY_KILLED_T2", 1);
            g_PoolT2[i].Deactivate(); // 撃破して消去
            activeCount--;        // 生存数を減らす（即座にスポーンさせるため）
        }
        else {
            activeCount++;
        }
    }

    // T3
    for (int i = 0; i < MAX_T3; i++) {
        if (!g_PoolT3[i].IsActive()) continue;

        // --- 1. 画面外判定（デスポーン） ---
        XMVECTOR posV = XMLoadFloat3(&g_PoolT3[i].GetPosition());
        XMVECTOR playerPosV = XMLoadFloat3(&GetPlayer()->GetPosition());
        float distSq = XMVectorGetX(XMVector3LengthSq(posV - playerPosV));

        if (frustum.Contains(posV) == DISJOINT && distSq > (DESSPAWNDIST * DESSPAWNDIST)) {
            g_PoolT3[i].Deactivate();
            continue;
        }

        // --- 2. 通常更新 ---
        g_PoolT3[i].Update(elapsed_time);

        // --- 3. 撃破判定（実績加算） ---
        if (g_PoolT3[i].GetHP() <= 0) {
            // 実績システムにタイプ番号を渡す
            AchievementManager::Instance().OnNotify("ENEMY_KILLED_T3", 1);
            g_PoolT3[i].Deactivate(); // 撃破して消去
            activeCount--;        // 生存数を減らす（即座にスポーンさせるため）
        }
        else {
            activeCount++;
        }
    }
    
    // 3. 上限まで補充（スポーン）
    // 「今足りない分」を計算して、一気にスポーンさせる
    int need = g_CurrentMaxEnemies - activeCount;
    for (int i = 0; i < need; i++) {
        XMFLOAT3 spawnPos = CalculateSpawnPosition();
        if (spawnPos.y < 0.0f) continue; // マップ外ならスキップして次を試す

        // 種類抽選ロジック（既存）
        int dice = rand() % 100;
        if (dice < SPAWN_T1_PERCENT) {
            for (auto& e : g_PoolT1) if (!e.IsActive()) { e.Activate(spawnPos); break; }
        }
        else if (dice < (SPAWN_T2_PERCENT)) {
            for (auto& e : g_PoolT2) if (!e.IsActive()) { e.Activate(spawnPos); break; }
        }
        else {
            for (auto& e : g_PoolT3) if (!e.IsActive()) { e.Activate(spawnPos); break; }
        }
    }
}

void EnemyManager::Draw() {
    // 500体分入る大きな作業用の箱を用意（使い回す）
    static DirectX::XMMATRIX matrices[MAX_INSTANCES];

    // 描画用の視錐台を作成
    XMMATRIX view = XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
    XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetPerspectiveMatrix());
    BoundingFrustum frustum(proj);
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    frustum.Transform(frustum, invView);

    auto drawPool = [&](auto& pool, int size) {
        int count = 0;
        MODEL* pModel = nullptr;
        for (int i = 0; i < size; i++) {
            if (!pool[i].IsActive()) continue;

            // ★ここで描画カリング
            XMVECTOR pos = XMLoadFloat3(&pool[i].GetPosition());
            if (frustum.Contains(pos) == DISJOINT) continue; // 映っていないなら行列を積まない

            if (!pModel) pModel = pool[i].GetModel();
            if (count < MAX_INSTANCES) {
                matrices[count++] = XMMatrixTranspose(pool[i].GetWorldMatrix());
            }
        }
        if (count > 0) ModelDrawInstanced(pModel, matrices, count);
        };

    drawPool(g_PoolT1, MAX_T1);
    drawPool(g_PoolT2, MAX_T2);
    drawPool(g_PoolT3, MAX_T3);

}

int EnemyManager::GetMaxCount() { return TOTAL_MAX; }

Enemy* EnemyManager::GetEnemy(int index) {
    if (index < MAX_T1) return &g_PoolT1[index];
    index -= MAX_T1;
    if (index < MAX_T2) return &g_PoolT2[index];
    index -= MAX_T2;
    if (index < MAX_T3) return &g_PoolT3[index];
    return nullptr;
}


DirectX::XMFLOAT3 EnemyManager::CalculateSpawnPosition() {
    XMFLOAT3 playerPos = GetPlayer()->GetPosition();
    XMFLOAT3 cameraFront = Player_Camera_GetFront();

    // カメラの正面方向の角度
    float baseAngle = atan2f(cameraFront.x, cameraFront.z);

    // カメラの視界
    float offsetAngle = ((float)rand() / RAND_MAX - 0.5f) * XM_PI; // 左右90度
    float finalAngle = baseAngle + offsetAngle;

    // カメラの奥方向（150m先など）
    float dist = CAMERA_FRONT_DISTANCE + ((float)rand() / RAND_MAX * CAMERA_PALM_DISTANCE);

    DirectX::XMFLOAT3 pos{};
    pos.x = playerPos.x + sinf(finalAngle) * dist;
    pos.y = 1.0;
    pos.z = playerPos.z + cosf(finalAngle) * dist;

    // マップ境界チェック
    AABB mapBounds = MeshField_GetAABB();
    float margin = 5.0f;
    if (pos.x < mapBounds.min.x + margin || pos.x > mapBounds.max.x - margin ||
        pos.z < mapBounds.min.z + margin || pos.z > mapBounds.max.z - margin)
    {
        pos.y = -999.0f; // 確実に画面外へ出す
        return pos;
    }

    return pos;
}