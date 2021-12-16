#pragma once
#include <vector>
#include "Strings.h"

struct ICharacterInstance;
namespace Serialization { class IArchive; }

namespace CharacterTool
{

using std::vector;
struct SCharacterRigDummyComponent
{
	string name;

	void Serialize(Serialization::IArchive& ar);
};

struct SCharacterRigContent
{
	vector<SCharacterRigDummyComponent> m_components;

	void Serialize(Serialization::IArchive& ar);
	void ApplyToCharacter(ICharacterInstance* instance);
};

}
