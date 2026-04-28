/*
	弾丸の衝突エフェクトの表示：bullet_hit_effect.h

	2025/11/19	hibiki sakuma
*/

#include "bullet_hit_effect.h"
#include "resource_manager.h"
#include "sprite_animu.h"
#include "billboard.h"
#include "sheder_billboard.h"
using namespace DirectX;

// グローバル変数
static int g_AnimPatternId = -1;
static int g_EffectCount = 0;

// 定数宣言
static constexpr int EFFECT_MAX = 256;

class BulletHitEffect {
private:
	XMFLOAT3 m_position{};
	bool m_is_desyroy{ false };
	int m_anim_play_id{ -1 };
public:
	BulletHitEffect(const XMFLOAT3& position)
		: m_position(position), m_anim_play_id(SpriteAnim_CreatePlayer(g_AnimPatternId))
	{ 
		
	}
	~BulletHitEffect() {
		SpriteAnim_DestroyPlayer(m_anim_play_id);
	}

	void Updata();
	void Draw() const; // バーチャルだと別のでも呼ぶことができる
	
	bool IsDestroy() const {
		return m_is_desyroy;
	}
};

static BulletHitEffect* g_pEffects[EFFECT_MAX]{};

void Bullet_HitEffect_Initialize(){
	g_AnimPatternId = Spriteanim_RegisterPattern(Resouce_Manager_GetTexId(Effect_Explosion), 9, 3, 0.1, {83,83}, {0,0}, false, false);
	g_EffectCount = 0;
}

void Bullet_HitEffect_Finalize(){
	
	for (int i = 0; i < g_EffectCount; i++) {
		delete g_pEffects[i];
	}
}

void Bullet_HitEffect_Updata(){
	for (int i = 0; i < g_EffectCount; i++) {
		g_pEffects[i]->Updata();
	}

	for (int i = g_EffectCount - 1; i >= 0; i--) {
		if(g_pEffects[i]->IsDestroy()) {
			delete g_pEffects[i];
			g_pEffects[i] = g_pEffects[g_EffectCount - 1]; // 末尾を空いたところに入れる
			g_EffectCount--;
		}
	}
}

void Bullet_HitEffect_Draw(){
	for (int i = 0; i < g_EffectCount; i++) {
		g_pEffects[i]->Draw();
	}
}

void Bullet_HitEffect_Create(const DirectX::XMFLOAT3& position){
	g_pEffects[g_EffectCount++] = new BulletHitEffect(position);
}

void BulletHitEffect::Updata(){
	if (SpriteAnim_IsStopped(m_anim_play_id)) {
		m_is_desyroy = true; // 再生が終わったら削除する
	}
}

void BulletHitEffect::Draw() const{
	BillboardAnim_Draw(m_anim_play_id, m_position, { 2.0f, 2.0f });
	assert(m_anim_play_id >= 0);
}
