/*
	マップの管理(オブジェクトなど)：map.h

	2025/11/10	hibiki sakuma
*/

#include "map.h"
#include "cube.h"
#include "texture.h"
#include "light.h"
#include "meshfield.h"
#include "player_camera.h"
#include "player.h"
#include "model.h"
#include "direct3d.h"
#include "resource_manager.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h> // ファイルの更新日時チェック用
#include <DirectXMath.h>
using namespace DirectX;

static constexpr float MAP_SIZE_HALF = 50.0f;
static constexpr float MAP_VIEW_MAX = 5.0f;
static constexpr float MAP_VIEW_MIN = -10.5f;

// デバック用
static AABB ex;

// 現在の固定配列を vector に変更
static std::vector<MapObject> g_MapObjectsVector;
static time_t g_LastFileTime = 0; // 最後に読み込んだ時のファイル更新時刻
const std::string CSV_PATH = "resource/data/map_data.csv";

// CSVを読み込む関数
void Map_LoadCSV(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		MessageBox(nullptr, "マップデータを読み込めませんでした。", "エラー", MB_OK);
		return;
	}
	g_MapObjectsVector.clear();

	std::string line;
	std::getline(file, line); // ヘッダー(kind,x,y,z)を読み飛ばす

	while (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string item;
		MapObject obj{};

		// カンマ区切りでパース
		std::getline(ss, item, ','); obj.KindId = std::stoi(item);
		std::getline(ss, item, ','); obj.Position.x = std::stof(item);
		std::getline(ss, item, ','); obj.Position.y = std::stof(item);
		std::getline(ss, item, ','); obj.Position.z = std::stof(item);

		// 当たり判定の再設定（既存のロジックを流用）
		if (obj.KindId == 1) {
			obj.Aabb_collision = Cube_GetAABB(obj.Position);
			obj.isOneWay = false;
		} else if (obj.KindId == 2) {
			obj.Aabb_collision = Model_GetAABB(Resouce_Manager_GetModelId(Stone), obj.Position);
			obj.isOneWay = false;
		} else if (obj.KindId == 3) {
			obj.Aabb_collision = Model_GetAABB(Resouce_Manager_GetModelId(Wood), obj.Position);
			obj.isOneWay = false;
		} else if (obj.KindId == 0) { 
			obj.Aabb_collision = MeshField_GetAABB(obj.Position); 
			obj.isOneWay = true;
		}
		g_MapObjectsVector.push_back(obj);
	}
}

void Map_Initialize(){
	Map_LoadCSV(CSV_PATH);
}

void Map_Finalize(){
}

void Map_UpDate(double elapsed_time){
	// デバッグ時のみライブリロードを有効にする
#if defined(DEBUG) || defined(_DEBUG)
	struct stat result;
	if (stat(CSV_PATH.c_str(), &result) == 0) {
		// ファイルの最終更新日時が前回読み込み時より新しければリロード
		if (g_LastFileTime != result.st_mtime) {
			Map_LoadCSV(CSV_PATH);
			g_LastFileTime = result.st_mtime;
			// デバッグログを出すと分かりやすい
			OutputDebugStringA("Map Data Reloaded!\n");
		}
	}
#endif
}

void Map_Draw(){
	XMMATRIX mtxWorld;

	for (const auto& o : g_MapObjectsVector) {
		// プレイヤーより高い床を描画しない
		if (o.Position.y >= GetPlayer()->GetPosition().y + MAP_VIEW_MIN && o.Position.y <= GetPlayer()->GetPosition().y + MAP_VIEW_MAX) {
			switch (o.KindId) {
			case 0: // メッシュフィールドの描画
				Light_SetSpecularWorld(Player_Camera_GetPosition(), 50.0f, { 0.3f,0.3f,0.3f,0.3f });
				MeshField_Draw({ o.Position.x, o.Position.y, o.Position.z });
				break;
			case 1: // キューブ
				mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
				Cube_Draw(Resouce_Manager_GetTexId(Cube), mtxWorld);
				break;
			case 2: // 石
				mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
				ModelDraw(Resouce_Manager_GetModelId(Stone), mtxWorld);
				break;
			case 3: // 木
				mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
				ModelDraw(Resouce_Manager_GetModelId(Wood), mtxWorld);
				break;
			default:
				MessageBox(nullptr, "マップの描画で問題が発生しています。", "エラー", MB_OK);
				break;
			}
		}
#if defined(DEBUG) || defined(_DEBUG) 
		Collision_DebugDraw(o.Aabb_collision);
		ex = ExpandAABB(o.Aabb_collision, 0.5f);
		Collision_DebugDraw(ex, { 0.0f,0.0f,1.0f,1.0f });
#endif
	}
}

int Map_GetObjectsCount(){
	return (int)g_MapObjectsVector.size(); // 全体のサイズ/一個のサイズ=個数
}

float Map_GetGroundHeight(float x, float z, float targetY) {
	float bestY = -FLT_MAX;
	float bestDiff = FLT_MAX;

	for (int i = 0; i < Map_GetObjectsCount(); i++) {
		const MapObject* obj = Map_GetObject(i);

		if (obj->KindId != 0) continue;

		const AABB& box = obj->Aabb_collision;

		// XZ一致
		if (x >= box.min.x && x <= box.max.x && z >= box.min.z && z <= box.max.z) {
			float floorY = box.max.y;

			float diff = fabsf(floorY - targetY);

			// 一番近い床を採用
			if (diff < bestDiff) {
				bestDiff = diff;
				bestY = floorY;
			}
		}
	}

	return bestY;
}

const MapObject* Map_GetObject(int index){
	return &g_MapObjectsVector[index];
}
