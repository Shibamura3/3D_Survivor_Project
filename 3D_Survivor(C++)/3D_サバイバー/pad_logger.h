/*
	ゲームパッドの制御：pad_logger.h

	2025/12/03	hibiki sakuma
*/

#ifndef PAD_LOGGER
#define PAD_LOGGER

#include <windows.h>
#include <Xinput.h>
#include <SDL.h>
#include <DirectXMath.h>

// 操作モードの定義
enum class InputMode {
	KeyboardMouse,
	Controller
};

void PadLogger_Initalize();

void PadLogger_Update(); // フレームの初めに予備->いつ前の入力を保存している為

// user_index : 0,1,2,3　でつないだ人数を最大4人まで判定できる
// buttons : ボタンのみの判定、レバーやトリガーは対応していない
bool PadLogger_IsPressed(DWORD user_index, SDL_GameControllerButton button); // 押したままの判定
bool PadLogger_IsTrigger(DWORD user_index, SDL_GameControllerButton button); // 一瞬だけ押す判定
//bool PadLogger_IsRelease(DWORD user_index, WORD buttons); // 離した瞬間の判定

// スティックの取得
DirectX::XMFLOAT2 PadLogger_GetLeftThumbStick(WORD user_index);
DirectX::XMFLOAT2 PadLogger_GetRightThumbStick(WORD user_index);

// トリガーの取得
float PadLogger_GetLeftTrigger(WORD user_index);
float PadLogger_GetRightTrigger(WORD user_index);

// 現在のモードを外部から取得する関数
InputMode PadLogger_GetInputMode();
// 現在の接続状況を外部から取得する関数
bool PadLogger_IsConnected();
// 振動
//void PadLogger_SetVibrationEnable(WORD user_index, bool enable); // オンオフのみの振動

#endif // !PAD_LOGGER
