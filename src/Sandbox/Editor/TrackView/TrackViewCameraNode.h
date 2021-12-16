//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2014.
//
//  Created: 26/2/2014 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TrackViewAnimNode.h"
#include "Objects/CameraObject.h"

////////////////////////////////////////////////////////////////////////////
//
// This class represents an IAnimNode that is a camera
//
////////////////////////////////////////////////////////////////////////////
class CTrackViewCameraNode : public CTrackViewAnimNode, public ICameraObjectListener
{	
public:
	CTrackViewCameraNode(IAnimSequence *pSequence, IAnimNode *pAnimNode, CTrackViewNode *pParentNode)
		: CTrackViewAnimNode(pSequence, pAnimNode, pParentNode) {}

	virtual void OnNodeAnimated(IAnimNode *pNode) override;

	virtual void BindToEditorObjects() override;
	virtual void UnBindFromEditorObjects() override;

	// Get camera shake rotation
	void GetShakeRotation(const float time, Quat &rotation);

private:
	virtual void OnFovChange(const float fov);
	virtual void OnNearZChange(const float nearZ);
	virtual void OnFarZChange(const float farZ) {}
	virtual void OnShakeAmpAChange(const Vec3 amplitude);
	virtual void OnShakeAmpBChange(const Vec3 amplitude);
	virtual void OnShakeFreqAChange(const Vec3 frequency);
	virtual void OnShakeFreqBChange(const Vec3 frequency);
	virtual void OnShakeMultChange(const float amplitudeAMult, const float amplitudeBMult, const float frequencyAMult, const float frequencyBMult);
	virtual void OnShakeNoiseChange(const float noiseAAmpMult, const float noiseBAmpMult, const float noiseAFreqMult, const float noiseBFreqMult);
	virtual void OnShakeWorkingChange(const float timeOffsetA, const float timeOffsetB);
	virtual void OnCameraShakeSeedChange(const int seed);
};