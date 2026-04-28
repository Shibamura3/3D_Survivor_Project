/*
	当たり判定の制御：collision.h

	2025/11/05	hibiki sakuma
*/

#ifndef COLLISION_H
#define COLLISION_H

#include <d3d11.h>
#include <DirectXMath.h>

struct Sphere { // 球の定義
	DirectX::XMFLOAT3 center_position; // 中心座標
	float radius; // 半径
};

struct Capsule{ // カプセル型
	DirectX::XMFLOAT3 start; // 下端（足元）
	DirectX::XMFLOAT3 end;   // 上端（頭）
	float radius;
};

struct Circle { // 円の定義
	DirectX::XMFLOAT2 center_position; // 中心座標
	float radius; // 半径
};

struct Box { // 四角の定義
	//画像は左上座標と幅高さなので改変が必要かも
	DirectX::XMFLOAT2 center_position;
	float hafe_width;
	float hafe_height;
};

struct Hit { // 当たった面の法線を取得
	bool isHit;
	DirectX::XMFLOAT3 normal;
	float depth;
};

struct AABB { // 三次元の箱
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;

	DirectX::XMFLOAT3 GetCenter() const {
		DirectX::XMFLOAT3 center;
		DirectX::XMFLOAT3 half = GetHalf();
		center.x = min.x + half.x * 0.5f; // ベクトルの長さ÷２
		center.y = min.y + half.y * 0.5f;
		center.z = min.z + half.z * 0.5f;

		return center;
	}

	DirectX::XMFLOAT3 GetHalf() const {
		DirectX::XMFLOAT3 half;
		half.x = (max.x - min.x) * 0.5f;
		half.y = (max.y - min.y) * 0.5f;
		half.z = (max.z - min.z) * 0.5f;

		return half;
	}
};

struct OBB
{
	DirectX::XMFLOAT3 center;   // 中心
	DirectX::XMFLOAT3 extents;  // 各軸の半サイズ (x,y,z)
	DirectX::XMFLOAT3 axis[3];  // ローカルX,Y,Z（正規化）
};

// 球体の衝突判定
bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b); // 球と球の当たり判定
bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point); // 球と点の当たり判定
// 2Dの衝突判定
bool Collision_IsOverlapCircle(const Circle a, const Circle b);
bool Collision_IsOverlapBox(const Box a, const Box b); 
// AABB同士の衝突判定
bool Collision_IsOverlapAABB(const AABB& a, const AABB& b); // 3DBOXコリジョン
Hit Collision_IsHitAABB(const AABB& a, const AABB& b); // aにbのどの方向にぶつかったか
// Raycast
bool PointInsideAABB(const DirectX::XMFLOAT3& p, const AABB& b);
bool Collision_RaycastAABB(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const AABB& box, float& outT, DirectX::XMFLOAT3& outNormal);
bool Collision_RaycastSphere(const DirectX::XMFLOAT3& start, DirectX::XMFLOAT3& end, const Sphere& sphere, float& outT, DirectX::XMFLOAT3& outNormal);
bool Collision_RaycastCapsule(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const Capsule& capsule, float& outT, DirectX::XMFLOAT3& outNormal);
// Capsule
bool Collision_IsOverlapCapsulevsAABB(const Capsule& c, const AABB& a, DirectX::XMFLOAT3& outNormal, float& outDepth);//スプライトを使用するのでデバックの際はスプライトの下
Hit Collision_IsHitCapsulevsAABB(const Capsule& c, const AABB& a);
Hit Collision_IsHitCapsuleVsSphere(const Capsule& cap, const DirectX::XMFLOAT3& sphereCenter, float sphereRadius);
//Swept
AABB ExpandAABB(const AABB& aabb, float r);
bool Collision_SweptCapsuleVsAABB(const Capsule& oldCap, const Capsule& newCap, const AABB& box, float& outT, DirectX::XMFLOAT3& outNormal);

// デバック関連
void Collision_DebudInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Collision_DebudFinalize();

void Collision_DebugDraw(const  Circle& circle, const DirectX::XMFLOAT4 color = { 1.0f,1.0f,0.0f,1.0f });
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4 color = { 1.0f,0.0f,0.0f,1.0f });

// 三次元用
void Collision_Debud3DInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Collision_Debud3DFinalize();
void Collision_DebugDraw(const AABB& aabb, const DirectX::XMFLOAT4 color = {1.0f,0.0f,0.0f,1.0f});
void Collision_DebugDraw(const Sphere& s, const DirectX::XMFLOAT4 color = { 1.0f,0.0f,0.0f,1.0f });
void Collision_DebugDraw(const Capsule& c, const DirectX::XMFLOAT4 color = { 1.0f,0.0f,0.0f,1.0f });
void Collision_DebugDraw_Execute();
#endif // ! COLLISION_H

