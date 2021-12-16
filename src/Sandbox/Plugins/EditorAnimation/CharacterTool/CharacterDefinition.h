#pragma once

#include <Cry_Math.h>
#include <ICryAnimation.h>
#include <IAttachment.h>
#include <vector>

namespace Serialization { class IArchive; }

struct ICharacterInstance;

namespace CharacterTool
{

using std::vector;

struct CharacterAttachment
{

	enum TransformSpace
	{
		SPACE_CHARACTER,
		SPACE_JOINT
	};

	enum ProxyPurpose
	{
		AUXILIARY,
		CLOTH,
		RAGDOLL
	};

	enum ProjectionSelection1
	{
		PS1_NoProjection,
		PS1_ShortarcRotation,
		PS1_ShortvecTranslation,
		PS1_DirectedTranslation
	};
	enum ProjectionSelection2
	{
		PS2_NoProjection,
		PS2_DirectedRotation,
		PS2_ShortvecTranslation,
		PS2_DirectedTranslation
	};

	AttachmentTypes m_attachmentType;
	string m_strSocketName;
	TransformSpace m_positionSpace;
	TransformSpace m_rotationSpace;
	bool m_alignRotation;
	bool m_alignPosition;
	QuatT m_relativeCharacterPosition;
	QuatT m_relativeJointPosition;
	string m_strJointName;
	string m_strGeometryFilepath;
	string m_strMaterial;
	Vec4   m_ProxyParams;
	ProxyPurpose m_ProxyPurpose;
	SimulationParams m_simulationParams;
	SVClothParams m_vclothParams;
	float m_viewDistanceMultiplier;
	int m_nFlags;

	CharacterAttachment()
	: m_attachmentType(CA_BONE)
	, m_positionSpace(SPACE_JOINT)
	, m_rotationSpace(SPACE_JOINT)
	, m_alignRotation(0)
	, m_alignPosition(0)
	, m_relativeCharacterPosition(IDENTITY)
	, m_relativeJointPosition(IDENTITY)
	, m_ProxyParams(0,0,0,0)
	, m_ProxyPurpose(AUXILIARY)
	, m_viewDistanceMultiplier(1.0f)
	, m_nFlags(0)
	{
	}

	void Serialize(Serialization::IArchive& ar);
	void AlignWithJoint(ICharacterInstance* character);
};

struct ICryAnimation;

struct CharacterDefinition
{
	string skeleton;
	string materialPath;
	string physics;
	string rig;
	vector<CharacterAttachment> attachments;

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
	IAnimationSerializablePtr modifiers;
#endif

	CharacterDefinition();
	bool LoadFromXml(const XmlNodeRef& root);
	bool LoadFromXmlFile(const char* filename);
	void ApplyToCharacter(bool* skinSetChanged, ICharacterInstance* character, ICharacterManager* cryAnimation);
	void SynchModifiers(ICharacterInstance& character);

	void Serialize(Serialization::IArchive& ar);
	bool Save(const char* filename);
	XmlNodeRef SaveToXml();
	bool SaveToMemory(vector<char>* buffer);
};

}
