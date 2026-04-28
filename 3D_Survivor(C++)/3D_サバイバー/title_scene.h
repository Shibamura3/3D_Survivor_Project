/*
	É^ÉCÉgÉãâÊñ ÇÃêßå‰ : title_scene.h

	2025/07/10	hibiki sakuma
*/


#ifndef TITLE_SCENE_H
#define TITLE_SCENE_H

#include "collision.h"

void TitleScene_Initialize();
void TitleScene_Finalize();
void TitleScene_Update(double elapsed_time);
void TitleScene_Draw();

Box TitleScene_GetButton(int index);

#endif // TITLE_SCENE_H