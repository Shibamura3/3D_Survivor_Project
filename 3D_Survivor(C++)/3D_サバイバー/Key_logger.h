/*******************************************************
*	
*	キーボード入力の記録 kye_logger.h
*
*	2025/06/27	hibiki sakuma
*	パッド版も同じように作れる
*********************************************************/

#ifndef KEY_LOGGER_H
#define KEY_LOGGER_H

#include "keyboard.h"

//キーボードのプログラムをラッピングする
void KeyLogger_Initialize();

void KeyLogger_Update();

bool KeyLogger_IsPressed(Keyboard_Keys key); //押した瞬間
bool KeyLogger_IsTrigger(Keyboard_Keys key); //押したときだけ
bool KeyLogger_IsRelease(Keyboard_Keys key); //離したとき
bool KeyLogger_AnyKeyPressed(); // 何かのキーが押された時
#endif // KEY_LOGGER_H
