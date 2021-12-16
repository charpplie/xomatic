//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxHelperObject.h"

MaxHelperObject::MaxHelperObject(INode* pMaxNode)
:	pMaxNode(pMaxNode)
{
}

HelperTypes MaxHelperObject::GetType()
{
	Object *der_obj = this->pMaxNode->GetObjectRef();
	if (!der_obj)
	{
		assert(false);
		return HP_POINT;
	}

	Object *obj = der_obj->Eval(0).obj;

	Class_ID cid = obj->ClassID();
	if (this->pMaxNode->SuperClassID()==SYSTEM_CLASS_ID && this->pMaxNode->ClassID()==Class_ID(XREFOBJ_CLASS_ID))
	{
		return HP_XREF;
	}
	else if (cid==Class_ID(POINTHELP_CLASS_ID, 0))	
	{
		return HP_POINT;
	}
	else if(cid==Class_ID(DUMMY_CLASS_ID, 0))
	{
		return HP_DUMMY;
	}
	else if(cid==Class_ID(CAMERA_CLASS_ID, 0))
	{
		return HP_CAMERA;
	}
	else
	{
		assert(false);
		return HP_POINT;
	}
}
