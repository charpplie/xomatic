#pragma once

#include <Cry_Math.h>
#include "Serialization/Strings.h"

class XmlNodeRef;
class CAnimEventData;
struct AnimEventInstance;

namespace Serialization { class IArchive; }

namespace CharacterTool
{
using Serialization::string;

struct AnimEvent
{
	float startTime;
	float endTime;
	string type;
	string parameter;
	string boneName;
	string model;
	Vec3 offset;
	Vec3 direction;

	AnimEvent()
	: startTime(0.0f)
	, endTime(0.0f)
	, type("sound")
	, offset(ZERO)
	, direction(ZERO)
	{
	}

	bool operator<(const AnimEvent& animEvent) const { return startTime < animEvent.startTime; }

	void Serialize(Serialization::IArchive& ar);
	void FromData(const CAnimEventData& eventData);	
	void ToData(CAnimEventData* eventData) const;	
	void ToInstance(AnimEventInstance* instance) const;	
	bool LoadFromXMLNode(const XmlNodeRef& node);
};
typedef std::vector<AnimEvent> AnimEvents;

struct AnimEventPreset
{
	string name;
	float colorHue;
	AnimEvent event;

	AnimEventPreset() : name("Preset"), colorHue(0.66f) {}

	void Serialize(Serialization::IArchive& ar);
};
typedef std::vector<AnimEventPreset> AnimEventPresets;

bool IsAudioEventType(const char* audioEventType);

}
