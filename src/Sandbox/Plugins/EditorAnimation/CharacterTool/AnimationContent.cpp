#include "AnimationContent.h"
#include "EditorCompressionPresetTable.h"
#include "EditorDBATable.h"
#include "Shared/AnimationFilter.h"
#include "CharacterDocument.h"
#include <ICryAnimation.h>
#include "Serialization.h"
#include "EntryList.h"

namespace CharacterTool {

AnimationContent::AnimationContent()
: type(ANIMATION)
, size(-1)
, importState(NOT_SET)
, loadedInEngine(false)
, loadedAsAdditive(false)
, animationId(-1)
{
}

bool AnimationContent::HasAudioEvents() const
{
	for (size_t i = 0; i < events.size(); ++i)
		if (IsAudioEventType(events[i].type.c_str()))
			return true;
	return false;
}

void AnimationContent::ApplyToCharacter(bool* triggerPreview, ICharacterInstance* characterInstance, const char* animationPath)
{
	IAnimEvents* animEvents = gEnv->pCharacterManager->GetIAnimEvents();

	if (IAnimEventList* animEventList = animEvents->GetAnimEventList(animationPath))
	{
		animEventList->Clear();
		for (size_t i = 0; i < events.size(); ++i)
		{
			CAnimEventData animEvent;
			events[i].ToData(&animEvent);
			animEventList->Append(animEvent);
		}
	}

	animEvents->InitializeSegmentationDataFromAnimEvents(animationPath);

	if (type == BLEND_SPACE)
	{
		XmlNodeRef xml = blendSpace.SaveToXml();
		_smart_ptr<IXmlStringData> content = xml->getXMLData();
		gEnv->pCharacterManager->InjectBSPACE(animationPath, content->GetString(), content->GetStringLength());
		gEnv->pCharacterManager->ReloadLMG(animationPath);
		gEnv->pCharacterManager->ClearBSPACECache();
		if (triggerPreview)
			*triggerPreview = true;
	}
	if (type == COMBINED_BLEND_SPACE)
	{
		XmlNodeRef xml = combinedBlendSpace.SaveToXml();
		_smart_ptr<IXmlStringData> content = xml->getXMLData();
		gEnv->pCharacterManager->InjectBSPACE(animationPath, content->GetString(), content->GetStringLength());
		gEnv->pCharacterManager->ReloadLMG(animationPath);
		gEnv->pCharacterManager->ClearBSPACECache();
		if (triggerPreview)
			*triggerPreview = true;
	}
	
}

void AnimationContent::UpdateBlendSpaceMotionParameters(IAnimationSet* animationSet, IDefaultSkeleton* skeleton)
{
	for (size_t i = 0; i < blendSpace.m_examples.size(); ++i)
	{
		BlendSpaceExample& e = blendSpace.m_examples[i];
		Vec4 v;
		if (animationSet->GetMotionParameters(animationId, i, skeleton, v))
		{
			if (!e.specified[0]) e.parameters.x = v.x;
			if (!e.specified[1]) e.parameters.y = v.y;
			if (!e.specified[2]) e.parameters.z = v.z;
			if (!e.specified[3]) e.parameters.w = v.w;
		}
	}
}

void AnimationContent::Serialize(Serialization::IArchive& ar)
{
	if (type == ANIMATION)
	{
		ar(settings.build.additive, "additive", "Additive");
	}
	if (type == ANIMATION && importState == NEW_ANIMATION)
	{
		ar(SkeletonAlias(newAnimationSkeleton), "skeletonAlias", "Skeleton Alias");
		if (newAnimationSkeleton.empty())
			ar.Warning(newAnimationSkeleton, "Skeleton alias should be specified in order to import animation.");
	}

	if ((type == ANIMATION && importState != NEW_ANIMATION) || type == AIMPOSE || type == LOOKPOSE)
	{
		ar(SkeletonAlias(settings.build.skeletonAlias), "skeletonAlias", "Skeleton Alias");
		if (settings.build.skeletonAlias.empty())
			ar.Error(settings.build.skeletonAlias, "Skeleton alias is not specified for the animation.");
		ar(TagList(settings.build.tags), "tags", "Tags");
	}

	if (type == ANIMATION && importState != NEW_ANIMATION)
	{
		bool presetApplied = false;
		if (ar.IsEdit() && ar.IsOutput())
		{
			if (EntryBase* entryBase = ar.FindContext<EntryBase>())
			{
				SAnimationFilterItem item;
				item.path = entryBase->path;
				item.skeletonAlias = settings.build.skeletonAlias;
				item.tags = settings.build.tags;

				if (EditorCompressionPresetTable* presetTable = ar.FindContext<EditorCompressionPresetTable>())
				{
					const EditorCompressionPreset* preset = presetTable->FindPresetForAnimation(item);
					if (preset)
					{
						presetApplied = true;
						if (ar.OpenBlock("automaticCompressionSettings", "+!Compression Preset"))
						{
							ar(preset->entry.name, "preset", "!^");
							const_cast<EditorCompressionPreset*>(preset)->entry.settings.Serialize(ar);
							ar.CloseBlock();
						}
					}
				}

				if (EditorDBATable* dbaTable = ar.FindContext<EditorDBATable>())
				{
					int dbaIndex = dbaTable->FindDBAForAnimation(item);
					if (dbaIndex >= 0)
					{
						string dbaName = dbaTable->GetEntryByIndex(dbaIndex)->entry.path;
						ar(dbaName, "dbaName", "<!DBA");
					}
				}
			}
		}
		if (!ar.IsEdit() || !presetApplied)
		{
			int oldFilter = ar.GetFilter();
			ar.SetFilter(ar.GetFilter() | SERIALIZE_COMPRESSION_SETTINGS_AS_TREE);
			ar(settings.build.compression, "compression", "Compression");
			ar.SetFilter(oldFilter);
		}
	}
	if (type == BLEND_SPACE)
	{
		if (ar.IsEdit() && ar.IsOutput())
		{
			if (ICharacterInstance* characterInstance = ar.FindContext<ICharacterInstance>())
			{
				UpdateBlendSpaceMotionParameters(characterInstance->GetIAnimationSet(), &characterInstance->GetIDefaultSkeleton());
			}
		}
		blendSpace.Serialize(ar);
	}
	if (type == COMBINED_BLEND_SPACE)
	{
		combinedBlendSpace.Serialize(ar);
	}
	if (type == ANM)
	{
		string msg = "Contains no properties.";
		ar(msg, "msg", "<!");
	}
	if (type != ANM && type != AIMPOSE && type != LOOKPOSE && importState != NEW_ANIMATION)
	{
		ar(events, "events", "Animation Events");
	}
	ar(size, "size");
}

}
