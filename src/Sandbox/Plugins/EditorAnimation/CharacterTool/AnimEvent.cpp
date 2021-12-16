#include "AnimEvent.h"
#include <ICryAnimation.h>
#include "Serialization.h"
#include "Serialization/Decorators/ResourcesAudio.h"

namespace {

using Serialization::IArchive;

enum EUsedParameters
{
	USES_BONE = 1 << 0,
	USES_BONE_INLINE = 1 << 1,
	USES_OFFSET_AND_DIRECTION = 1 << 2,
};

void SerializeParameterString(CharacterTool::AnimEvent& ev, IArchive& ar) { ar(ev.parameter, "parameter", "^"); }
void SerializeParameterEffect(CharacterTool::AnimEvent& ev, IArchive& ar) { ar(Serialization::ParticleName(ev.parameter), "parameter", "^"); }
void SerializeParameterNone(CharacterTool::AnimEvent& ev, IArchive& ar) { ar(ev.parameter, "parameter", 0); }
void SerializeParameterAudioTrigger(CharacterTool::AnimEvent& ev, IArchive& ar)  { ar(Serialization::AudioTrigger(ev.parameter), "parameter", "^"); }
void SerializeParameterAudioEnvironment(CharacterTool::AnimEvent& ev, IArchive& ar)  { ar(Serialization::AudioEnvironment(ev.parameter), "parameter", "^"); }
void SerializeParameterAudioPreload(CharacterTool::AnimEvent& ev, IArchive& ar)  { ar(Serialization::AudioPreloadRequest(ev.parameter), "parameter", "^"); }

static void SplitSwitchParameter(string* switchName, string* switchValue, const char* parameter)
{
	const char* sep = strchr(parameter, '=');
	if (!sep)
	{
		*switchName = parameter;
		*switchValue = string();
	}
	else
	{
		*switchName = string(parameter, sep);
		*switchValue = sep+1;
	}
}

void SerializeParameterAudioSwitch(CharacterTool::AnimEvent& ev, IArchive& ar) 
{
	string switchName; 
	string switchState;
	SplitSwitchParameter(&switchName, &switchState, ev.parameter.c_str());
	ar(Serialization::AudioSwitch(switchName), "switchName", "^"); 
	ar(Serialization::AudioSwitchState(switchState), "switchState", "^"); 
	if (ar.IsInput())
		ev.parameter = switchName + "=" + switchState;
}

typedef void(*TSerializeParameterFunc)(CharacterTool::AnimEvent& ev, IArchive&);
struct SEventType
{
	const char* name;
	TSerializeParameterFunc serializeParameterFunc;
	int usage;
};

enum { EVENT_TYPE_COUNT = 12 };
typedef SEventType (SEventTypeArray)[EVENT_TYPE_COUNT];
static SEventTypeArray& GetEventTypes();

SEventType& FindEventType(const char* name)
{
	SEventTypeArray& types = GetEventTypes();
	for (size_t i = 1; i < EVENT_TYPE_COUNT; ++i)
	{
		if (stricmp(name, types[i].name) == 0)
			return types[i];
	}
	return types[0];
}

static const Serialization::StringList& GetTypeList()
{
	static Serialization::StringList result;
	if (result.empty())
	{
		const SEventTypeArray& types = GetEventTypes();
		result.push_back("Custom");
		for (int i = 1; i < EVENT_TYPE_COUNT; ++i)
			result.push_back(types[i].name);
	}
	return result;
}

static SEventTypeArray& GetEventTypes()
{
	static SEventTypeArray result = {
		{ 0, &SerializeParameterString, USES_BONE | USES_OFFSET_AND_DIRECTION }, // custom
		{ "effect", &SerializeParameterEffect, USES_BONE | USES_OFFSET_AND_DIRECTION },
		{ "foley", &SerializeParameterString,  USES_BONE },
		{ "footstep", &SerializeParameterString, USES_BONE },
		{ "sound", &SerializeParameterString, USES_BONE },
		{ "audio_trigger", &SerializeParameterAudioTrigger, USES_BONE },
		{ "audio_environment", &SerializeParameterAudioEnvironment },
		{ "audio_preload", &SerializeParameterAudioPreload },
		{ "audio_switch", &SerializeParameterAudioSwitch },
		{ "segment1", &SerializeParameterString },
		{ "segment2", &SerializeParameterString },
		{ "segment3", &SerializeParameterString }
	};
	return result;
}

}

namespace CharacterTool
{
			
bool IsAudioEventType(const char* type)
{
	static const char* audioEvents[] = { "sound", "foley", "footstep" };
	for (int i = 0; i < sizeof audioEvents / sizeof audioEvents[0]; ++i)
		if (stricmp(type, audioEvents[i]))
			return true;
	return false;
}

void AnimEvent::Serialize(IArchive& ar)
{
	if(!ar.IsEdit())
	{
		ar(startTime, "startTime");
		ar(endTime, "endTime");
		ar(type, "type");
		ar(parameter, "parameter");
		ar(boneName, "boneName");
		ar(offset, "offset");
		ar(direction, "direction");
		ar(model, "model");
	}
	else
	{
		if (startTime >= 0.0f)
		{
			ar(startTime, "startTime", ">50>^");
			ar(endTime, "endTime", 0);
			if (ar.IsInput())
			{
				if(startTime < 0.0f)
					startTime = 0.0f;
				if (endTime < 0.0f)
					endTime = 0.0f;
			}
		}

		{
			SEventType& eventType = FindEventType(type.c_str());
			const Serialization::StringList& eventList = GetTypeList();
			string typeLower = type;
			typeLower.MakeLower();
			int typeIndex = eventList.find(typeLower.c_str());
			Serialization::StringListValue typeChoice(eventList, eventType.name == 0 || typeIndex == -1 ? 0 : typeIndex);
			int oldIndex = typeChoice.index();
			ar(typeChoice, "typeChoice", "^");
			if (ar.IsInput() && oldIndex != typeChoice.index())
			{
				if (typeChoice.index() > 0)
					type = typeChoice.c_str();
				else if (FindEventType(type.c_str()).name != 0)
					type.clear();
			}
			if (typeChoice.index() <= 0) // Custom
				ar(type, "type", "^"); 
		}
		SEventType& eventType = FindEventType(type.c_str());

		if (eventType.name)
			eventType.serializeParameterFunc(*this, ar);
		else
			ar(parameter, "parameter", "Parameter");

		if (eventType.usage & USES_BONE_INLINE)
			ar(JointName(boneName), "boneName", "^");
		else if (eventType.usage & USES_BONE)
			ar(JointName(boneName), "boneName", "Bone Name");
		else
			ar(JointName(boneName), "boneName", 0);

		if (eventType.usage & USES_OFFSET_AND_DIRECTION)
		{
			ar(LocalToJoint(offset, boneName), "offset", "Offset");
			ar(LocalToJoint(direction, boneName), "direction", "Direction");
		}
		else
		{
			ar(LocalToJoint(offset, boneName), "offset", 0);
			ar(LocalToJoint(direction, boneName), "direction", 0);
		}

		ar(model, "model");
	}		
}

void AnimEvent::FromData(const CAnimEventData& eventData)
{
	startTime = eventData.GetNormalizedTime();
	endTime = eventData.GetNormalizedEndTime();
	type = eventData.GetName();
	parameter = eventData.GetCustomParameter();
	boneName = eventData.GetBoneName();
	offset = eventData.GetOffset();
	direction = eventData.GetDirection();
	model = eventData.GetModelName();
}

void AnimEvent::ToData(CAnimEventData* data) const
{
	data->SetNormalizedTime(startTime);
	data->SetNormalizedEndTime(endTime);
	data->SetName(type.c_str());
	data->SetCustomParameter(parameter.c_str());
	data->SetBoneName(boneName.c_str());
	data->SetOffset(offset);
	data->SetDirection(direction);
	data->SetModelName(model.c_str());
}

void AnimEvent::ToInstance(AnimEventInstance* instance) const
{
	instance->m_time = startTime;
	instance->m_endTime = endTime;
	instance->m_EventName = type.c_str();
	instance->m_EventNameLowercaseCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(type.c_str());
	instance->m_CustomParameter = parameter.c_str();
	instance->m_BonePathName = boneName.c_str();
	instance->m_vOffset = offset;
	instance->m_vDir = direction;
}

bool AnimEvent::LoadFromXMLNode(const XmlNodeRef& dataIn)
{
	if ( !dataIn )
		return false;
	if ( stack_string( "event" ) != dataIn->getTag() )
		return false;

	type = dataIn->getAttr( "name" );
	dataIn->getAttr( "time", startTime = 0.0f );
	dataIn->getAttr( "endTime", endTime = startTime );
	parameter = dataIn->getAttr( "parameter" );
	boneName = dataIn->getAttr( "bone" );
	dataIn->getAttr( "offset", offset = Vec3(0, 0, 0) );
	dataIn->getAttr( "dir", direction = Vec3(0, 0, 0) );
	model = dataIn->getAttr( "model" );
	return true;
}

void AnimEventPreset::Serialize(IArchive& ar)
{
	ar(name, "name", "Name");
	ar(Serialization::Decorators::Slider(colorHue, 0.0f, 1.0f), "colorHue", "Color Hue");
	ar(event, "event", "<Event");
	if (ar.IsInput())
	{
		// a little hack to hide time from presets
		event.startTime = -1.0f;
		event.endTime = -1.0f;
	}
}

}
