////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   AnimActionActor.h
//  Created:     30/05/2008 by Timur.
//  Modified:    02/07/2008 by Dejan
//  Description: 
// -------------------------------------------------------------------------
//  History:
//    Dejan 02/07/2008: First implementation
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AnimActionActor_h__
#define __AnimActionActor_h__
#pragma once

#include <CryExtension/Impl/ClassWeaver.h>
#include <IAnimAction.h>


class CAnimActionManager;
class CAnimActionActor : public IAnimActionActor
{
public:
	CAnimActionActor();
	~CAnimActionActor();

public:
	void Init(ICharacterInstance* pCharInstance);

	virtual bool StartAnimAction( const char *sAnimAction,SAnimActionStartParams &params );
	virtual bool StopAnimAction( uint32 nLayer );
	virtual int32 GetActiveAnimAction() const;
	virtual IAnimAction* GetActiveIAnimAction() const;

	void Update(); // Called every frame to update animation action actor.

	ICharacterInstance* GetCharacterInstance() const {	return m_pICharInstance;	}
	virtual const QuatT& GetAnimLocation() const { return m_AnimLocation; }
	virtual void SetAnimLocation(const QuatT& qtAnimLocation) { m_AnimLocation = qtAnimLocation; };

	CAnimation* FindInFIFOByAAToken( int32 nLayerID, uint32 nAnimActionToken, int8* index );
	f32 GetTransWeightFromFIFO( const char* strAName, uint32 idx );
	f32 GetAnimTimeFromFIFO( const char* strAName, uint32 idx );
	size_t SizeOfThis (ICrySizer * pSizer);
	void GetMemoryUsage (ICrySizer * pSizer) const;

public:
	Vec3 m_vCamPos;
	QuatT m_AnimLocation;

	uint32 m_nLastAnimationToken;

protected:
	CAnimActionManager* m_pManager;

	static uint32 g_nAnimActionToken;
	ICharacterInstance* m_pICharInstance;
	ISkeletonAnim* m_pISkeletonAnim;
	IAnimationSet* m_pIAnimationSet;
	int32 m_nClassIndex;
	std::vector<IAnimActionPtr> m_Actions;
};



//////////////////////////////////////////////////////////////////////////
inline void DrawAnimActionDebugText( float x,float y, float font_size, ColorF color,const char * label_text, ...) PRINTF_PARAMS(5, 6);

inline void DrawAnimActionDebugText( float x,float y, float font_size, ColorF color,const char * label_text, ...)
{
	va_list args;
	va_start(args,label_text);
	SDrawTextInfo ti;
	ti.xscale = ti.yscale = font_size;
	ti.flags = eDrawText_2D|eDrawText_800x600 | eDrawText_FixedSize;// | eDrawText_Monospace;
	ti.color[0] = color[0]; ti.color[1] = color[1]; ti.color[2] = color[2]; ti.color[3] = color[3];
	gEnv->pRenderer->DrawTextQueued( Vec3(x,y,0.5f),ti,label_text,args );
	va_end(args);
}

#endif //__AnimActionActor_h__
