////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   3DConnexionDriver.h
//  Version:     v1.00
//  Created:     03/04/2008 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __3DConnexionDriver_h__
#define __3DConnexionDriver_h__
#pragma once

struct S3DConnexionMessage
{
	bool bGotTranslation;
	bool bGotRotation;

	int  raw_translation[3];
	int  raw_rotation[3];

	Vec3 vTranslate;
	Vec3 vRotate;

	unsigned char buttons[3];

	S3DConnexionMessage() : bGotRotation(false),bGotTranslation(false)
	{
		raw_translation[0] = raw_translation[1] = raw_translation[2] = 0;
		raw_rotation[0] = raw_rotation[1] = raw_rotation[2] = 0;
		vTranslate.Set(0,0,0);
		vRotate.Set(0,0,0);
		buttons[0] = buttons[1] = buttons[2] = 0;
	};
};

//////////////////////////////////////////////////////////////////////////
// Implementation of the 3DConnexion device
//////////////////////////////////////////////////////////////////////////
class C3DConnexionDriver : public IPlugin
{
public:
	C3DConnexionDriver();
	~C3DConnexionDriver();

	bool InitDevice();
	
	bool GetInputMessageData( LPARAM lParam,S3DConnexionMessage &msg );

	//////////////////////////////////////////////////////////////////////////
	// IPlugin interface
	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { delete this; };
	virtual void ShowAbout() {};
	virtual const char * GetPluginGUID() { return "{AD109901-9128-4ffd-8E67-137CB2B1C41B}"; };
	virtual DWORD GetPluginVersion() { return 1; };
	virtual const char * GetPluginName() { return "3DConnexionDriver"; };
	virtual bool CanExitNow() { return true; };
	//////////////////////////////////////////////////////////////////////////

private:
	class C3DConnexionDriverImpl* m_pImpl;

	PRAWINPUTDEVICELIST g_pRawInputDeviceList;
	PRAWINPUTDEVICE     g_pRawInputDevices;
	int                 g_nUsagePage1Usage8Devices;

	float m_fMultiplier;
};

#endif // __3DConnexionDriver_h__