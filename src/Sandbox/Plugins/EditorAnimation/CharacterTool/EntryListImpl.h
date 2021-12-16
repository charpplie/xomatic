#pragma once

#include "EntryList.h"
#include "Serialization.h"

namespace CharacterTool
{

template<class T>
void SEntry<T>::Serialize(Serialization::IArchive& ar)
{
	Serialization::SContext<EntryBase> entryContext(ar, this); 
	Serialization::SNavigationContext nav;
	nav.path = path;
	Serialization::SContext<Serialization::SNavigationContext> navContext(ar, &nav); 

	content.Serialize(ar);
}

}