////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CGFLoader.h
//  Version:     v1.00
//  Created:     6/11/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CAFLoader_h__
#define __CAFLoader_h__
#pragma once

#include <IChunkFile.h>
#include <CGFContent.h>

//#include <IChunkFile.h>

struct IChunkFile;
class CContentCGF;
struct ChunkDesc;
//////////////////////////////////////////////////////////////////////////

typedef DynArray<PQLog> TrackPQLog;
typedef DynArray<int> TrackTimes;
typedef DynArray<CryTCB3Key> TrackTCB3Keys;
typedef DynArray<CryTCBQKey> TrackTCBQKeys;

typedef TrackPQLog * TrackPQLogPtr;
typedef TrackTimes * TrackTimesPtr;
typedef TrackTCB3Keys * TrackTCB3KeysPtr;
typedef TrackTCBQKeys * TrackTCBQKeysPtr;


class IController;

struct CCommonSkinningInfo/* : public _reference_target_t*/
{
	DynArray</*IControllerAu *>* /	*/IController_AutoPtr> m_pControllers;
	DynArray<uint32> m_arrControllerId;

	int32 m_nTicksPerFrame;
	f32 m_secsPerTick;
	int32 m_nStart;
	int32 m_nEnd;
	f32 m_Speed;
	f32 m_Distance;
	f32 m_Slope;
	int m_nAssetFlags;
	f32 m_LHeelStart,m_LHeelEnd;
	f32 m_LToe0Start,m_LToe0End;
	f32 m_RHeelStart,m_RHeelEnd;
	f32 m_RToe0Start,m_RToe0End;
	Vec3 m_MoveDirection;
	DynArray<uint8> m_FootPlantBits;

	CCommonSkinningInfo() : m_Speed(-1.0f), m_Distance(-1.0f), m_nAssetFlags(0), m_LHeelStart(-10000.0f),m_LHeelEnd(-10000.0f),
		m_LToe0Start(-10000.0f),m_LToe0End(-10000.0f),m_RHeelStart(-10000.0f),m_RHeelEnd(-10000.0f),m_RToe0Start(-10000.0f),m_RToe0End(-10000.0f),m_Slope(0)
	{

	};

};

struct CInternalSkinningInfo : public CCommonSkinningInfo, public _reference_target_t
{
	DynArray<TCBFlags> m_TrackVec3Flags;
	DynArray<TCBFlags> m_TrackQuatFlags;
	DynArray<TrackTCB3KeysPtr> m_TrackVec3;
	DynArray<TrackTCBQKeysPtr > m_TrackQuat;
	DynArray<CControllerType> m_arrControllers;
	DynArray<string> m_arrBoneNameTable;	//names of bones
	uint32 m_numChunks;
	QuatT m_StartLocation;
	bool m_bNewFormat;
	bool m_bOldFormat;
	bool m_bTCBFormat;
	DWORD m_dwTimeStamp;

	CInternalSkinningInfo() : m_bNewFormat(false), m_bOldFormat(false), m_bTCBFormat(false)
	{

		m_dwTimeStamp = 0 ;
	}
};




//////////////////////////////////////////////////////////////////////////
class ILoaderCAFListener
{
public:
	virtual void Warning( const char *format ) = 0;
	virtual void Error( const char *format ) = 0;
};

class CLoaderCAF
{
public:
	CLoaderCAF();
	~CLoaderCAF();

	CInternalSkinningInfo* LoadCAF( const char *filename, ILoaderCAFListener* pListener );
	CInternalSkinningInfo* LoadCAF( const char *filename,IChunkFile * pChunkFile, ILoaderCAFListener* pListener );

	const char *GetLastError() { return m_LastError; }
//	CContentCGF* GetCContentCAF() { return m_pCompiledCGF; } 

	bool m_bLoadOldChunk;

private:
	bool LoadChunks();

	bool ReadTiming (IChunkFile::ChunkDesc *pChunkDesc  );
	bool ReadTimestamp (  IChunkFile::ChunkDesc *pChunkDesc );
	bool ReadController (IChunkFile::ChunkDesc *pChunkDesc  );
	bool ReadBoneNameList( IChunkFile::ChunkDesc *pChunkDesc );
	bool ReadSpeedInfo (IChunkFile::ChunkDesc *pChunkDesc  );
	bool ReadFootPlantInfo(IChunkFile::ChunkDesc *pChunkDesc);
	void Warning(const char* szFormat, ...);


private:
	string m_LastError;

	string m_filename;

	IChunkFile *m_pChunkFile;
	//CContentCGF *m_pCGF;

	CInternalSkinningInfo	* m_pSkinningInfo;

	ILoaderCAFListener* m_pListener;




};

#endif //__CGFLoader_h__
