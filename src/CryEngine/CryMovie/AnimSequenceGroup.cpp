//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : AnimSequenceGroup.cpp
//  Author           : Jaewon Jung
//  Time of creation : 5/3/2010   15:21
//  Compilers        : VS2008
//  Description      : Implementation of IAnimSequenceGroup interface. 
//  Notice           : Refer AnimSequence.h, Movie.h also.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimSequenceGroup.h"
#include "StlUtils.h"

CAnimSequenceGroup::CAnimSequenceGroup( IMovieSystem *pMovieSystem )
	: m_pMovieSystem(pMovieSystem), m_pParent(NULL)
{
}

const char* CAnimSequenceGroup::GetName() const
{
	return m_name.c_str();
}

void CAnimSequenceGroup::OnRenamed(const char *path) const
{
	for(uint i=0; i<GetChildSequencesCount(); ++i)
	{
		string before = path;
		before += "/";
		string name = GetChildSequence(i)->GetName();
		before += name;
		string fullname = GetChildSequence(i)->GetFullName();
		m_pMovieSystem->OnSequenceRenamed(before, fullname.c_str());
		if(GetChildSequence(i)->GetOwner())
			GetChildSequence(i)->GetOwner()->OnModified();
	}

	for(uint i=0; i<GetChildGroupsCount(); ++i)
	{
		string pathExtended = path;
		pathExtended += "/";
		string name = GetChildSequence(i)->GetName();
		pathExtended += name;
		GetChildGroup(i)->OnRenamed(pathExtended.c_str());
	}
}

void CAnimSequenceGroup::SetName(const char *name)
{
	string name0 = GetFullName();

	m_name = name;

	OnRenamed(name0.c_str());
}

const char* CAnimSequenceGroup::GetFullName() const
{
	if(GetParent() == NULL)	// An edge case of the root group
		return "";						// The root group has no name.

	m_fullNameHolder = m_name;
	IAnimSequenceGroup *pGroup = m_pParent;
	while(pGroup 
		&& pGroup->GetParent())	// To skip the name of the root node
	{
		m_fullNameHolder = string(pGroup->GetName()) + "/" + m_fullNameHolder;
		pGroup = pGroup->GetParent();
	}

	return m_fullNameHolder.c_str();
}

void CAnimSequenceGroup::SetParent(IAnimSequenceGroup *pParent)
{
	assert(pParent);
	assert(m_pParent);

	string name0 = GetFullName();

	if(m_pParent)
	{
		bool ok = m_pParent->RemoveChildGroup(this);
		assert(ok);
	}

	m_pParent = pParent;

	if(m_pParent)
	{
		bool ok = m_pParent->AddChildGroup(this);
		assert(ok);
	}

	OnRenamed(name0.c_str());
}

void CAnimSequenceGroup::SetParentAndName(IAnimSequenceGroup *pParent, const char *name)
{
	assert(pParent);

	string name0 = GetFullName();

	if(m_pParent)
	{
		bool ok = m_pParent->RemoveChildGroup(this);
		assert(ok);
	}

	m_pParent = pParent;
	m_name = name;

	if(m_pParent)
	{
		bool ok = m_pParent->AddChildGroup(this);
		assert(ok);
	}

	OnRenamed(name0.c_str());
}

IAnimSequenceGroup* CAnimSequenceGroup::GetParent() const
{
	return m_pParent;
}

bool CAnimSequenceGroup::AddChildSequence(IAnimSequence* pSequence)
{
	return stl::push_back_unique(m_childSequences, pSequence);
}

bool CAnimSequenceGroup::RemoveChildSequence(IAnimSequence* pSequence)
{
	return stl::find_and_erase(m_childSequences, pSequence);
}

IAnimSequence* CAnimSequenceGroup::GetChildSequence(uint32 i) const
{
	assert(i < GetChildSequencesCount());

	if(i < GetChildSequencesCount())
		return m_childSequences[i];
	else
		return NULL;
}

uint32 CAnimSequenceGroup::GetChildSequencesCount() const
{
	return (uint32)m_childSequences.size();
}

bool CAnimSequenceGroup::AddChildGroup(IAnimSequenceGroup* pGroup)
{
	return stl::push_back_unique(m_childGroups, pGroup);
}

bool CAnimSequenceGroup::RemoveChildGroup(IAnimSequenceGroup* pGroup)
{
	return stl::find_and_erase(m_childGroups, pGroup);
}

IAnimSequenceGroup* CAnimSequenceGroup::GetChildGroup(uint32 i) const
{
	assert(i < GetChildGroupsCount());

	if(i < GetChildGroupsCount())
		return m_childGroups[i];
	else
		return NULL;
}

uint32 CAnimSequenceGroup::GetChildGroupsCount() const
{
	return (uint32)m_childGroups.size();
}

void CAnimSequenceGroup::Clear()
{
	stl::free_container(m_childGroups);
	stl::free_container(m_childSequences);
}

template <typename T>
struct LexicoCompare : public std::binary_function<_smart_ptr<T>,_smart_ptr<T>,bool> 
{
	bool operator()( _smart_ptr<T> a1, _smart_ptr<T> a2 ) const
	{
		string a1Str = a1->GetFullName();
		string a2Str = a2->GetFullName();
		return stricmp(a1Str.c_str(), a2Str.c_str()) < 0;
	}
};

void CAnimSequenceGroup::Sort()
{
	std::sort(m_childGroups.begin(), m_childGroups.end(), LexicoCompare<IAnimSequenceGroup>());
	std::sort(m_childSequences.begin(), m_childSequences.end(), LexicoCompare<IAnimSequence>());

	for(uint32 i=0; i<GetChildGroupsCount(); ++i)
		GetChildGroup(i)->Sort();
}

void CAnimSequenceGroup::Serialize(XmlNodeRef &xmlNode, bool bLoading)
{
	if(bLoading)
	{
		Clear();

		const char *name = xmlNode->getAttr( "Name" );
		SetName(name);
		
		XmlNodeRef childGroups = xmlNode->findChild("ChildGroups");
		if(childGroups)
		{
			for(int i=0; i<childGroups->getChildCount(); ++i)
			{
				XmlNodeRef childNode = childGroups->getChild(i);
				const char * childName = childNode->getAttr("Name");
				IAnimSequenceGroup *pNewGroup = m_pMovieSystem->CreateSequenceGroup(childName, this);
				pNewGroup->Serialize(childNode, bLoading);
			}
		}


		XmlNodeRef childSequences = xmlNode->findChild("ChildSequences");
		if(childSequences)
		{
			for(int i=0; i<childSequences->getChildCount(); ++i)
			{
				XmlNodeRef childNode = childSequences->getChild(i);
				XmlString seqName = childNode->getAttr("Name");
				XmlString seqFullName = seqName;
				if(GetParent())	// We should skip the name of the dummy root node.
				{
					string fullname = GetFullName();
					seqFullName = XmlString(XmlString(fullname.c_str()) + "/" + seqName);
				}
				IAnimSequence *pSequence = m_pMovieSystem->FindSequence(seqFullName.c_str());
				// If the sequence has been already loaded, set up the relationship.
				if(pSequence)
					pSequence->SetGroupAndName(this, seqName.c_str());
			}
		}
	}
	else
	{
		xmlNode->setAttr("Name", m_name.c_str());
		
		XmlNodeRef childGroups = xmlNode->newChild("ChildGroups");
		for(uint32 i=0; i<GetChildGroupsCount(); ++i)
		{
			XmlNodeRef childNode = childGroups->newChild("SequenceGroup");
			GetChildGroup(i)->Serialize(childNode, bLoading);
		}

		// For sequences, just save it name.
		XmlNodeRef childSequences = xmlNode->newChild("ChildSequences");
		for(uint32 i=0; i<GetChildSequencesCount(); ++i)
		{
			XmlNodeRef childNode = childSequences->newChild("Sequence");
			string name = GetChildSequence(i)->GetName();
			childNode->setAttr("Name", name.c_str());
		}
	}
}