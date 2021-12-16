//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#if !defined(AFX_NAMELIST_H__D3723895_346C_47D3_9A29_0510BE0071AD__INCLUDED_)
#define AFX_NAMELIST_H__D3723895_346C_47D3_9A29_0510BE0071AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include "IChunkList.h"

class NameList  
{
public:
	std::vector<std::string> arrNames;
	INodeTab			nodes;
	Tab<Matrix3>		invTM;

	NameList();
	virtual ~NameList();

	int GetID(INode *node, bool bAppend = true);
	void Reset();
};

enum EChunkFlags
{
	CHUNK_FLAG_SUB_MATERIAL = 0x0001,
};

// Additional info per chunk.
struct SChunkInfo
{
	int flags;
	void* pRefTarget;
};

class CHUNK_LIST : public IChunkList
{
public:
	Tab<SChunkInfo>   ptr_list;
	Tab<CHUNK_HEADER> ch_list;

	CHUNK_LIST()			{Reset();};
	virtual ~CHUNK_LIST()	{};

	int GetID(void *ptr,int ChunkType);
	int GetID_Xept_type(void *ptr,int Xept_ChunkType);
	void* GetPtr(int ID, int ChunkType);
	SChunkInfo* GetInfo(int ID, int ChunkType);

	int	Append(void *ptr, CHUNK_HEADER *ch, bool assign_ID,int flags=0);
	void Reset();
};
#endif // !defined(AFX_NAMELIST_H__D3723895_346C_47D3_9A29_0510BE0071AD__INCLUDED_)
