//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#include "stdafx.h"
#include "NameList.h"
#include "Utility.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern CSExportUtility theCSExportUtility;

NameList::NameList()
{
	Reset();
}

NameList::~NameList()
{

}

void NameList::Reset()
{
	arrNames.clear();
	nodes.ZeroCount();
	invTM.ZeroCount();
}

int NameList::GetID(INode *node, bool bAppend)
{
	if (!node) return -1;

	for(int i=0;i<nodes.Count();i++)
	{
		if(node==nodes[i]) return i;
	}

	if (!bAppend)
		return -1;

	nodes.Append(1,&node);
	arrNames.push_back(node->GetName());

	TimeValue time = theCSExportUtility.GetTime();
	Matrix3 invmat=node->GetNodeTM(time);
	invmat.NoScale();
	invmat.Invert();
	invTM.Append(1,&invmat);

	return nodes.Count()-1;
}


// ==============================================================
void CHUNK_LIST::Reset()
{
	ch_list.ZeroCount();
	ptr_list.ZeroCount();
}

int CHUNK_LIST::GetID(void* ptr, int ChunkType)
{

	for(int i=0;i<ptr_list.Count();i++)
	{
		if(ptr == ptr_list[i].pRefTarget && ( !ChunkType || ChunkType==ch_list[i].ChunkType )) return ch_list[i].ChunkID;
	}

	return -1;
}

int CHUNK_LIST::GetID_Xept_type(void* ptr, int Xept_ChunkType)
{

	for(int i=0;i<ptr_list.Count();i++)
	{
		if(ptr == ptr_list[i].pRefTarget && Xept_ChunkType!=ch_list[i].ChunkType) return ch_list[i].ChunkID;
	}

	return -1;
}

void* CHUNK_LIST::GetPtr(int ID, int ChunkType)
{
	for(int i=0;i<ch_list.Count();i++)
	{
		if(ID == ch_list[i].ChunkID && ( !ChunkType || ChunkType==ch_list[i].ChunkType )) return ptr_list[i].pRefTarget;
	}

	return NULL;
}

SChunkInfo* CHUNK_LIST::GetInfo(int ID, int ChunkType)
{
	for(int i=0;i<ch_list.Count();i++)
	{
		if(ID == ch_list[i].ChunkID && ( !ChunkType || ChunkType==ch_list[i].ChunkType )) return &(ptr_list[i]);
	}
	return NULL;
}

int	CHUNK_LIST::Append(void *ptr, CHUNK_HEADER *ch, bool assign_ID,int flags)
{
	//check if we already have a chunk for this pointer. If so skip this
	if(ptr && GetID(ptr,ch->ChunkType) != -1) return -1;

	if(assign_ID) ch->ChunkID = ch_list.Count();

	ch_list.Append(1,ch);

	SChunkInfo chi;
	chi.flags = flags;
	chi.pRefTarget = ptr;
	ptr_list.Append(1,&chi);

	return ch_list.Count()-1;
}

