/*
	キーボード入力の記録 kye_logger.h

	2025/06/27	hibiki sakuma
	パッド版も同じように作れる
*/

#include "Key_logger.h"

static Keyboard_State g_PrevState{}; // 1フレーム前の入力

static Keyboard_State g_TriggerState{};
static Keyboard_State g_Releasestate{};


void KeyLogger_Initialize(){
	Keyboard_Initialize();
}

void KeyLogger_Update(){
	const Keyboard_State* pState = Keyboard_GetState();
	LPBYTE pn = (LPBYTE)pState; // unsigned char pointer としてごまかし
	LPBYTE pp = (LPBYTE)&g_PrevState;
	LPBYTE pt = (LPBYTE)&g_TriggerState;
	LPBYTE pr = (LPBYTE)&g_Releasestate;
	
	for (int i = 0; i < sizeof(Keyboard_State);i++) {
		pt[i] = (pp[i] ^ pn[i]) & pn[i];// XOR ^ AND &　
		pr[i] = (pp[i] ^ pn[i]) & pp[i];
	}

	g_PrevState = *pState;//過去のものに今のものを移す

}

bool KeyLogger_IsPressed(Keyboard_Keys key){

    return Keyboard_IsKeyDown(key);
}

bool KeyLogger_IsTrigger(Keyboard_Keys key){

	return Keyboard_IsKeyDown(key, &g_TriggerState);
}

bool KeyLogger_IsRelease(Keyboard_Keys key){
    
	return Keyboard_IsKeyDown(key, &g_Releasestate);
}

// どこかキーが押されているか判定
bool KeyLogger_AnyKeyPressed() {
	const Keyboard_State* pState = Keyboard_GetState();
	const unsigned char* pData = (const unsigned char*)pState;

	for (int i = 0; i < sizeof(Keyboard_State); i++) {
		if (pData[i] != 0) return true; // どこか1つでもキーが押されていればtrue
	}
	return false;
}
