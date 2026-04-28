/*
	当たり判定の制御：collision.h

	2025/11/05	hibiki sakuma
*/

#include "collision.h"
#include "direct3d.h"
#include "texture.h"
#include "shader.h"
#include "shader3d_unlit.h"
#include "cube.h"
#include <algorithm>
#include <DirectXMath.h>
using namespace DirectX;


bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b)
{
	XMVECTOR ac = XMLoadFloat3(&a.center_position);
	XMVECTOR bc = XMLoadFloat3(&b.center_position);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac);

	return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
}

bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point){
	XMVECTOR ac = XMLoadFloat3(&a.center_position);
	XMVECTOR bc = XMLoadFloat3(&point);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac);

	return a.radius * a.radius > XMVectorGetX(lsq); // 長さの二乗なので半径も二乗する

}

bool Collision_IsOverlapCircle(const Circle a, const Circle b) {
	
	float xl = b.center_position.x - a.center_position.x;
	float yl = b.center_position.y - a.center_position.y;
	//ture 当たり
	return (xl * xl + yl * yl) < (a.radius + b.radius) * (a.radius + b.radius);
}

bool Collision_IsOverlapBox(const Box a, const Box b){
	float a_t = a.center_position.y - a.hafe_height;
	float a_b = a.center_position.y + a.hafe_height;
	float a_l = a.center_position.x - a.hafe_width;
	float a_r = a.center_position.x + a.hafe_width;
	float b_t = b.center_position.y - b.hafe_height;
	float b_b = b.center_position.y + b.hafe_height;
	float b_l = b.center_position.x - b.hafe_width;
	float b_r = b.center_position.x + b.hafe_width;

	return a_l<b_r && a_r>b_l && a_t<b_b && a_b>b_t;
}

bool Collision_IsOverlapAABB(const AABB& a, const AABB& b){
	return a.min.x < b.max.x
		&& a.max.x > b.min.x
		&& a.min.y < b.max.y
		&& a.max.y > b.min.y
		&& a.min.z < b.max.z
		&& a.max.z > b.min.z;
}

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
	Hit hit{};
	// 衝突結果の保存
	hit.isHit = Collision_IsOverlapAABB(a, b);
	
	if (!hit.isHit) { // 当たってなかった時
		return hit;
	}

	// 各軸の深度を調べる
	float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

	bool isShallowX = false;
	bool isShallowY = false;
	bool isShallowZ = false;

	// 一番浅い深度を調べる
	if (xdepth > ydepth) { 
		if (ydepth > zdepth) {
			// zの軸
			isShallowZ = true;
		} else {
			// yの軸
			isShallowY = true;
		}
	} else {
		if (zdepth > xdepth) {
			// xの軸
			isShallowX = true;
		} else {
			// zの軸
			isShallowZ = true;
		}
	}

	// +-のどちらから当たったか
	XMFLOAT3 a_center = a.GetCenter();
	XMFLOAT3 b_center = b.GetCenter();
	XMVECTOR normal = XMLoadFloat3(&b_center) - XMLoadFloat3(&a_center); // b～aの位置関係を計算し+-のついた数値を得る

	if (isShallowX) {
		normal = XMVector3Normalize(normal * XMVECTOR{ 1.0f,0.0f,0.0f });
	} else if (isShallowY) {
		normal = XMVector3Normalize(normal * XMVECTOR{ 0.0f,1.0f,0.0f });
	} else if (isShallowZ) {
		normal = XMVector3Normalize(normal * XMVECTOR{ 0.0f,0.0f,1.0f });
	}
	XMStoreFloat3(&hit.normal, normal);
	return hit;
}
bool PointInsideAABB(const XMFLOAT3& p, const AABB& b){

	return  p.x >= b.min.x && p.x <= b.max.x &&
			p.y >= b.min.y && p.y <= b.max.y &&
			p.z >= b.min.z && p.z <= b.max.z;
}
bool Collision_RaycastAABB(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const AABB& box, float& outT, DirectX::XMFLOAT3& outNormal){
	XMVECTOR vStart = XMLoadFloat3(&start);
	XMVECTOR vEnd = XMLoadFloat3(&end);
	XMVECTOR vDir = vEnd - vStart;

	XMFLOAT3 dir;
	XMStoreFloat3(&dir, vDir);

	float tMin = 0.0f;
	float tMax = 1.0f;
	outNormal = { 0,0,0 };

	auto checkAxis = [&](float start, float dir, float min, float max, int axis) -> bool
		{
			if (fabsf(dir) < 1e-6f) {
				// 平行：外にいたら当たらない
				return (start >= min && start <= max);
			}

			float inv = 1.0f / dir;
			float t1 = (min - start) * inv;
			float t2 = (max - start) * inv;

			float enter = std::min(t1, t2);
			float exit  = std::max(t1, t2);

			if (enter > tMin) {
				tMin = enter;

				outNormal = { 0,0,0 };
				if (axis == 0) outNormal.x = (t1 < t2) ? -1.0f : 1.0f;
				if (axis == 1) outNormal.y = (t1 < t2) ? -1.0f : 1.0f;
				if (axis == 2) outNormal.z = (t1 < t2) ? -1.0f : 1.0f;
			}

			tMax = std::min(tMax, exit);
			return tMin <= tMax;
		};
	// X軸
	if (!checkAxis(start.x, dir.x, box.min.x, box.max.x, 0)) return false;
	// Y軸
	if (!checkAxis(start.y, dir.y, box.min.y, box.max.y, 1)) return false;
	// Z軸
	if (!checkAxis(start.z, dir.z, box.min.z, box.max.z, 2)) return false;

	outT = tMin;
	return true;
}
bool Collision_RaycastSphere(const DirectX::XMFLOAT3& start, DirectX::XMFLOAT3& end, const Sphere& sphere, float& outT, DirectX::XMFLOAT3& outNormal){
	XMVECTOR p0 = XMLoadFloat3(&start);
	XMVECTOR p1 = XMLoadFloat3(&end);
	XMVECTOR c = XMLoadFloat3(&sphere.center_position);

	XMVECTOR d = p1 - p0;       // レイ方向
	XMVECTOR m = p0 - c;        // 球中心→レイ始点

	float a = XMVectorGetX(XMVector3Dot(d, d));
	float b = XMVectorGetX(XMVector3Dot(m, d));
	float c2 = XMVectorGetX(XMVector3Dot(m, m)) - sphere.radius * sphere.radius;

	// 始点が球の外 & 球から遠ざかっている
	if (c2 > 0.0f && b > 0.0f) return false;

	float discr = b * b - a * c2;
	if (discr < 0.0f) return false;

	float t = (-b - sqrtf(discr)) / a;

	// 線分外チェック
	if (t < 0.0f || t > 1.0f) return false;

	outT = t;

	XMVECTOR hitPos = p0 + d * t;
	XMVECTOR normal = XMVector3Normalize(hitPos - c);
	XMStoreFloat3(&outNormal, normal);

	return true;
}
// カプセルの判定用の変数
float ClosestRaySegment(const DirectX::XMFLOAT3& rayStart, const DirectX::XMFLOAT3& rayDir, const DirectX::XMFLOAT3& segA, const DirectX::XMFLOAT3& segB, float& outRayT){
	XMVECTOR p = XMLoadFloat3(&rayStart);
	XMVECTOR d = XMLoadFloat3(&rayDir);
	XMVECTOR a = XMLoadFloat3(&segA);
	XMVECTOR b = XMLoadFloat3(&segB);
	XMVECTOR e = b - a;

	XMVECTOR r = p - a;

	float dd = XMVectorGetX(XMVector3Dot(d, d));
	float ee = XMVectorGetX(XMVector3Dot(e, e));
	float de = XMVectorGetX(XMVector3Dot(d, e));
	float dr = XMVectorGetX(XMVector3Dot(d, r));
	float er = XMVectorGetX(XMVector3Dot(e, r));

	float denom = dd * ee - de * de;

	float t = 0.0f;
	float u = 0.0f;

	if (denom > 1e-6f){
		t = (de * er - ee * dr) / denom;
		u = (dd * er - de * dr) / denom;
	}

	t = std::max(t, 0.0f);
	u = std::clamp(u, 0.0f, 1.0f);

	outRayT = t;

	XMVECTOR closestRay = p + d * t;
	XMVECTOR closestSeg = a + e * u;

	return XMVectorGetX(XMVector3LengthSq(closestRay - closestSeg));
}
bool Collision_RaycastCapsule(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const Capsule& capsule, float& outT, DirectX::XMFLOAT3& outNormal){
	XMVECTOR p0 = XMLoadFloat3(&start);
	XMVECTOR p1 = XMLoadFloat3(&end);
	XMVECTOR dir = p1 - p0;

	XMFLOAT3 d;
	XMStoreFloat3(&d, dir);

	float t;
	float distSq = ClosestRaySegment(start, d, capsule.start, capsule.end, t);

	if (distSq > capsule.radius * capsule.radius)
		return false;

	// 線分外チェック
	if (t < 0.0f || t > 1.0f) return false;

	outT = t;

	XMVECTOR hitPos = p0 + dir * t;

	// 最近接点から法線を作る
	XMVECTOR a = XMLoadFloat3(&capsule.start);
	XMVECTOR b = XMLoadFloat3(&capsule.end);
	XMVECTOR e = b - a;

	float u = XMVectorGetX( XMVector3Dot(hitPos - a, e) / XMVector3Dot(e, e));
	u = std::clamp(u, 0.0f, 1.0f);

	XMVECTOR closest = a + e * u;
	XMVECTOR normal = XMVector3Normalize(hitPos - closest);

	XMStoreFloat3(&outNormal, normal);
	return true;
}
//CapsulevsAABB で使用する関数
// 線分 vs 点 の最近接点
XMFLOAT3 ClosestPointOnSegment(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& p){
	XMVECTOR A = XMLoadFloat3(&a);
	XMVECTOR B = XMLoadFloat3(&b);
	XMVECTOR P = XMLoadFloat3(&p);

	XMVECTOR AB = B - A;
	float t = XMVectorGetX(XMVector3Dot(P - A, AB)) / XMVectorGetX(XMVector3Dot(AB, AB));

	t = std::clamp(t, 0.0f, 1.0f);

	XMVECTOR Q = A + AB * t;

	XMFLOAT3 result;
	XMStoreFloat3(&result, Q);
	return result;
}

XMFLOAT3 ClosestPointOnAABB(const XMFLOAT3& p, const AABB& box){
	return {
		std::clamp(p.x, box.min.x, box.max.x),
		std::clamp(p.y, box.min.y, box.max.y),
		std::clamp(p.z, box.min.z, box.max.z)
	};
}

bool Collision_IsOverlapCapsulevsAABB(const Capsule& c, const AABB& a, XMFLOAT3& outNormal, float& outDepth){
	
	// 線分上の最近接点
	XMFLOAT3 segPoint = ClosestPointOnSegment(c.start, c.end, a.GetCenter());

	// AABB 上の最近接点
	XMFLOAT3 boxPoint = ClosestPointOnAABB(segPoint, a);

	// 距離判定
	XMVECTOR v = XMLoadFloat3(&segPoint) - XMLoadFloat3(&boxPoint);

	float distSq = XMVectorGetX(XMVector3LengthSq(v));

	if (distSq > c.radius * c.radius) return false;

	float dist = sqrtf(distSq);
	outDepth = c.radius - dist;

	if (dist > 1e-6f) {
		XMVECTOR n = XMVector3Normalize(v);
		XMStoreFloat3(&outNormal, n);
	} else {
		// 内部に完全に入った場合（保険）
		outNormal = { 0, 1, 0 };
	}

	return true;
}

Hit Collision_IsHitCapsulevsAABB(const Capsule& c, const AABB& a){
	Hit hit{};
	hit.isHit = false;

	// Capsule線分の最近点
	XMFLOAT3 boxCenter = a.GetCenter();
	XMFLOAT3 capsulePoint =ClosestPointOnSegment(c.start, c.end, boxCenter);

	// AABBの最近点
	XMFLOAT3 boxPoint = ClosestPointOnAABB(capsulePoint, a);

	// 距離判定
	XMVECTOR n = XMLoadFloat3(&capsulePoint) - XMLoadFloat3(&boxPoint);

	float distSq = XMVectorGetX(XMVector3LengthSq(n));

	if (distSq > c.radius * c.radius) return hit;

	float dist = sqrtf(distSq);

	hit.isHit = true;
	hit.depth = c.radius - dist;

	if (dist > 1e-6f){
		n = XMVector3Normalize(n);
		XMStoreFloat3(&hit.normal, n);
	} else {
		hit.normal = { 0,1,0 }; // 保険
	}

	return hit;
}

Hit Collision_IsHitCapsuleVsSphere(const Capsule& cap, const DirectX::XMFLOAT3& sphereCenter, float sphereRadius){
	Hit hit{};
	hit.isHit = false;

	XMVECTOR A = XMLoadFloat3(&cap.start);
	XMVECTOR B = XMLoadFloat3(&cap.end);
	XMVECTOR P = XMLoadFloat3(&sphereCenter);

	XMVECTOR AB = B - A;

	float abLenSq = XMVectorGetX(XMVector3LengthSq(AB));
	if (abLenSq < 1e-6f) return hit; // 念のため

	float t = XMVectorGetX(
		XMVector3Dot(P - A, AB) / abLenSq
	);
	t = std::clamp(t, 0.0f, 1.0f);

	XMVECTOR Q = A + AB * t;

	XMVECTOR diff = P - Q;
	float distSq = XMVectorGetX(XMVector3LengthSq(diff));

	float r = cap.radius + sphereRadius;

	if (distSq > r * r)
		return hit;

	float dist = sqrtf(distSq);

	hit.isHit = true;

	// 法線
	if (dist > 1e-6f) {
		XMVECTOR n = diff / dist;
		XMStoreFloat3(&hit.normal, n);
	}
	else {
		// 完全に重なった場合の保険
		hit.normal = { 0,1,0 };
	}

	hit.depth = r - dist;
	return hit;
}

AABB ExpandAABB(const AABB& aabb, float r){
	return {
		{ aabb.min.x - r , aabb.min.y - r , aabb.min.z - r },
		{ aabb.max.x + r , aabb.max.y  , aabb.max.z + r }
	};
}

bool Collision_SweptCapsuleVsAABB(const Capsule& oldCap, const Capsule& newCap, const AABB& box, float& outT, XMFLOAT3& outNormal){
	outT = 1.0f;
	outNormal = { 0,0,0 };
	bool hit = false;

	// 半径分AABBを拡張
	AABB expanded = ExpandAABB(box, oldCap.radius);

	auto testRay = [&](const XMFLOAT3& s0, const XMFLOAT3& s1)
		{
			// 開始点が拡張AABBの中なら Swept では無視
			if (PointInsideAABB(s0, expanded)) return;

			XMVECTOR d = XMLoadFloat3(&s1) - XMLoadFloat3(&s0);
			if (XMVectorGetX(XMVector3LengthSq(d)) < 1e-6f)
				return;

			float t;
			XMFLOAT3 n;
			if (Collision_RaycastAABB(s0, s1, expanded, t, n)) {
				d = XMLoadFloat3(&newCap.start) - XMLoadFloat3(&oldCap.start);
				XMVECTOR nVec = XMLoadFloat3(&n);
				// 法線を必ず進行方向と逆に
				if (XMVectorGetX(XMVector3Dot(d, nVec)) > 0.0f) {
					nVec = -nVec;
					XMStoreFloat3(&n, nVec);
				}

				if (t < outT) {
					outT = t;
					outNormal = n;
					hit = true;
				}
			}
			if (t <= 1e-4f) return;
		};


	// 下端レイ
	testRay(oldCap.start, newCap.start);
	// 下半球（床用）
	XMFLOAT3 oldBottom = {
		oldCap.start.x,
		oldCap.start.y - oldCap.radius,
		oldCap.start.z
	};
	XMFLOAT3 newBottom = {
		newCap.start.x,
		newCap.start.y - newCap.radius,
		newCap.start.z
	};
	testRay(oldBottom, newBottom);
	// 上端レイ
	testRay(oldCap.end, newCap.end);
	// 中心レイ（保険：段差・斜め）
	XMFLOAT3 oldMid{
		(oldCap.start.x + oldCap.end.x) * 0.5f,
		(oldCap.start.y + oldCap.end.y) * 0.5f,
		(oldCap.start.z + oldCap.end.z) * 0.5f
	};
	XMFLOAT3 newMid{
		(newCap.start.x + newCap.end.x) * 0.5f,
		(newCap.start.y + newCap.end.y) * 0.5f,
		(newCap.start.z + newCap.end.z) * 0.5f
	};
	testRay(oldMid, newMid);

	return hit;
}

//デバック用
//ある理由から頂点の数ぴったりにする
static constexpr int NUM_VERTEX = 5000; // 頂点数 約1600*3.14

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11ShaderResourceView* g_pTexture = nullptr; // テクスチャ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 頂点構造体
struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;    // 色　XMFLOAT4　floatを４つ入れる構造体
	XMFLOAT2 uv; // テクスチャ座標　UV値
};

static int g_WhiteTexID = -1;

void DrawDebugLine(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT4& color); // デバック用の線描画

void Collision_DebudInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext){
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	g_WhiteTexID = Texture_Load(L"resuce/img/w.png");
}

void Collision_DebudFinalize(){
	SAFE_RELEASE(g_pTexture); // テクスチャの返却
	SAFE_RELEASE(g_pVertexBuffer); // 頂点バッファの解放
}

void Collision_DebugDraw(const Circle& circle,const XMFLOAT4 color){
	// 点の数の算出
	int NumVertex = (int)(circle.radius * 2.0f * XM_PI + 1); // 直径の長さ≒円の調点数

	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	const float rad = XM_2PI / NumVertex;

	for (int i = 0;i < NumVertex;i++) {
		v[i].position.x = cosf(rad * i) * circle.radius + circle.center_position.x;
		v[i].position.y = sinf(rad * i) * circle.radius + circle.center_position.y;
		v[i].position.z = 0.0f;
		v[i].color = color;
		v[i].uv = { 0.0f,0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);
	//ワールド変換行列を設定
	Shader_SetWorldMatrix(XMMatrixIdentity());
	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	//テクスチャの設定
	Texture_SetTexture(g_WhiteTexID);

	// ポリゴン描画命令発行
	g_pContext->Draw(NumVertex, 0);
}

void Collision_DebugDraw(const Box& box, const XMFLOAT4 color){
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	//頂点情報の書き込み
	v[0].position = { box.center_position.x - box.hafe_width,box.center_position.y - box.hafe_height,0.0f };
	v[1].position = { box.center_position.x + box.hafe_width,box.center_position.y - box.hafe_height,0.0f };
	v[2].position = { box.center_position.x + box.hafe_width,box.center_position.y + box.hafe_height,0.0f };
	v[3].position = { box.center_position.x - box.hafe_width,box.center_position.y + box.hafe_height,0.0f };
	v[4].position = { box.center_position.x - box.hafe_width,box.center_position.y - box.hafe_height,0.0f };

	for (int i = 0;i < 5;i++) {
		v[i].color = color;
		v[i].uv = { 0.0f,0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);
	//ワールド変換行列を設定
	//回転に対応していないから回転させてはいけない
	Shader_SetWorldMatrix(XMMatrixIdentity());
	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	//テクスチャの設定
	Texture_SetTexture(g_WhiteTexID);
	
	// ポリゴン描画命令発行
	g_pContext->Draw(5, 0);
}

// 3D用当たり判定描画
void AABB_GetVertices(const AABB& aabb, XMFLOAT3 out[8])
{
	const auto& min = aabb.min;
	const auto& max = aabb.max;

	out[0] = { min.x, min.y, min.z };
	out[1] = { max.x, min.y, min.z };
	out[2] = { max.x, max.y, min.z };
	out[3] = { min.x, max.y, min.z };

	out[4] = { min.x, min.y, max.z };
	out[5] = { max.x, min.y, max.z };
	out[6] = { max.x, max.y, max.z };
	out[7] = { min.x, max.y, max.z };
}

static const uint16_t AABB_LINE_INDEX[24] =
{
	0,1, 1,2, 2,3, 3,0, // 前面
	4,5, 5,6, 6,7, 7,4, // 背面
	0,4, 1,5, 2,6, 3,7  // 縦
};

struct DebugVertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};


// 定数宣言
static constexpr int DEBUG_VERTEX_MAX = 4096; // 1フレームで描画できる最大頂点数

// 変数宣言
static DebugVertex g_DebugVertexArray[DEBUG_VERTEX_MAX]; // 頂点を溜めるバケツ
static int g_DebugVertexCount = 0; // 現在溜まっている頂点数
ID3D11Buffer* g_DebugVB = nullptr;

void Collision_Debud3DInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext){
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DebugVertex) * DEBUG_VERTEX_MAX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_DebugVB);

	//g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	g_WhiteTexID = Texture_Load(L"resuce/img/w.png");
}

void Collision_Debud3DFinalize() {
	//SAFE_RELEASE(g_pTexture); // テクスチャの返却
	SAFE_RELEASE(g_DebugVB); // 頂点バッファの解放
}

// 登録した頂点を超えたら朝―とで止める
// デバックドロウを呼ぶたびに頂点情報をバッファに書き込んで終わり→頂点数を計算
// 登録しながら数える　そのフレームの最後に書き込む
//　頂点数を数える関数を作れば三通ともにできる
void Collision_DebugDraw(const AABB& aabb, const XMFLOAT4 color)
{
	// 頂点数チェック（24個分）
	if (g_DebugVertexCount + 24 > DEBUG_VERTEX_MAX) {
		assert(false && "DebugDraw: AABBの頂点が入り切りません！");
		return;
	}

	XMFLOAT3 corner[8];
	AABB_GetVertices(aabb, corner);

	// バケツ（g_DebugVertexArray）の現在の位置から24個書き込む
	for (int i = 0; i < 24; i++)
	{
		g_DebugVertexArray[g_DebugVertexCount].position = corner[AABB_LINE_INDEX[i]];
		g_DebugVertexArray[g_DebugVertexCount].color = color;
		g_DebugVertexCount++; // 1個入れるごとにカウントアップ
	}

	//XMFLOAT3 corner[8];
	//AABB_GetVertices(aabb, corner);
	//
	//DebugVertex vtx[24];
	//for (int i = 0; i < 24; i++) // ココで貯めて
	//{
	//	vtx[i].position = corner[AABB_LINE_INDEX[i]];
	//	vtx[i].color = color;
	//}
	//
	//D3D11_MAPPED_SUBRESOURCE ms;
	//auto ctx = Direct3D_GetContext();
	//ctx->Map(g_DebugVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	//memcpy(ms.pData, vtx, sizeof(vtx));
	//ctx->Unmap(g_DebugVB, 0);
	//
	//UINT stride = sizeof(DebugVertex);
	//UINT offset = 0;
	//ctx->IASetVertexBuffers(0, 1, &g_DebugVB, &stride, &offset);
	//ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//
	//// Unlitシェーダを使用
	//Shader3dUnlit_Begin();
	//Shader3dUnlit_SetWorldMatrix(XMMatrixIdentity());
	//Shader3dUnlit_SetColor(color);
	//
	//ctx->Draw(24, 0); // 貯めたのを一気によぶ
}

// 各軸の円の描画
/*
void MakeCircleXY( // XY平面上の円
	const Sphere& s,
	DebugVertex* vtx,
	int segments,
	const XMFLOAT4& color
)
{
	float step = XM_2PI / segments;

	for (int i = 0; i < segments; i++)
	{
		float a0 = step * i;
		float a1 = step * (i + 1);

		vtx[i * 2 + 0].position =
		{
			s.center_position.x + cosf(a0) * s.radius,
			s.center_position.y + sinf(a0) * s.radius,
			s.center_position.z
		};
		vtx[i * 2 + 0].color = color;

		vtx[i * 2 + 1].position =
		{
			s.center_position.x + cosf(a1) * s.radius,
			s.center_position.y + sinf(a1) * s.radius,
			s.center_position.z
		};
		vtx[i * 2 + 1].color = color;
	}
}

void MakeCircleXZ( // XZ平面上の円
	const Sphere& s,
	DebugVertex* vtx,
	int segments,
	const XMFLOAT4& color
)
{
	float step = XM_2PI / segments;

	for (int i = 0; i < segments; i++)
	{
		float a0 = step * i;
		float a1 = step * (i + 1);

		vtx[i * 2 + 0].position =
		{
			s.center_position.x + cosf(a0) * s.radius,
			s.center_position.y ,
			s.center_position.z + sinf(a0) * s.radius
		};
		vtx[i * 2 + 0].color = color;

		vtx[i * 2 + 1].position =
		{
			s.center_position.x + cosf(a1) * s.radius,
			s.center_position.y ,
			s.center_position.z + sinf(a1) * s.radius
		};
		vtx[i * 2 + 1].color = color;
	}
}

void MakeCircleYZ( // YZ平面上の円
	const Sphere& s,
	DebugVertex* vtx,
	int segments,
	const XMFLOAT4& color
)
{
	float step = XM_2PI / segments;

	for (int i = 0; i < segments; i++)
	{
		float a0 = step * i;
		float a1 = step * (i + 1);

		vtx[i * 2 + 0].position =
		{
			s.center_position.x ,
			s.center_position.y + cosf(a0) * s.radius,
			s.center_position.z + sinf(a0) * s.radius
		};
		vtx[i * 2 + 0].color = color;

		vtx[i * 2 + 1].position =
		{
			s.center_position.x ,
			s.center_position.y + cosf(a1) * s.radius,
			s.center_position.z + sinf(a1) * s.radius
		};
		vtx[i * 2 + 1].color = color;
	}
}
*/
void MakeCircleXY(const Sphere& s, int segments, const XMFLOAT4& color){
	float step = XM_2PI / segments;
	for (int i = 0; i < segments; i++)
	{
		if (g_DebugVertexCount + 2 > DEBUG_VERTEX_MAX) return; // 安全策

		float a0 = step * i;
		float a1 = step * (i + 1);

		// 線分1つにつき頂点2個をバケツへ
		g_DebugVertexArray[g_DebugVertexCount++] = {
			{ s.center_position.x + cosf(a0) * s.radius, 
			  s.center_position.y + sinf(a0) * s.radius,
			  s.center_position.z },
			color
		};
		g_DebugVertexArray[g_DebugVertexCount++] = {
			{ s.center_position.x + cosf(a1) * s.radius,
			  s.center_position.y + sinf(a1) * s.radius,
			  s.center_position.z },
			color
		};
	}
}

void MakeCircleXZ(const Sphere& s, int segments, const XMFLOAT4& color) {
	float step = XM_2PI / segments;
	for (int i = 0; i < segments; i++)
	{
		if (g_DebugVertexCount + 2 > DEBUG_VERTEX_MAX) return; // 安全策

		float a0 = step * i;
		float a1 = step * (i + 1);

		// 線分1つにつき頂点2個をバケツへ
		g_DebugVertexArray[g_DebugVertexCount++] = {
			{ s.center_position.x + cosf(a0) * s.radius,
			  s.center_position.y ,
			  s.center_position.z + sinf(a0) * s.radius},
			color
		};
		g_DebugVertexArray[g_DebugVertexCount++] = {
			{ s.center_position.x + cosf(a1) * s.radius,
			  s.center_position.y ,
			  s.center_position.z + sinf(a1) * s.radius},
			color
		};
	}
}

void MakeCircleYZ(const Sphere& s, int segments, const XMFLOAT4& color) {
	float step = XM_2PI / segments;
	for (int i = 0; i < segments; i++)
	{
		if (g_DebugVertexCount + 2 > DEBUG_VERTEX_MAX) return; // 安全策

		float a0 = step * i;
		float a1 = step * (i + 1);

		// 線分1つにつき頂点2個をバケツへ
		g_DebugVertexArray[g_DebugVertexCount++] = {
			{ s.center_position.x , 
			  s.center_position.y + cosf(a0) * s.radius,
			  s.center_position.z + sinf(a0) * s.radius },
			  color };
		g_DebugVertexArray[g_DebugVertexCount++] = { 
			{ s.center_position.x , 
			  s.center_position.y + cosf(a1) * s.radius,
			  s.center_position.z + sinf(a1) * s.radius }, 
			  color };
	}		  
}

void Collision_DebugDraw(const Sphere& s, const DirectX::XMFLOAT4 color){
	constexpr int SEG = 32;
	// 頂点数チェック (SEG * 2本 * 3つの円 = SEG * 6)
	assert(g_DebugVertexCount + (SEG * 6) <= DEBUG_VERTEX_MAX);

	MakeCircleXY(s, SEG, color);
	MakeCircleXZ(s, SEG, color); // これらも同様に修正が必要
	MakeCircleYZ(s, SEG, color);
	
	//constexpr int SEG = 32;
	//DebugVertex vtx[SEG * 2 * 3];
	//int offset = 0;
	//
	//MakeCircleXY(s, &vtx[offset], SEG, color);
	//offset += SEG * 2;
	//
	//MakeCircleXZ(s, &vtx[offset], SEG, color);
	//offset += SEG * 2;
	//
	//MakeCircleYZ(s, &vtx[offset], SEG, color);
	//
	//// バッファ書き込み
	//D3D11_MAPPED_SUBRESOURCE ms;
	//auto ctx = Direct3D_GetContext();
	//ctx->Map(g_DebugVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	//memcpy(ms.pData, vtx, sizeof(vtx));
	//ctx->Unmap(g_DebugVB, 0);
	//UINT stride = sizeof(DebugVertex);
	//UINT ofs = 0;
	//ctx->IASetVertexBuffers(0, 1, &g_DebugVB, &stride, &ofs);
	//ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//
	//Shader3dUnlit_Begin();
	//Shader3dUnlit_SetWorldMatrix(XMMatrixIdentity());
	//Shader3dUnlit_SetColor(color);
	//
	//ctx->Draw(_countof(vtx), 0);
}

void Collision_DebugDraw(const Capsule& c, const DirectX::XMFLOAT4 color){
	// 内部で修正済みの Sphere や DrawDebugLine を呼ぶ
	Sphere s0 = { c.start, c.radius };
	Sphere s1 = { c.end, c.radius };

	Collision_DebugDraw(s0, color); // ここで頂点が溜まる
	Collision_DebugDraw(s1, color); // ここでも溜まる

	// 円柱の縦線4本
	DrawDebugLine({ c.start.x + c.radius, c.start.y, c.start.z }, { c.end.x + c.radius, c.end.y, c.end.z }, color);
	DrawDebugLine({ c.start.x - c.radius, c.start.y, c.start.z }, { c.end.x - c.radius, c.end.y, c.end.z }, color);
	DrawDebugLine({ c.start.x, c.start.y, c.start.z + c.radius }, { c.end.x, c.end.y, c.end.z + c.radius }, color);
	DrawDebugLine({ c.start.x, c.start.y, c.start.z - c.radius }, { c.end.x, c.end.y, c.end.z - c.radius }, color);
	
	//// ① 上下の球
	//Sphere s0;
	//s0.center_position = c.start;
	//s0.radius = c.radius;
	//
	//Sphere s1;
	//s1.center_position = c.end;
	//s1.radius = c.radius;
	//
	//Collision_DebugDraw(s0, color);
	//Collision_DebugDraw(s1, color);
	//
	//// ② 円柱部分（4本で十分）
	//DrawDebugLine(
	//	{ c.start.x + c.radius, c.start.y, c.start.z },
	//	{ c.end.x + c.radius, c.end.y,   c.end.z },
	//	color
	//);
	//DrawDebugLine(
	//	{ c.start.x - c.radius, c.start.y, c.start.z },
	//	{ c.end.x - c.radius, c.end.y,   c.end.z },
	//	color
	//);
	//DrawDebugLine(
	//	{ c.start.x, c.start.y, c.start.z + c.radius },
	//	{ c.end.x,   c.end.y,   c.end.z + c.radius },
	//	color
	//);
	//DrawDebugLine(
	//	{ c.start.x, c.start.y, c.start.z - c.radius },
	//	{ c.end.x,   c.end.y,   c.end.z - c.radius },
	//	color
	//);

}

void DrawDebugLine(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT4& color){
	// 登録した頂点数(今回は2個)を超えないかチェック（先生のアドバイス：アサート）
	assert(g_DebugVertexCount + 2 <= DEBUG_VERTEX_MAX && "DebugDraw: 頂点数が多すぎます！");

	// バケツに頂点情報を入れるだけ
	g_DebugVertexArray[g_DebugVertexCount++] = { a, color };
	g_DebugVertexArray[g_DebugVertexCount++] = { b, color };
	
	//DebugVertex vtx[2];
	//vtx[0] = { a, color };
	//vtx[1] = { b, color };
	//
	//D3D11_MAPPED_SUBRESOURCE ms;
	//auto ctx = Direct3D_GetContext();
	//ctx->Map(g_DebugVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	//memcpy(ms.pData, vtx, sizeof(vtx));
	//ctx->Unmap(g_DebugVB, 0);
	//
	//UINT stride = sizeof(DebugVertex);
	//UINT offset = 0;
	//ctx->IASetVertexBuffers(0, 1, &g_DebugVB, &stride, &offset);
	//ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//
	//Shader3dUnlit_Begin();
	//Shader3dUnlit_SetWorldMatrix(DirectX::XMMatrixIdentity());
	//Shader3dUnlit_SetColor(color);
	//
	//ctx->Draw(2, 0);
}

void Collision_DebugDraw_Execute() {
	if (g_DebugVertexCount == 0) return; // 描画するものがなければ何もしない
	Direct3D_SetDepthEnable(false);

	// ★安全チェック：カウントが最大数を超えていたら、最大数で丸める
	int drawCount = (g_DebugVertexCount > DEBUG_VERTEX_MAX) ? DEBUG_VERTEX_MAX : g_DebugVertexCount;

	// バケツの中身を丸ごと GPU に送る（1回だけ！）
	//D3D11_MAPPED_SUBRESOURCE ms;
	//auto ctx = Direct3D_GetContext();
	//if (SUCCEEDED(ctx->Map(g_DebugVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
	//	memcpy(ms.pData, g_DebugVertexArray, sizeof(DebugVertex) * drawCount);
	//	ctx->Unmap(g_DebugVB, 0);
	//}

	D3D11_MAPPED_SUBRESOURCE ms;
	Direct3D_GetContext()->Map(g_DebugVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, g_DebugVertexArray, sizeof(DebugVertex) * drawCount);
	Direct3D_GetContext()->Unmap(g_DebugVB, 0);

	// 描画設定
	UINT stride = sizeof(DebugVertex);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_DebugVB, &stride, &offset);
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	Shader3dUnlit_Begin();
	Shader3dUnlit_SetWorldMatrix(XMMatrixIdentity());
	Shader3dUnlit_SetColor({1.0f,0.0f,0.0f,1.0f}); // 線の色は赤
	// 一気によぶ！
	Direct3D_GetContext()->Draw(g_DebugVertexCount, 0);

	// 次のフレームのためにカウントをリセット
	g_DebugVertexCount = 0;
}