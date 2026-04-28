#pragma comment(lib, "winmm.lib")

#ifndef AUDIO_H
#define AUDIO_H

void InitAudio();
void UninitAudio();

int LoadAudio(const char* FileName);
void UnloadAudio(int Index);
void PlayAudio(int Index, bool Loop = false);
void StopAllAudio();
void StopAudio(int Index);

#endif // !AUDIO_H