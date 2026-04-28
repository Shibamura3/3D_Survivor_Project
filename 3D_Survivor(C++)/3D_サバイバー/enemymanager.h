/*
	敵全体の制御：enemymanager.h

	2026/01/10	hibiki sakuma
*/
#ifndef ENEMYMANAGER_H
#define ENEMYMANAGER_H

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "enemy.h"
#include "enemy_t1.h"
#include "enemy_t2.h" 
#include "enemy_t3.h" 
// 前方宣言（ヘッダーを軽くするため）
class Enemy;

class EnemyManager {
public:
    // 初期化・解放
    static void Initialize();
    static void Finalize();

    // 更新・描画（Game.cppから毎フレーム呼ぶ）
    static void Update(double elapsed_time);
    static void Draw();

    // 当たり判定用に全種類取得できるようにする
    static int GetMaxCount();
    static Enemy* GetEnemy(int index);

private:
    // 内部的な計算（外部には隠す）
    static DirectX::XMFLOAT3 CalculateSpawnPosition();
};

#endif // !ENEMYMANAGER_H
