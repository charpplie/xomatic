//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxController.h"
#include "IControllerKeyHandler.h"

MaxController::MaxController(Control* pMaxController)
:	pMaxController(pMaxController)
{
}

MaxController::~MaxController()
{
}

CtrlTypes MaxController::GetType()
{
	if (this->pMaxController == 0)
		return CTRL_NONE;
	Class_ID cid = this->pMaxController->ClassID();
	CtrlTypes type = CTRL_NONE;

	if (cid == Class_ID(LININTERP_FLOAT_CLASS_ID,0))
		type = CTRL_LINEER1;
	else if (cid == Class_ID(LININTERP_POSITION_CLASS_ID,0))
		type = CTRL_LINEER3;
	else if (cid == Class_ID(LININTERP_SCALE_CLASS_ID,0))
		type = CTRL_LINEER3;
	else if (cid == Class_ID(LININTERP_ROTATION_CLASS_ID,0))
		type = CTRL_LINEERQ;

	else if (cid == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))
		type = CTRL_BEZIER1;
	else if (cid == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID,0))
		type = CTRL_BEZIER3; 
	else if (cid == Class_ID(HYBRIDINTERP_POINT3_CLASS_ID,0))
		type = CTRL_BEZIER3;   
	else if (cid == Class_ID(HYBRIDINTERP_SCALE_CLASS_ID,0))
		type = CTRL_BEZIER3;
	else if (cid == Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0))
		type = CTRL_BEZIERQ;

	else if (cid == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))
		type = CTRL_TCB1;
	else if (cid == Class_ID(TCBINTERP_POSITION_CLASS_ID,0))
		type = CTRL_TCB3;
	else if (cid == Class_ID(TCBINTERP_POINT3_CLASS_ID,0))
		type = CTRL_TCB3; 
	else if (cid == Class_ID(TCBINTERP_SCALE_CLASS_ID,0))
		type = CTRL_TCB3;
	else if (cid == Class_ID(TCBINTERP_ROTATION_CLASS_ID,0))
		type = CTRL_TCBQ;

	return type;
}

void MaxController::HandleKeys(IControllerKeyHandler* pKeyHandler)
{
	if (this->pMaxController == 0)
		return;
	IKeyControl* ikey = GetKeyControlInterface(this->pMaxController);
	if (ikey == 0)
		return;

	Class_ID cid = this->pMaxController->ClassID();

	for (int i = 0; i < ikey->GetNumKeys(); i++)
	{
		if(cid==Class_ID(LININTERP_FLOAT_CLASS_ID,0))
		{
			ILinFloatKey key;		ikey->GetKey(i, &key); 
			CryLin1Key ck; 
			ck.time	= key.time;
			ck.val	= key.val;
			pKeyHandler->LinearInterpolationFloat(ck);
		}
		else if(cid==Class_ID(LININTERP_POSITION_CLASS_ID,0))
		{
			ILinPoint3Key key;		ikey->GetKey(i, &key); 
			CryLin3Key ck;
			ck.time	= key.time;
			ck.val	= Vec3(key.val.x, key.val.y, key.val.z);
			pKeyHandler->LinearInterpolationPosition(ck);
		}
		else if(cid==Class_ID(LININTERP_SCALE_CLASS_ID,0))
		{
			ILinScaleKey key;		ikey->GetKey(i, &key); 
			CryLin3Key ck;
			ck.time	= key.time;
			ck.val	= Vec3(key.val.s.x, key.val.s.y, key.val.s.z);
			pKeyHandler->LinearInterpolationScale(ck);
		}
		else if(cid==Class_ID(LININTERP_ROTATION_CLASS_ID,0))
		{
			ILinRotKey key;			ikey->GetKey(i, &key); 
			CryLinQKey ck;
			ck.time	= key.time;
			ck.val	= CryQuat(key.val.w,key.val.x, key.val.y, key.val.z);
			pKeyHandler->LinearInterpolationRotation(ck);
		}
		//======================= HYBRID ===================================
		else if(cid==Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))
		{
			IBezFloatKey key;		ikey->GetKey(i, &key); 
			CryBez1Key ck; 
			ck.time		= key.time;
			ck.val		= key.val;
			ck.intan	= key.intan;
			ck.outtan	= key.outtan;
			pKeyHandler->HybridInterpolationFloat(ck);
		}
		else if(cid==Class_ID(HYBRIDINTERP_POSITION_CLASS_ID,0))
		{
			IBezPoint3Key key;		ikey->GetKey(i, &key); 
			CryBez3Key ck; 
			ck.time		= key.time;
			ck.val		= Vec3(key.val.x, key.val.y, key.val.z);
			ck.intan	= Vec3(key.intan.x, key.intan.y, key.intan.z);
			ck.outtan	= Vec3(key.outtan.x, key.outtan.y, key.outtan.z);
			pKeyHandler->HybridInterpolationPosition(ck);
		}
		else if(cid==Class_ID(HYBRIDINTERP_POINT3_CLASS_ID,0))
		{
			IBezPoint3Key key;		ikey->GetKey(i, &key); 
			CryBez3Key ck; 
			ck.time		= key.time;
			ck.val		= Vec3(key.val.x, key.val.y, key.val.z);
			ck.intan	= Vec3(key.intan.x, key.intan.y, key.intan.z);
			ck.outtan	= Vec3(key.outtan.x, key.outtan.y, key.outtan.z);
			pKeyHandler->HybridInterpolationPoint3(ck);
		}
		else if(cid==Class_ID(HYBRIDINTERP_SCALE_CLASS_ID,0))
		{
			IBezScaleKey key;		ikey->GetKey(i, &key); 
			CryBez3Key ck; 
			ck.time		= key.time;
			ck.val		= Vec3(key.val.s.x, key.val.s.y, key.val.s.z);
			ck.intan	= Vec3(key.intan.x, key.intan.y, key.intan.z);
			ck.outtan	= Vec3(key.outtan.x, key.outtan.y, key.outtan.z);
			pKeyHandler->HybridInterpolationScale(ck);
		}
		else if(cid==Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0))
		{
			IBezQuatKey key;		ikey->GetKey(i, &key); 
			CryBezQKey ck; 
			ck.time		= key.time;
			ck.val		= CryQuat(key.val.w,key.val.x, key.val.y, key.val.z);
			pKeyHandler->HybridInterpolationRotation(ck);
		}
		//======================== TCB ==================================
		else if(cid==Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))
		{
			ITCBFloatKey key;		ikey->GetKey(i, &key); 
			CryTCB1Key ck; 
			ck.time		= key.time;
			ck.val		= key.val;
			ck.t		= key.tens;
			ck.c		= key.cont;
			ck.b		= key.bias;
			ck.ein		= key.easeIn;
			ck.eout		= key.easeOut;
			pKeyHandler->TCBInterpolationFloat(ck);
		}
		else if(cid==Class_ID(TCBINTERP_POSITION_CLASS_ID,0))
		{
			ITCBPoint3Key key;		ikey->GetKey(i, &key); 
			CryTCB3Key ck; 
			ck.time		= key.time;
			ck.val		= Vec3(key.val.x, key.val.y, key.val.z);
			ck.t		= key.tens;
			ck.c		= key.cont;
			ck.b		= key.bias;
			ck.ein		= key.easeIn;
			ck.eout		= key.easeOut;
			pKeyHandler->TCBInterpolationPosition(ck);
		}
		else if (cid==Class_ID(TCBINTERP_POINT3_CLASS_ID,0))
		{
			ITCBPoint3Key key;		ikey->GetKey(i, &key); 
			CryTCB3Key ck; 
			ck.time		= key.time;
			ck.val		= Vec3(key.val.x, key.val.y, key.val.z);
			ck.t		= key.tens;
			ck.c		= key.cont;
			ck.b		= key.bias;
			ck.ein		= key.easeIn;
			ck.eout		= key.easeOut;
			pKeyHandler->TCBInterpolationPoint3(ck);
		}
		else if(cid==Class_ID(TCBINTERP_SCALE_CLASS_ID,0))
		{
			ITCBScaleKey key;		ikey->GetKey(i, &key); 
			CryTCB3Key ck; 
			ck.time		= key.time;
			ck.val		= Vec3(key.val.s.x, key.val.s.y, key.val.s.z);
			ck.t		= key.tens;
			ck.c		= key.cont;
			ck.b		= key.bias;
			ck.ein		= key.easeIn;
			ck.eout		= key.easeOut;
			pKeyHandler->TCBInterpolationScale(ck);
		}
		else if(cid==Class_ID(TCBINTERP_ROTATION_CLASS_ID,0))
		{
			ITCBRotKey key;		ikey->GetKey(i, &key); 
			// Export relative Angle Axis.
			//Quat q(key.val);
			CryTCBQKey ck; 
			ck.time		= key.time;
			ck.val	= CryQuat(key.val.angle,key.val.axis.x,key.val.axis.y,key.val.axis.z);
			ck.t		= key.tens;
			ck.c		= key.cont;
			ck.b		= key.bias;
			ck.ein		= key.easeIn;
			ck.eout		= key.easeOut;
			pKeyHandler->TCBInterpolationRotation(ck);
		}
	}
}

unsigned int MaxController::GetFlags()
{
	if (this->pMaxController == 0)
		return 0;
	unsigned int nFlags = 0;
	int ort = this->pMaxController->GetORT(ORT_AFTER);
	if (ort == ORT_CYCLE)
		nFlags |= CTRL_ORT_CYCLE;
	else if (ort == ORT_LOOP)
		nFlags |= CTRL_ORT_LOOP;
	return nFlags;
}

int MaxController::KeyCount()
{
	if (this->pMaxController == 0)
		return 0;
	IKeyControl* ikey = GetKeyControlInterface(this->pMaxController);
	if (!ikey) 
		return 0;

	return ikey->GetNumKeys();
}
