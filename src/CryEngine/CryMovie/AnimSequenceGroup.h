//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : AnimSequenceGroup.h
//  Author           : Jaewon Jung
//  Time of creation : 5/3/2010   15:21
//  Compilers        : VS2008
//  Description      : Implementation of IAnimSequenceGroup interface. 
//  Notice           : Refer AnimSequence.h, Movie.h also.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __animsequencegroup_h__
#define __animsequencegroup_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"

class CAnimSequenceGroup : public IAnimSequenceGroup
{
public:
	CAnimSequenceGroup( IMovieSystem *pMovieSystem );

	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { if (--m_nRefCounter <= 0) delete this; }
	//////////////////////////////////////////////////////////////////////////

	virtual const char* GetName() const;
	virtual void SetName(const char *name);
	virtual const char* GetFullName() const;
	virtual void SetParent(IAnimSequenceGroup *pParent);
	virtual IAnimSequenceGroup* GetParent() const;
	virtual void SetParentAndName(IAnimSequenceGroup *pParent, const char *name);

	virtual IAnimSequence* GetChildSequence(uint32 i) const;
	virtual uint32 GetChildSequencesCount() const;

	virtual IAnimSequenceGroup* GetChildGroup(uint32 i) const;
	virtual uint32 GetChildGroupsCount() const;

	virtual void Clear();
	virtual void Sort();

	virtual void Serialize(XmlNodeRef &xmlNode, bool bLoading);

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_name);
		pSizer->AddObject(m_childGroups);
		pSizer->AddObject(m_childSequences);
	}

	virtual bool AddChildSequence(IAnimSequence* pSequence);
	virtual bool RemoveChildSequence(IAnimSequence* pSequence);
	virtual bool AddChildGroup(IAnimSequenceGroup* pGroup);
	virtual bool RemoveChildGroup(IAnimSequenceGroup* pGroup);
	virtual void OnRenamed(const char *path) const;

protected:
	string m_name;
	mutable string m_fullNameHolder;

	typedef std::vector< _smart_ptr<IAnimSequenceGroup> > Groups;
	Groups m_childGroups;

	typedef std::vector< _smart_ptr<IAnimSequence> > Sequences;
	Sequences m_childSequences;

	IAnimSequenceGroup *m_pParent;
	IMovieSystem *m_pMovieSystem;
};

#endif // __animsequencegroup_h__
