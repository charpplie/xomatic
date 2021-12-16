//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ICONTROLLERKEYHANDLER_H__
#define __ICONTROLLERKEYHANDLER_H__

class IControllerKeyHandler
{
public:
	virtual void LinearInterpolationFloat(CryLin1Key& key) = 0;
	virtual void LinearInterpolationPosition(CryLin3Key& key) = 0;
	virtual void LinearInterpolationScale(CryLin3Key& key) = 0;
	virtual void LinearInterpolationRotation(CryLinQKey& key) = 0;
	virtual void HybridInterpolationFloat(CryBez1Key& key) = 0;
	virtual void HybridInterpolationPosition(CryBez3Key& key) = 0;
	virtual void HybridInterpolationPoint3(CryBez3Key& key) = 0;
	virtual void HybridInterpolationScale(CryBez3Key& key) = 0;
	virtual void HybridInterpolationRotation(CryBezQKey& key) = 0;
	virtual void TCBInterpolationFloat(CryTCB1Key& key) = 0;
	virtual void TCBInterpolationPosition(CryTCB3Key& key) = 0;
	virtual void TCBInterpolationPoint3(CryTCB3Key& key) = 0;
	virtual void TCBInterpolationScale(CryTCB3Key& key) = 0;
	virtual void TCBInterpolationRotation(CryTCBQKey& key) = 0;
};

#endif //__ICONTROLLERKEYHANDLER_H__
