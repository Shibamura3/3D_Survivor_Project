/*
	é¿ê—âÊñ ÇÃêßå‰ÅFachieve.h

	2025/12/19	hibiki sakuma
*/
#ifndef ACHIEVE_H
#define ACHIEVE_H

#include "collision.h"

void Achieve_Initialize();
void Achieve_Finalize();
void Achieve_Update(double elapsed_time);
void Achieve_Draw();

Box Achieve_GetCollision();

#endif // !ACHIEVE_H
