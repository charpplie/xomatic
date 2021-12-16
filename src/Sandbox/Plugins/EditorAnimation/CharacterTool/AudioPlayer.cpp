#include "StdAfx.h"
#include "AudioPlayer.h"
#include <IEditor.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <IAudioSystem.h>
#include "../../CryEngine/CryAction/IViewSystem.h"

namespace CharacterTool {


// A hack to move Character Tool audio objects away from level sounds
// (we have to share the same audio world).
static const Vec3 AUDIO_OFFSET(-8192, -8192, -8192);

AudioPlayer::AudioPlayer()
{
	m_audioProxy = gEnv->pAudioSystem->GetFreeAudioProxy();
	if (m_audioProxy)
		m_audioProxy->Initialize("Character Tool", false);
}

AudioPlayer::~AudioPlayer()
{
	if (m_audioProxy)
	{
		m_audioProxy->Release();
		m_audioProxy = 0;
	}
}

void AudioPlayer::UpdateSoundPositions(const QuatT& playerLocation, const Matrix34& cameraMatrix)
{
	if (!m_enabled)
		return;

	SAudioRequest request;
	request.nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
	request.nFlags = eARF_PRIORITY_NORMAL;
	request.pOwner = 0;

	Matrix34 cameraWithOffset = cameraMatrix;
	cameraWithOffset.SetTranslation(cameraMatrix.GetTranslation() + AUDIO_OFFSET);
	SAudioListenerRequestData<eALRT_SET_POSITION> requestData(cameraWithOffset, ZERO);
	request.pData = &requestData;

	gEnv->pAudioSystem->PushRequest(request);

	if (m_audioProxy)
	{
		SATLWorldPosition pos;
		pos.mPosition = Matrix34(playerLocation);
		pos.mPosition.SetTranslation(pos.mPosition.GetTranslation() + AUDIO_OFFSET);
		pos.vVelocity = ZERO;
		m_audioProxy->SetPosition(pos);
	}
}

void AudioPlayer::Enable(bool enable)
{
	if (m_enabled != enable)
	{
		m_enabled = enable;
	}
}

void AudioPlayer::Play(const char* soundName)
{
	if (!m_audioProxy)
		return;

	TAudioControlID audioControlId = INVALID_AUDIO_CONTROL_ID;
	gEnv->pAudioSystem->GetAudioTriggerID(soundName, audioControlId);

	if (audioControlId != INVALID_AUDIO_CONTROL_ID)
	{
		m_audioProxy->ExecuteTrigger(audioControlId, eLSM_None);
	}
}

}

