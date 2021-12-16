#pragma once
#include <vector>
#include "Strings.h"

struct ICharacterInstance;
namespace Serialization { class IArchive; }

namespace CharacterTool
{
using std::vector;

struct SPhysicsComponent
{
	bool enabled;
	string name;

	void Serialize(Serialization::IArchive& ar);
};

struct SCharacterPhysicsContent
{
	vector<SPhysicsComponent> m_components;

	void Serialize(Serialization::IArchive& ar);

	void ApplyToCharacter(ICharacterInstance* instance);
};

}
