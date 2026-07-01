/*
	マップの管理(オブジェクトなど)：map.h

	2025/11/10	hibiki sakuma
*/
// 別のマップを移動させるなどの処理もココの情報の書き換え制御になる
#ifndef MAP_H
#define MAP_H

#include <DirectXMath.h>
#include "collision.h"

void Map_Initialize();
void Map_Finalize();
	 
void Map_UpDate(double elapsed_time); // 移動床などの動くものがある際は必要
void Map_Draw(); 

int Map_GetObjectsCount(); // 構造体の個数を返す→マップ内のオブジェクト数を返す
float Map_GetGroundHeight(float x, float z, float targetY); // 現在の床情報の取得

struct MapObject {
	int KindId; // 種類の番号
	DirectX::XMFLOAT3 Position;
	AABB Aabb_collision; 
	bool isOneWay;
};

const MapObject* Map_GetObject(int index); // 生成した構造体のindex番を返す

#endif // !MAP_H
