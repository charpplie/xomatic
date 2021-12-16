#pragma once
#include <IEntitySystem.h>

namespace CharacterTool
{

class AudioPlayer
{
public:
	AudioPlayer();
	~AudioPlayer();

	void Enable(bool enabled);
	void Play(const char* soundName);
	void UpdateSoundPositions(const QuatT& playerPhysicalPosition, const Matrix34& cameraMatrix);

private:
	bool m_enabled;
	IAudioProxy* m_audioProxy;
};

};
