////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   FaceJoystick.h
//  Version:     v1.00
//  Created:     7/8/2006 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FACEJOYSTICK_H__
#define __FACEJOYSTICK_H__

#include "IJoystick.h"
#include "smartptr.h"

struct IFacialAnimChannel;

class IFacialJoystickSerializeContext
{
public:
	virtual IFacialAnimChannel* FindChannel(const char* szName) = 0;
};

class CFacialJoystickChannel : public IJoystickChannel
{
public:
	CFacialJoystickChannel(const string& channelName);
	CFacialJoystickChannel(IFacialAnimChannel* pChannel);

	// IJoystickChannel
	virtual void AddRef();
	virtual void Release();

	VIRTUAL const char* GetName() const;

	VIRTUAL void* GetTarget();
	VIRTUAL int GetSplineCount();
	VIRTUAL ISplineInterpolator* GetSpline(int splineIndex);

	VIRTUAL void SetFlipped(bool flipped);
	VIRTUAL bool GetFlipped() const;

	VIRTUAL void SetVideoMarkerOffset(float offset);
	VIRTUAL float GetVideoMarkerOffset() const;
	VIRTUAL void SetVideoMarkerScale(float scale);
	VIRTUAL float GetVideoMarkerScale() const;

	VIRTUAL void CleanupKeys(float fErrorMax);

	const char* GetPath() const;

	IFacialAnimChannel* GetFacialChannel();

	void Bind(IFacialJoystickSerializeContext* pContext);

private:
	IFacialAnimChannel* m_pChannel;
	string m_channelName;
	int m_refCount;
	bool m_bFlipped;
	float m_videoMarkerOffset;
	float m_videoMarkerScale;
};

class CFacialJoystick : public IJoystick
{
public:
	CFacialJoystick(uint64 id);

	// IJoystick
	virtual void AddRef();
	virtual void Release();

	VIRTUAL uint64 GetID() const;

	VIRTUAL void SetName(const char* szName);
	VIRTUAL const char* GetName();

	VIRTUAL IJoystickChannel* GetChannel(ChannelType type);
	VIRTUAL void SetChannel(ChannelType type, IJoystickChannel* pChannel);

	VIRTUAL const Vec2& GetCentre() const;
	VIRTUAL void SetCentre(const Vec2& vCentre);
	VIRTUAL const Vec2& GetDimensions() const;
	VIRTUAL void SetDimensions(const Vec2& vDimensions);

	CFacialJoystickChannel* GetDerivedChannel(ChannelType type);

	void Bind(IFacialJoystickSerializeContext* pContext);

	VIRTUAL void SetColor(const Color& colour);
	VIRTUAL IJoystick::Color GetColor() const;

private:
	_smart_ptr<CFacialJoystickChannel> m_channels[2];
	uint64 m_id;
	Vec2 m_vCentre;
	Vec2 m_vDimensions;
	int m_refCount;
	string m_name;
	Color m_colour;
};

class CFacialJoystickSet : public IJoystickSet
{
public:
	CFacialJoystickSet();

	void HandleRemovedChannel(IFacialAnimChannel *pChannel);

	// IJoystickSet
	virtual void AddRef();
	virtual void Release();

	VIRTUAL void SetName(const char* name);
	VIRTUAL const char* GetName() const;
	VIRTUAL void AddJoystick(IJoystick* pJoystick);
	VIRTUAL void RemoveJoystick(IJoystick* pJoystick);
	VIRTUAL int GetJoystickCount() const;
	VIRTUAL IJoystick* GetJoystick(int index);
	VIRTUAL IJoystick* GetJoystickAtPoint(const Vec2& vPosition);
	VIRTUAL IJoystick* GetJoystickByID(uint64 id);

	VIRTUAL void Serialize(XmlNodeRef& node, bool bLoading);

	void Bind(IFacialJoystickSerializeContext* pContext);

private:
	typedef std::vector<_smart_ptr<CFacialJoystick> > JoystickContainer;
	JoystickContainer m_joysticks;
	int m_refCount;
	string m_name;
};

#endif //__FACEJOYSTICK_H__
