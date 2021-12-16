#include "CharacterDefinition.h"
#include "Strings.h"
#include "Serialization.h"
#include "Expected.h"
#include <ICryAnimation.h>
#include <CryExtension/ICryFactory.h>
#include <Serialization/CryExtension.h>
#include <Serialization/CryExtensionImpl.h>
#include "Serialization/IArchiveHost.h"
#include <CryExtension/CryCreateClassInstance.h>


#include <IEditor.h>

namespace CharacterTool
{

SERIALIZATION_ENUM_BEGIN(AttachmentTypes,  "Attachment Type")
SERIALIZATION_ENUM(CA_BONE,   "bone",   "Joint")
SERIALIZATION_ENUM(CA_FACE,   "face",   "Face")
SERIALIZATION_ENUM(CA_SKIN,   "skin",   "Skin")
SERIALIZATION_ENUM(CA_PROX,   "prox",   "Proxy")
SERIALIZATION_ENUM(CA_VCLOTH, "vcloth", "VCloth")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(SimulationParams, ClampType, "Clamp Mode")
SERIALIZATION_ENUM(SimulationParams::DISABLED,               "disabled",  "Disabled")
SERIALIZATION_ENUM(SimulationParams::PENDULUM_CONE,          "cone",      "Cone")
SERIALIZATION_ENUM(SimulationParams::PENDULUM_HINGE_PLANE,   "hinge",     "Hinge")
SERIALIZATION_ENUM(SimulationParams::PENDULUM_HALF_CONE,     "halfCone",  "Half Cone")
SERIALIZATION_ENUM(SimulationParams::SPRING_ELLIPSOID,       "ellipsoid", "Ellipsoid")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(CharacterAttachment, ProxyPurpose, "Proxy Pupose")
SERIALIZATION_ENUM(CharacterAttachment::AUXILIARY,      "auxiliary", "Auxiliary")
SERIALIZATION_ENUM(CharacterAttachment::CLOTH,          "cloth",     "Cloth")
SERIALIZATION_ENUM(CharacterAttachment::RAGDOLL,        "ragdoll",   "Rag Doll")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(CharacterAttachment, ProjectionSelection1, "projectionType")
SERIALIZATION_ENUM(CharacterAttachment::PS1_NoProjection,     "no_projection",         "No Projection")
SERIALIZATION_ENUM(CharacterAttachment::PS1_ShortarcRotation, "shortarc_rotation",    "Shortarc Rotation")
SERIALIZATION_ENUM(CharacterAttachment::PS1_ShortvecTranslation, "shortvec_translation", "Shortvec Translation")
SERIALIZATION_ENUM(CharacterAttachment::PS1_DirectedTranslation, "directed_translation", "Directed Translation")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(CharacterAttachment, ProjectionSelection2, "projectionType")
SERIALIZATION_ENUM(CharacterAttachment::PS2_NoProjection, "no_projection",         "No Projection")
SERIALIZATION_ENUM(CharacterAttachment::PS2_DirectedRotation, "directed_rotation",    "Directed Rotation")
SERIALIZATION_ENUM(CharacterAttachment::PS2_ShortvecTranslation, "shortvec_translation", "Shortvec Translation")
SERIALIZATION_ENUM(CharacterAttachment::PS2_DirectedTranslation, "directed_translation", "Directed Translation")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(CharacterAttachment, TransformSpace, "Position Space")
SERIALIZATION_ENUM(CharacterAttachment::SPACE_CHARACTER,  "character", "Character (Bind Pose)")
SERIALIZATION_ENUM(CharacterAttachment::SPACE_JOINT,      "joint",     "Joint")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(AttachmentFlags, "Attachment Flags")
SERIALIZATION_ENUM(FLAGS_ATTACH_HIDE_ATTACHMENT,         "hide",                    "Hidden")
SERIALIZATION_ENUM(FLAGS_ATTACH_PHYSICALIZED_RAYS,       "physicalized_rays",       "Physicalized Rays")
SERIALIZATION_ENUM(FLAGS_ATTACH_PHYSICALIZED_COLLISIONS, "physicalized_collisions", "Physicalized Collisions")
SERIALIZATION_ENUM(FLAGS_ATTACH_SW_SKINNING,             "software_skinning",       "Software Skinning")
SERIALIZATION_ENUM(FLAGS_ATTACH_COMBINEATTACHMENT,       "combine_attachment",      "Combine Attachment")
SERIALIZATION_ENUM_END()



void CharacterAttachment::Serialize(Serialization::IArchive& ar)
{
	using Serialization::Range;
	using Serialization::LocalToJoint;
	using Serialization::LocalToCharacter;
	using Serialization::LocalToEntity;

	if (ar.IsEdit() && ar.IsOutput())
	{
		ar(m_strSocketName, "inlineName", "!^");
		ar(m_attachmentType, "inlineType", "!>50>^");
	}

	stack_string oldName = m_strSocketName;
	ar(m_strSocketName, "name", "Name");
	if (m_strSocketName.empty())
		ar.Warning(m_strSocketName, "Missing attachment name.");

	ar(m_attachmentType, "type", "Type");

	stack_string oldGeometry = m_strGeometryFilepath;
	if (m_attachmentType == CA_BONE)
	{
		ar(JointName(m_strJointName), "jointName", "Joint");

//		ar( m_alignRotation, "alignRotation", "Align Rotation");
//		ar( m_alignPosition, "alignPosition", "Align Position");

		ICharacterInstance* pICharacterInstance = ar.FindContext<ICharacterInstance>();
	/*	if (pICharacterInstance)
		{
			const IDefaultSkeleton& rIDefaultSkeleton = pICharacterInstance->GetIDefaultSkeleton();
			int id = rIDefaultSkeleton.GetJointIDByName(m_strJointName.c_str());
			if (id != -1)
			{
				const QuatT& defaultJointTransform = rIDefaultSkeleton.GetDefaultAbsJointByID(id);
				if (m_alignPosition)
					m_relativeCharacterPosition.t=defaultJointTransform.t,m_relativeJointPosition.t=Vec3(ZERO);
				if (m_alignRotation)
					m_relativeCharacterPosition.q=defaultJointTransform.q,m_relativeJointPosition.q.SetIdentity();
			}
		}*/

		/*
		ar(m_positionSpace, "positionSpace", "Position Space");
		if (m_positionSpace == SPACE_JOINT)
				ar(LocalToJoint(m_relativeJointPosition.t, m_strJointName, &m_relativeJointPosition.t), "positionJoint", "Position");
		else
			ar(m_relativeJointPosition, "positionJoint", 0);

		if (m_positionSpace != SPACE_JOINT)
				ar(LocalToCharacter(m_relativeCharacterPosition.t, m_strJointName, &m_relativeCharacterPosition.t), "positionCharacter", "Position");
		else
			ar(m_relativeCharacterPosition, "positionCharacter", 0);



		ar(m_rotationSpace, "rotationSpace", "Rotation Space");
		if (m_rotationSpace == SPACE_JOINT)
			ar(LocalToJoint(m_relativeJointPosition.q, m_strJointName, &m_relativeJointPosition.q), "rotationJoint", "Rotation");
		else
			ar(m_relativeJointPosition, "rotationJoint", 0);

		if (m_rotationSpace != SPACE_JOINT)
			ar(LocalToCharacter(m_relativeCharacterPosition.q, m_strJointName, &m_relativeCharacterPosition.q), "rotationCharacter", "Rotation");
		else
			ar(m_relativeCharacterPosition, "rotationCharacter", 0);


		if (pICharacterInstance)
		{
			const IDefaultSkeleton& rIDefaultSkeleton = pICharacterInstance->GetIDefaultSkeleton();
			int id = rIDefaultSkeleton.GetJointIDByName(m_strJointName.c_str());
			if (id != -1)
			{
				const QuatT& defaultJointTransform = rIDefaultSkeleton.GetDefaultAbsJointByID(id);
				if (m_positionSpace == SPACE_JOINT)
					m_relativeCharacterPosition.t = (defaultJointTransform*m_relativeJointPosition).t;
				else
					m_relativeJointPosition.t = (defaultJointTransform.GetInverted()*m_relativeCharacterPosition).t;

				if (m_rotationSpace == SPACE_JOINT)
					m_relativeCharacterPosition.q = (defaultJointTransform*m_relativeJointPosition).q;
				else
					m_relativeJointPosition.q = (defaultJointTransform.GetInverted()*m_relativeCharacterPosition).q;
			}
		}*/


		if (m_positionSpace == m_rotationSpace)
		{

		}


		bool inJointSpace = m_positionSpace == SPACE_JOINT && m_rotationSpace == SPACE_JOINT;
		if (inJointSpace)
			ar(LocalToJoint(m_relativeJointPosition, m_strJointName, this), "positionJoint", "Position");
		else
			ar(m_relativeJointPosition, "positionJoint", 0);

		if (!inJointSpace)
			ar(LocalToCharacter(m_relativeCharacterPosition, m_strJointName, this), "positionCharacter", "Position");
		else
			ar(m_relativeCharacterPosition, "positionCharacter", 0);

		if (pICharacterInstance)
		{
			const IDefaultSkeleton& rIDefaultSkeleton = pICharacterInstance->GetIDefaultSkeleton();
			int id = rIDefaultSkeleton.GetJointIDByName(m_strJointName.c_str());
			if (id != -1)
			{
				const QuatT& defaultJointTransform = rIDefaultSkeleton.GetDefaultAbsJointByID(id);
				if (inJointSpace)
					m_relativeCharacterPosition = defaultJointTransform * m_relativeJointPosition;
				else
					m_relativeJointPosition = defaultJointTransform.GetInverted() * m_relativeCharacterPosition;
			}
		}


		ar(ResourceFilePath(m_strGeometryFilepath, "Attachment Geometry (cgf, cga, cdf, chr, skin)|*.cgf;*.cga;*.cdf;*chr;*.skin", "Objects"), "geometry", "<Geometry");
		ar(ResourceFilePath(m_strMaterial, "Materials (mtl)|*.mtl", "Materials"), "material", "<Material");
		ar(m_viewDistanceMultiplier, "viewDistanceMultiplier", "View Distance Multiplier");

		if (ar.OpenBlock("simulation", "+Simulation")) 
		{
			SimulationParams::ClampType ct=m_simulationParams.m_nClampType;
			ar(ct, "clampType", "^");
			m_simulationParams.m_nClampType=ct;
			if (ct==SimulationParams::PENDULUM_CONE || ct==SimulationParams::PENDULUM_HINGE_PLANE || ct==SimulationParams::PENDULUM_HALF_CONE )
			{
				ar( m_simulationParams.m_useRedirect,                                          "useRedirect",      "Redirect to Joint");
				if (m_simulationParams.m_useRedirect || m_strGeometryFilepath.empty()==0)
				{
					ar( m_simulationParams.m_useDebug,                            "useDebug",         "Debug Setup");
					ar( m_simulationParams.m_useSimulation,                       "useSimulation",    "Activate Simulation");

					ar( Range(m_simulationParams.m_nSimFPS,uint8(10),uint8(255)), "simulationFPS",    "Simulation FPS");

					ar( Range(m_simulationParams.m_fMaxDeg,0.0f,179.0f),          "maxAngle",  "Maximal Angle");
					ar( Range(m_simulationParams.m_fHRotation,0.0f,359.0f),       "hRotation", "Hinge Rotation");

					ar( Range(m_simulationParams.m_fMass,0.0001f,10.0f),          "mass",      "Mass");
					ar( Range(m_simulationParams.m_fGravity,-20.0f,20.0f),        "gravity",   "Gravity");
					ar( Range(m_simulationParams.m_fDamping,0.0f,10.0f),          "damping",   "Damping");
					ar( Range(m_simulationParams.m_fStiffness,0.0f,999.0f),       "Stiffness", "Stiffness");

					ar(m_simulationParams.m_vPivotOffset,    "pivotOffset",     "Pivot Offset");
					ar(m_simulationParams.m_vSimulationAxis, "SimulationAxis",  "Simulation Axis");
					Vec2& vStiffnessTarget = (Vec2&)m_simulationParams.m_vStiffnessTarget;
					ar(vStiffnessTarget,                     "StiffnessTarget", "Spring Target");

					if (ct==SimulationParams::PENDULUM_CONE)
					{
						ProjectionSelection1 pt = ProjectionSelection1(m_simulationParams.m_nProjectionType);
						ar(pt, "projectionType", "Projection Type");
						m_simulationParams.m_nProjectionType=pt;
					}
					else
					{
						ProjectionSelection2 pt = ProjectionSelection2(m_simulationParams.m_nProjectionType);
						ar(pt, "projectionType", "Projection Type");
						m_simulationParams.m_nProjectionType=pt;
					}
					ar(m_simulationParams.m_vCapsule, "capsule",   "Capsule");
				}
			}

			if (ct==SimulationParams::SPRING_ELLIPSOID)
			{
				ar( m_simulationParams.m_useRedirect,														"useRedirect",    "Redirect to Joint");
				if (m_simulationParams.m_useRedirect || m_strGeometryFilepath.empty()==0)
				{
					ar( m_simulationParams.m_useDebug,														"useDebug",       "Debug Setup");
					ar( m_simulationParams.m_useSimulation,												"useSimulation",  "Activate Simulation");
					ar( Range(m_simulationParams.m_nSimFPS,uint8(10),uint8(255)), "simulationFPS",  "Simulation FPS");

					ar( Range(m_simulationParams.m_fMaxDeg,0.0f,179.0f),          "radius",         "Radius");
					ar( Range(m_simulationParams.m_fScaleZN,0.0f,99.0f),          "scaleNegative",  "Scale Negative");
					ar( Range(m_simulationParams.m_fScaleZP,0.0f,99.0f),          "scalePositive",  "Scale Positive");

					ar( Range(m_simulationParams.m_fHRotation,0.0f,359.0f),       "hRotation",      "Hinge Rotation");

					ar( Range(m_simulationParams.m_fMass,0.0001f,10.0f),          "mass",					  "Mass");
					ar( Range(m_simulationParams.m_fGravity,0.0001f,99.0f),       "gravity",   		  "Gravity");
					ar( Range(m_simulationParams.m_fDamping,0.0f,10.0f),          "damping",   		  "Damping");
					ar( Range(m_simulationParams.m_fStiffness,0.0f,999.0f),       "Stiffness", 		  "Stiffness");

					ar(m_simulationParams.m_vPivotOffset,     "pivotOffset",    "Pivot Offset");
					ar(m_simulationParams.m_vStiffnessTarget, "SpringTarget",   "Spring Target");
				}
			}

			ar.CloseBlock();
		}
		ar(BitFlags<AttachmentFlags>(m_nFlags), "flags", "+Flags");
		if ((m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0)
			ar.Warning(*this, "Hidden by default.");
	}


	if (m_attachmentType == CA_FACE)
	{
		ar(LocalToEntity(m_relativeCharacterPosition, this), "positionCharacter", "Position");

		ar(ResourceFilePath(m_strGeometryFilepath, "Attachment Geometry (cgf, cga, cdf, chr, skin)|*.cgf;*.cga;*.cdf;*chr;*.skin", "Objects"), "geometry", "<Geometry");
		ar(ResourceFilePath(m_strMaterial, "Materials (mtl)|*.mtl", "Materials"), "material", "<Material");
		ar(m_viewDistanceMultiplier, "viewDistanceMultiplier", "View Distance Multiplier");

		if (ar.OpenBlock("simulation", "+Simulation")) 
		{
			SimulationParams::ClampType ct=m_simulationParams.m_nClampType;
			ar(ct, "clampType", "^");
			m_simulationParams.m_nClampType=ct;
			if (ct==SimulationParams::PENDULUM_CONE || ct==SimulationParams::PENDULUM_HINGE_PLANE || ct==SimulationParams::PENDULUM_HALF_CONE )
			{
				if (m_strGeometryFilepath.empty()==0)
				{
					ar( m_simulationParams.m_useDebug,														"useDebug",       "Debug Setup");
					ar( m_simulationParams.m_useSimulation, 											"useSimulation",  "Activate Simulation");
					ar( Range(m_simulationParams.m_nSimFPS,uint8(10),uint8(255)), "simulationFPS",  "Simulation FPS");

					ar( Range(m_simulationParams.m_fMaxDeg,0.0f,179.0f),          "maxAngle",			  "Maximal Angle");
					ar( Range(m_simulationParams.m_fHRotation,0.0f,359.0f),       "hRotation", 		  "Hinge Rotation");

					ar( Range(m_simulationParams.m_fMass,0.0001f,10.0f),          "mass",					  "Mass");
					ar( Range(m_simulationParams.m_fGravity,0.0001f,99.0f),       "gravity",   		  "Gravity");
					ar( Range(m_simulationParams.m_fDamping,0.0f,10.0f),          "damping",   		  "Damping");
					ar( Range(m_simulationParams.m_fStiffness,0.0f,999.0f),       "stiffness", 		  "Stiffness"); //in the case of a pendulum its a joint-based force 

					ar(m_simulationParams.m_vPivotOffset,    "pivotOffset",     "Pivot Offset");
					ar(m_simulationParams.m_vSimulationAxis, "simulationAxis",  "Simulation Axis");
					Vec2& vStiffnessTarget = (Vec2&)m_simulationParams.m_vStiffnessTarget;
					ar(vStiffnessTarget,                     "stiffnessTarget", "Stiffness Target");
					ar(m_simulationParams.m_vCapsule,			   "capsule",         "Capsule");
				}
			}

			if (ct==SimulationParams::SPRING_ELLIPSOID)
			{
				if (m_strGeometryFilepath.empty()==0)
				{
					ar( m_simulationParams.m_useDebug,														"useDebug",				 "Debug Setup");
					ar( m_simulationParams.m_useSimulation,									      "useSimulation",   "Activate Simulation");
					ar( Range(m_simulationParams.m_nSimFPS,uint8(10),uint8(255)), "simulationFPS",   "Simulation FPS");
		
					ar( Range(m_simulationParams.m_fMaxDeg,0.0f,179.0f),          "radius",          "Radius");
					ar( Range(m_simulationParams.m_fScaleZN,0.0f,99.0f),          "scaleNegative",   "Scale Negative");
					ar( Range(m_simulationParams.m_fScaleZP,0.0f,99.0f),          "scalePositive",   "Scale Positive");
					ar( Range(m_simulationParams.m_fHRotation,0.0f,359.0f),       "hRotation",       "Hinge Rotation");

					ar( Range(m_simulationParams.m_fMass,0.0001f,10.0f),          "mass",					   "Mass");
					ar( Range(m_simulationParams.m_fGravity,0.0001f,99.0f),       "gravity",   		   "Gravity");
					ar( Range(m_simulationParams.m_fDamping,0.0f,10.0f),          "damping",   		   "Damping");
					ar( Range(m_simulationParams.m_fStiffness,0.0f,999.0f),       "Stiffness",       "Stiffness"); //in the case of a spring its a position-based force

					ar(m_simulationParams.m_vPivotOffset,                "pivotOffset",     "Pivot Offset");
					ar(m_simulationParams.m_vStiffnessTarget,            "StiffnessTarget", "Spring Target");
				}
			}
			ar.CloseBlock();
		}
	}


	if (m_attachmentType == CA_SKIN)
	{
		ar(ResourceFilePath(m_strGeometryFilepath, "Attachment Geometry (skin) | *.skin", "Objects"), "geometry", "<Geometry");
		ar(ResourceFilePath(m_strMaterial, "Materials (mtl)|*.mtl", "Materials"), "material", "<Material");
		ar(m_viewDistanceMultiplier, "viewDistanceMultiplier", "View Distance Multiplier");

		ar(BitFlags<AttachmentFlags>(m_nFlags), "flags", "+Flags");
		if ((m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0)
			ar.Warning(*this, "Hidden by default.");
	}


	if (m_attachmentType == CA_PROX)
	{
		ar(JointName(m_strJointName), "jointName", "Joint");
		ar(m_positionSpace, "positionSpace", "Position Space");
		bool inJointSpace = m_positionSpace == SPACE_JOINT;
		ar(LocalToJoint(m_relativeJointPosition, m_strJointName, this), "positionJoint", inJointSpace ? "Position" : 0);
		ar(LocalToCharacter(m_relativeCharacterPosition, m_strJointName, this), "positionCharacter", !inJointSpace ? "Position" : 0);

		ICharacterInstance* pICharacterInstance = ar.FindContext<ICharacterInstance>();
		if (pICharacterInstance)
		{
			const IDefaultSkeleton& rIDefaultSkeleton = pICharacterInstance->GetIDefaultSkeleton();
			int id = rIDefaultSkeleton.GetJointIDByName(m_strJointName.c_str());
			if (id != -1)
			{
				const QuatT& defaultJointTransform = rIDefaultSkeleton.GetDefaultAbsJointByID(id);
				if (inJointSpace)
					m_relativeCharacterPosition = defaultJointTransform * m_relativeJointPosition;
				else
					m_relativeJointPosition = defaultJointTransform.GetInverted() * m_relativeCharacterPosition;
			}
		}

		ar(m_ProxyPurpose, "proxyPurpose", "Purpose");

		ar( Range(m_ProxyParams.w,0.0f,10.0f), "radius", "Radius");
		ar( Range(m_ProxyParams.x,0.0f,10.0f), "x-axis", "X-axis");
		ar( Range(m_ProxyParams.y,0.0f,10.0f), "y-axis", "Y-axis");
		ar( Range(m_ProxyParams.z,0.0f,10.0f), "z-axis", "Z-axis");
	}

	if (ar.IsInput())
	{
		// assign attachment name based on geometry file path when geometry is changing, by the name is empty
		if (m_strSocketName.empty() && oldName.empty() && oldGeometry.empty() && !m_strGeometryFilepath.empty())
			m_strSocketName = PathUtil::GetFileName(m_strGeometryFilepath.c_str());
	}
}


void CharacterAttachment::AlignWithJoint(ICharacterInstance* character)
{
	if (!character)
		return;
	IAttachmentManager* attachmentManager = character->GetIAttachmentManager();
	if (!attachmentManager)
		return;
	IDefaultSkeleton& rIDefaultSkeleton = character->GetIDefaultSkeleton();

	uint32 numJoints = rIDefaultSkeleton.GetJointCount();
	int boneId = rIDefaultSkeleton.GetJointIDByName(m_strJointName.c_str());
	if (boneId < numJoints)
	{
		m_relativeCharacterPosition = character->GetISkeletonPose()->GetAbsJointByID(boneId);
		m_relativeJointPosition = IDENTITY;
	}
}

// ---------------------------------------------------------------------------

CharacterDefinition::CharacterDefinition()
{
}

void CharacterDefinition::Serialize(Serialization::IArchive& ar)
{
	ar(SkeletonPath(skeleton), "skeleton", "Skeleton");
	if (skeleton.empty())
		ar.Warning(skeleton, "Skeleton is required every character.");
	ar(MaterialPath(materialPath), "material", "Material");
	// evgenya: .rig and .phys references are temporarily disabled
	ar(CharacterPhysicsPath(physics), "physics", 0);
	ar(CharacterRigPath(rig), "rig", 0);
	ar(attachments, "attachments", "Attachments");
	if (attachments.empty())
		ar.Warning(attachments, "Add attachment to create character geometry. Skeleton display can be enabled in \"Display Options\"");
#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
	if (modifiers)
		modifiers->Serialize(ar);
#endif
}

bool CharacterDefinition::LoadFromXmlFile(const char* filename)
{
	XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(filename);
	if (root==0)
		return false;

	return LoadFromXml(root);
}

bool CharacterDefinition::LoadFromXml(const XmlNodeRef& root)
{
	uint32 numChilds = root->getChildCount();
	if (numChilds==0)
		return false;

	for (uint32 xmlnode=0; xmlnode<numChilds; xmlnode++)
	{
		XmlNodeRef node = root->getChild(xmlnode);
		const char* tag = node->getTag();

		//-----------------------------------------------------------
		//load base model 
		//-----------------------------------------------------------
		if (strcmp(tag,"Model")==0)
		{
			skeleton = node->getAttr( "File" ); 
			materialPath = node->getAttr("Material");
			physics = node->getAttr("Physics");
			rig = node->getAttr("Rig File");
		}

		//-----------------------------------------------------------
		//load attachment-list 
		//-----------------------------------------------------------
		if (strcmp(tag,"AttachmentList")==0)
		{			
			XmlNodeRef nodeAttachements = node;
			uint32 num = nodeAttachements->getChildCount();
			attachments.clear();
			attachments.reserve(num);

			for (uint32 i=0; i<num; i++) 
			{
				CharacterAttachment attach;
				XmlNodeRef nodeAttach = nodeAttachements->getChild(i);
				const char* AttachTag = nodeAttach->getTag();
				if (strcmp(AttachTag,"Attachment"))
					continue; //invalid

				stack_string Type = nodeAttach->getAttr( "Type" );
				attach.m_attachmentType = CA_Invalid;
				if (Type=="CA_BONE") 
					attach.m_attachmentType = CA_BONE;
				if (Type=="CA_FACE") 
					attach.m_attachmentType = CA_FACE;
				if (Type=="CA_SKIN") 
					attach.m_attachmentType = CA_SKIN;
				if (Type=="CA_PROX") 
					attach.m_attachmentType = CA_PROX;
				if (Type=="CA_VCLOTH") 
					attach.m_attachmentType = CA_VCLOTH;

				attach.m_strSocketName = nodeAttach->getAttr( "AName" );

				attach.m_positionSpace = attach.SPACE_CHARACTER;
				attach.m_rotationSpace = attach.SPACE_CHARACTER;

				nodeAttach->getAttr( "AlignRot", attach.m_alignRotation );
				nodeAttach->getAttr( "AlignPos", attach.m_alignPosition );

				QuatT& transform =  attach.m_relativeCharacterPosition;
				nodeAttach->getAttr( "Rotation", transform.q );
				nodeAttach->getAttr( "Position", transform.t );

				attach.m_strJointName = nodeAttach->getAttr( "BoneName" );
				attach.m_strGeometryFilepath = nodeAttach->getAttr( "Binding" );
				attach.m_strMaterial = nodeAttach->getAttr("Material");

				nodeAttach->getAttr( "ProxyParams",attach.m_ProxyParams );
				uint32 nProxyPurpose=0;
				nodeAttach->getAttr( "ProxyPurpose",nProxyPurpose ),attach.m_ProxyPurpose=CharacterAttachment::ProxyPurpose(nProxyPurpose);


				SimulationParams::ClampType ct=SimulationParams::DISABLED;
				nodeAttach->getAttr( "PA_PendulumType",(int&)ct );

				if (ct==SimulationParams::PENDULUM_CONE || ct==SimulationParams::PENDULUM_HINGE_PLANE || ct==SimulationParams::PENDULUM_HALF_CONE )
				{
					attach.m_simulationParams.m_nClampType=ct;
					nodeAttach->getAttr( "PA_FPS",             attach.m_simulationParams.m_nSimFPS );
					nodeAttach->getAttr( "PA_MaxAngle",        attach.m_simulationParams.m_fMaxDeg );
					nodeAttach->getAttr( "PA_HRotation",       attach.m_simulationParams.m_fHRotation );
					nodeAttach->getAttr( "PA_Redirect",        attach.m_simulationParams.m_useRedirect );

					nodeAttach->getAttr( "PA_Mass",            attach.m_simulationParams.m_fMass );
					nodeAttach->getAttr( "PA_Gravity",         attach.m_simulationParams.m_fGravity );
					nodeAttach->getAttr( "PA_Damping",         attach.m_simulationParams.m_fDamping );
					nodeAttach->getAttr( "PA_Stiffness",       attach.m_simulationParams.m_fStiffness );

					nodeAttach->getAttr( "PA_PivotOffset",     attach.m_simulationParams.m_vPivotOffset );
					nodeAttach->getAttr( "PA_SimulationAxis",  attach.m_simulationParams.m_vSimulationAxis );
					nodeAttach->getAttr( "PA_StiffnessTarget", attach.m_simulationParams.m_vStiffnessTarget );

					nodeAttach->getAttr( "PA_ProjectionType",  attach.m_simulationParams.m_nProjectionType );
					nodeAttach->getAttr( "PA_CapsuleX",        attach.m_simulationParams.m_vCapsule.x );
					nodeAttach->getAttr( "PA_CapsuleY",        attach.m_simulationParams.m_vCapsule.y );
					char proxytag[] = "PA_Proxy00";
					for (uint32 i=0; i<10; i++)
					{
						string strProxyName = nodeAttach->getAttr( proxytag );
						proxytag[9]++;
						if (strProxyName.empty())
							continue;
						uint32 nCRC32 = GetIEditor()->GetSystem()->GetCrc32Gen()->GetCRC32Lowercase(strProxyName.c_str());
						attach.m_simulationParams.m_arrProxyCRC32.push_back(nCRC32);
						attach.m_simulationParams.m_arrProxyName.push_back(strProxyName.c_str());
					}
				}

				ct=SimulationParams::DISABLED;
				nodeAttach->getAttr( "SA_SpringType", (int&)ct); 
				if (ct)
				{
					attach.m_simulationParams.m_nClampType=SimulationParams::SPRING_ELLIPSOID;
					nodeAttach->getAttr( "SA_FPS",             attach.m_simulationParams.m_nSimFPS); 
					nodeAttach->getAttr( "SA_Radius",          attach.m_simulationParams.m_fMaxDeg);
					nodeAttach->getAttr( "SA_HRotation",       attach.m_simulationParams.m_fHRotation);  
					nodeAttach->getAttr( "SA_ScaleZP",         attach.m_simulationParams.m_fScaleZP); 
					nodeAttach->getAttr( "SA_ScaleZN",         attach.m_simulationParams.m_fScaleZN); 
				  nodeAttach->getAttr( "SA_Redirect",        attach.m_simulationParams.m_useRedirect );

					nodeAttach->getAttr( "SA_Mass",            attach.m_simulationParams.m_fMass );
					nodeAttach->getAttr( "SA_Gravity",         attach.m_simulationParams.m_fGravity );
					nodeAttach->getAttr( "SA_Damping",         attach.m_simulationParams.m_fDamping );
					nodeAttach->getAttr( "SA_Stiffness",       attach.m_simulationParams.m_fStiffness );

					nodeAttach->getAttr( "SA_PivotOffset",     attach.m_simulationParams.m_vPivotOffset );
					nodeAttach->getAttr( "SA_StiffnessTarget", attach.m_simulationParams.m_vStiffnessTarget );
				}


				if (Type=="CA_VCLOTH")
				{
					nodeAttach->getAttr( "hide",                  attach.m_vclothParams.hide);
					//nodeAttach->getAttr( "debug",               attach.m_vclothParams.debug );
					nodeAttach->getAttr( "thickness",             attach.m_vclothParams.thickness );
					nodeAttach->getAttr( "collsionDamping",       attach.m_vclothParams.collisionDamping );
					//nodeAttach->getAttr( "SA_Mass",             attach.m_vclothParams.dragDamping );
					nodeAttach->getAttr( "stretchStiffness",      attach.m_vclothParams.stretchStiffness );
					nodeAttach->getAttr( "shearStiffness",        attach.m_vclothParams.shearStiffness );
					nodeAttach->getAttr( "bendStiffness",         attach.m_vclothParams.bendStiffness );
					nodeAttach->getAttr( "numIterations",         attach.m_vclothParams.numIterations );
					nodeAttach->getAttr( "timeStep",              attach.m_vclothParams.timeStep );
					nodeAttach->getAttr( "rigidDamping",          attach.m_vclothParams.rigidDamping );
					nodeAttach->getAttr( "translationBlend",      attach.m_vclothParams.translationBlend );
					nodeAttach->getAttr( "rotationBlend",         attach.m_vclothParams.rotationBlend );
					nodeAttach->getAttr( "friction",              attach.m_vclothParams.friction );
					nodeAttach->getAttr( "pullStiffness",         attach.m_vclothParams.pullStiffness );
					nodeAttach->getAttr( "SA_Mass",               attach.m_vclothParams.tolerance );
					nodeAttach->getAttr( "maxBlendWeight",        attach.m_vclothParams.maxBlendWeight );
					nodeAttach->getAttr( "maxAnimDistance",       attach.m_vclothParams.maxAnimDistance );
					//nodeAttach->getAttr( "SA_Mass",             attach.m_vclothParams.windBlend );
					//nodeAttach->getAttr( "SA_Mass",             attach.m_vclothParams.collDampingRange );
					nodeAttach->getAttr( "stiffnessGradient",     attach.m_vclothParams.stiffnessGradient );
					nodeAttach->getAttr( "halfStretchIterations", attach.m_vclothParams.halfStretchIterations );
					nodeAttach->getAttr( "isMainCharacter",       attach.m_vclothParams.isMainCharacter );

					attach.m_vclothParams.simMeshName                = nodeAttach->getAttr( "simMeshName");
					attach.m_vclothParams.renderMeshName             = nodeAttach->getAttr( "renderMeshName");
					attach.m_vclothParams.simBinding                 = nodeAttach->getAttr( "SimBinding");
					attach.m_vclothParams.renderBinding              = nodeAttach->getAttr( "Binding");
				} 

				uint32 flags;
				if (nodeAttach->getAttr("Flags",flags))
					attach.m_nFlags = flags;
				nodeAttach->getAttr( "ViewDistRatio", attach.m_viewDistanceMultiplier );

				attachments.push_back(attach);
			}
		}
	}

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
	if (CryCreateClassInstance("PoseModifierSetup", modifiers))
	{
		if(!Serialization::LoadXmlNode(*modifiers, root))
			return false;
	}
#endif

	return true;
}

XmlNodeRef CharacterDefinition::SaveToXml()
{
	XmlNodeRef root = GetIEditor()->GetSystem()->CreateXmlNode("CharacterDefinition");

	{
		XmlNodeRef node = root->newChild("Model");
		//-----------------------------------------------------------
		// load base model 
		//-----------------------------------------------------------
		node->setAttr( "File", skeleton ); 

		if (!materialPath.empty())
			node->setAttr("Material", materialPath);
		else
			node->delAttr("Material");

		if (!physics.empty())
			node->setAttr("Physics", physics.c_str());
		else
			node->delAttr("Physics");

		if (!rig.empty())
			node->setAttr("Rig", rig.c_str());
		else
			node->delAttr("Rig");

		// We don't currently modify these in the editor...

		// node->getAttr( "KeepModelsInMemory",def.m_nKeepModelsInMemory );
	}

	//-----------------------------------------------------------
	// load attachment-list 
	//-----------------------------------------------------------
	XmlNodeRef nodeAttachments = root->newChild("AttachmentList");
	const uint32 numAttachments = nodeAttachments->getChildCount();

	// clear out the existing attachments
	while ( XmlNodeRef childNode = nodeAttachments->findChild( "Attachment" ) )
	{
		nodeAttachments->removeChild( childNode );
	}

	// populate the new attachment list
	for (vector<CharacterAttachment>::const_iterator iter = attachments.begin(), itEnd = attachments.end(); iter!=itEnd; ++iter) 
	{
		const CharacterAttachment &attach = (*iter);

		XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode( "Attachment" );
		nodeAttachments->addChild( nodeAttach );
		//nodeAttach->setAttr( "value", value );

		//-----------------------------------------------------------------------------------
		//----  export the shared parameter for all attachment-types   ----------------------
		//-----------------------------------------------------------------------------------

		if (attach.m_attachmentType==CA_BONE)
			nodeAttach->setAttr( "Type", "CA_BONE");
		if (attach.m_attachmentType==CA_FACE)
			nodeAttach->setAttr( "Type", "CA_FACE");
		if (attach.m_attachmentType==CA_SKIN)
			nodeAttach->setAttr( "Type", "CA_SKIN");
		if (attach.m_attachmentType==CA_PROX)
			nodeAttach->setAttr( "Type", "CA_PROX");
		if (attach.m_attachmentType==CA_VCLOTH)
			nodeAttach->setAttr( "Type", "CA_VCLOTH");

		if(!attach.m_strSocketName.empty()) 
			nodeAttach->setAttr( "AName", attach.m_strSocketName);

		if(!attach.m_strGeometryFilepath.empty()) 
			nodeAttach->setAttr( "Binding", attach.m_strGeometryFilepath);
		else
			nodeAttach->delAttr( "Binding" );

		if (!attach.m_strMaterial.empty())
			nodeAttach->setAttr( "Material", attach.m_strMaterial);


		//-----------------------------------------------------------------------------------
		//----  export the specialized parameters                      ----------------------
		//-----------------------------------------------------------------------------------

		if (attach.m_attachmentType==CA_BONE || attach.m_attachmentType==CA_FACE)
		{
			if (attach.m_attachmentType==CA_FACE)
			{
				const QuatT& transform = attach.m_relativeCharacterPosition;
				nodeAttach->setAttr( "Rotation", transform.q );
				nodeAttach->setAttr( "Position", transform.t );
			}

			if (attach.m_attachmentType==CA_BONE)
			{
				const QuatT& transform = attach.m_relativeCharacterPosition;
				if (attach.m_alignRotation)
					nodeAttach->setAttr( "AlignRot", attach.m_alignRotation );
				else
					nodeAttach->setAttr( "Rotation", transform.q );

				if (attach.m_alignPosition)
					nodeAttach->setAttr( "AlignPos", attach.m_alignPosition );
				else
					nodeAttach->setAttr( "Position", transform.t );

				if(!attach.m_strJointName.empty())
					nodeAttach->setAttr( "BoneName", attach.m_strJointName );
				else
					nodeAttach->delAttr( "BoneName" );
			}

			if( SimulationParams::DISABLED!=attach.m_simulationParams.m_nClampType )
			{
				uint32 ct=attach.m_simulationParams.m_nClampType;
				if (ct==SimulationParams::PENDULUM_CONE || ct==SimulationParams::PENDULUM_HINGE_PLANE || ct==SimulationParams::PENDULUM_HALF_CONE )
				{
					nodeAttach->setAttr( "PA_PendulumType",(int&)attach.m_simulationParams.m_nClampType );

					//only store values in XML if they are not identical with the default-values
					if (attach.m_simulationParams.m_nSimFPS  !=10)         nodeAttach->setAttr( "PA_FPS",attach.m_simulationParams.m_nSimFPS );
					if (attach.m_simulationParams.m_fMaxDeg  !=45.0f)      nodeAttach->setAttr( "PA_MaxAngle",attach.m_simulationParams.m_fMaxDeg );
					if (attach.m_simulationParams.m_fHRotation)            nodeAttach->setAttr( "PA_HRotation",attach.m_simulationParams.m_fHRotation  );
					if (attach.m_simulationParams.m_useRedirect)           nodeAttach->setAttr( "PA_Redirect",attach.m_simulationParams.m_useRedirect );

					if (attach.m_simulationParams.m_fMass    !=1.00f)      nodeAttach->setAttr( "PA_Mass",attach.m_simulationParams.m_fMass );
					if (attach.m_simulationParams.m_fGravity !=9.81f)      nodeAttach->setAttr( "PA_Gravity",attach.m_simulationParams.m_fGravity );
					if (attach.m_simulationParams.m_fDamping !=1.00f)      nodeAttach->setAttr( "PA_Damping",attach.m_simulationParams.m_fDamping );
					if (attach.m_simulationParams.m_fStiffness)            nodeAttach->setAttr( "PA_Stiffness",attach.m_simulationParams.m_fStiffness );

					if (sqr(attach.m_simulationParams.m_vPivotOffset))     nodeAttach->setAttr( "PA_PivotOffset",attach.m_simulationParams.m_vPivotOffset );
					if (sqr(attach.m_simulationParams.m_vSimulationAxis))  nodeAttach->setAttr( "PA_SimulationAxis",attach.m_simulationParams.m_vSimulationAxis );
					if (sqr(attach.m_simulationParams.m_vStiffnessTarget)) nodeAttach->setAttr( "PA_StiffnessTarget",attach.m_simulationParams.m_vStiffnessTarget );

					if (attach.m_simulationParams.m_nProjectionType)       nodeAttach->setAttr( "PA_ProjectionType",attach.m_simulationParams.m_nProjectionType );
					if (attach.m_simulationParams.m_vCapsule.x)            nodeAttach->setAttr( "PA_CapsuleX",attach.m_simulationParams.m_vCapsule.x );
					if (attach.m_simulationParams.m_vCapsule.y)            nodeAttach->setAttr( "PA_CapsuleY",attach.m_simulationParams.m_vCapsule.y );

					uint32 numProxiesCRC32 = min(9,attach.m_simulationParams.m_arrProxyCRC32.size());
					char proxytag[] = "PA_Proxy00";
					for (uint32 i=0; i<numProxiesCRC32; i++)
					{
						nodeAttach->setAttr( proxytag, attach.m_simulationParams.m_arrProxyName[i] );
						proxytag[9]++;
					}
				}

				if (ct==SimulationParams::SPRING_ELLIPSOID)
				{
					nodeAttach->setAttr( "SA_SpringType", (int&)attach.m_simulationParams.m_nClampType ); 

					//only store values in XML if they are not identical with the default-values
					if (attach.m_simulationParams.m_nSimFPS  !=10)         nodeAttach->setAttr( "SA_FPS",            attach.m_simulationParams.m_nSimFPS); 
					if (attach.m_simulationParams.m_fMaxDeg  !=45.0f)      nodeAttach->setAttr( "SA_Radius",         attach.m_simulationParams.m_fMaxDeg);
					if (attach.m_simulationParams.m_fHRotation)            nodeAttach->setAttr( "SA_HRotation",      attach.m_simulationParams.m_fHRotation);  
					if (attach.m_simulationParams.m_fScaleZP !=1)          nodeAttach->setAttr( "SA_ScaleZP",        attach.m_simulationParams.m_fScaleZP); 
					if (attach.m_simulationParams.m_fScaleZP !=1)          nodeAttach->setAttr( "SA_ScaleZN",        attach.m_simulationParams.m_fScaleZN); 

					if (attach.m_simulationParams.m_useRedirect)           nodeAttach->setAttr( "SA_Redirect",       attach.m_simulationParams.m_useRedirect );

					if (attach.m_simulationParams.m_fMass    !=1.00f)      nodeAttach->setAttr( "SA_Mass",           attach.m_simulationParams.m_fMass );
					if (attach.m_simulationParams.m_fGravity !=9.81f)      nodeAttach->setAttr( "SA_Gravity",        attach.m_simulationParams.m_fGravity );
					if (attach.m_simulationParams.m_fDamping !=1.00f)      nodeAttach->setAttr( "SA_Damping",        attach.m_simulationParams.m_fDamping );
					if (attach.m_simulationParams.m_fStiffness)            nodeAttach->setAttr( "SA_Stiffness",      attach.m_simulationParams.m_fStiffness );

					if (sqr(attach.m_simulationParams.m_vPivotOffset))     nodeAttach->setAttr( "SA_PivotOffset",    attach.m_simulationParams.m_vPivotOffset );
					if (sqr(attach.m_simulationParams.m_vStiffnessTarget)) nodeAttach->setAttr( "SA_StiffnessTarget",attach.m_simulationParams.m_vStiffnessTarget );
				}
			}
		}

		//--------------------------------------------------------------------------------------------------------

		if (attach.m_attachmentType==CA_VCLOTH)
		{
			nodeAttach->setAttr( "hide", attach.m_vclothParams.hide);
			//nodeAttach->setAttr( "debug", attach.m_vclothParams.debug );
			nodeAttach->setAttr( "thickness", attach.m_vclothParams.thickness );
			nodeAttach->setAttr( "collsionDamping", attach.m_vclothParams.collisionDamping );
			//nodeAttach->getAttr( "SA_Mass", attach.m_vclothParams.dragDamping );
			nodeAttach->setAttr( "stretchStiffness", attach.m_vclothParams.stretchStiffness );
			nodeAttach->setAttr( "shearStiffness", attach.m_vclothParams.shearStiffness );
			nodeAttach->setAttr( "bendStiffness", attach.m_vclothParams.bendStiffness );
			nodeAttach->setAttr( "numIterations", attach.m_vclothParams.numIterations );
			nodeAttach->setAttr( "timeStep", attach.m_vclothParams.timeStep );
			nodeAttach->setAttr( "rigidDamping", attach.m_vclothParams.rigidDamping );
			nodeAttach->setAttr( "translationBlend", attach.m_vclothParams.translationBlend );
			nodeAttach->setAttr( "rotationBlend", attach.m_vclothParams.rotationBlend );
			nodeAttach->setAttr( "friction", attach.m_vclothParams.friction );
			nodeAttach->setAttr( "pullStiffness", attach.m_vclothParams.pullStiffness );
			nodeAttach->setAttr( "SA_Mass", attach.m_vclothParams.tolerance );
			nodeAttach->setAttr( "maxBlendWeight", attach.m_vclothParams.maxBlendWeight );
			nodeAttach->setAttr( "maxAnimDistance", attach.m_vclothParams.maxAnimDistance );
			//nodeAttach->setAttr( "SA_Mass", attach.m_vclothParams.windBlend );
			//nodeAttach->setAttr( "SA_Mass", attach.m_vclothParams.collDampingRange );
			nodeAttach->setAttr( "stiffnessGradient", attach.m_vclothParams.stiffnessGradient );
			nodeAttach->setAttr( "halfStretchIterations", attach.m_vclothParams.halfStretchIterations );
			nodeAttach->setAttr( "isMainCharacter", attach.m_vclothParams.isMainCharacter );

			nodeAttach->setAttr( "renderMeshName", attach.m_vclothParams.renderMeshName);
			nodeAttach->setAttr( "Binding", attach.m_vclothParams.renderBinding);

			nodeAttach->setAttr( "simMeshName", attach.m_vclothParams.simMeshName);
			nodeAttach->setAttr( "simBinding", attach.m_vclothParams.simBinding);
		}

		//--------------------------------------------------------------------------------------------------------

		if (attach.m_attachmentType==CA_PROX)
		{
			const QuatT& transform = attach.m_relativeCharacterPosition;
			nodeAttach->setAttr( "Rotation", transform.q );
			nodeAttach->setAttr( "Position", transform.t );

			if(!attach.m_strJointName.empty())
				nodeAttach->setAttr( "BoneName", attach.m_strJointName );
			else
				nodeAttach->delAttr( "BoneName" );

			nodeAttach->setAttr( "ProxyParams",attach.m_ProxyParams );
			nodeAttach->setAttr( "ProxyPurpose",attach.m_ProxyPurpose );
		}

		nodeAttach->setAttr("Flags", attach.m_nFlags);
		if (attach.m_viewDistanceMultiplier != 1.0f)
			nodeAttach->setAttr( "ViewDistRatio",attach.m_viewDistanceMultiplier );
		else
			nodeAttach->delAttr( "ViewDistRatio"  );
	}



	if (XmlNodeRef node = root->findChild("Modifiers"))
	{
		root->removeChild( node );
	}

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
	if (modifiers)
	{
		Serialization::SaveXmlNode(root, *modifiers);
	}
#endif
	// make sure we don't leave empty modifiers block
	XmlNodeRef modifiersNode = root->findChild("Modifiers");
	if (modifiersNode && modifiersNode->getChildCount() == 0 && modifiersNode->getNumAttributes() == 0)
		root->removeChild(modifiersNode);

	return root;
}

bool CharacterDefinition::Save(const char* filename)
{
	XmlNodeRef root = SaveToXml();
	if (!root)
		return false;
	return root->saveToFile(filename);
}

bool CharacterDefinition::SaveToMemory(vector<char>* buffer)
{
	XmlNodeRef root = SaveToXml();
	if (!root)
		return false;
	_smart_ptr<IXmlStringData> str(root->getXMLData());
	buffer->assign(str->GetString(), str->GetString() + str->GetStringLength());
	return true;
}

static string NormalizeMaterialName(const char* materialName)
{
	string result = materialName;
	result.MakeLower();
	PathUtil::RemoveExtension(result);
	return result;
}



static void ApplyBoneAttachment(IAttachment* pIAttachment, ICharacterInstance* pICharacterInstance, ICharacterManager* characterManager, const CharacterAttachment& desc)
{
	if (pIAttachment==0)
		return;

	uint32 type = pIAttachment->GetType();
	if (type == CA_BONE)
	{ 
		IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
		string existingBindingFilename;
		if (pIAttachmentObject)
		{
			if (IStatObj* statObj = pIAttachmentObject->GetIStatObj())
			{
				existingBindingFilename = statObj->GetFilePath();
			}
			else if (ICharacterInstance* attachedCharacter = pIAttachmentObject->GetICharacterInstance())
			{
				existingBindingFilename = attachedCharacter->GetFilePath();
			}
			else if (IAttachmentSkin* attachmentSkin = pIAttachmentObject->GetIAttachmentSkin())
			{
				if (ISkin* skin = attachmentSkin->GetISkin())
					existingBindingFilename = skin->GetModelFilePath();
			}
		}

		if (stricmp(existingBindingFilename.c_str(), desc.m_strGeometryFilepath.c_str()) != 0)
		{
			if (!desc.m_strGeometryFilepath.empty())
			{
				string fileExt = PathUtil::GetExt( desc.m_strGeometryFilepath.c_str() );

				bool IsCDF = (0 == stricmp(fileExt,"cdf"));
				bool IsCHR = (0 == stricmp(fileExt,"chr"));
				bool IsCGA = (0 == stricmp(fileExt,"cga"));
				bool IsCGF = (0 == stricmp(fileExt,"cgf"));
				if (IsCDF || IsCHR || IsCGA) 
				{
					ICharacterInstance* pIChildCharacter = characterManager->CreateInstance( desc.m_strGeometryFilepath.c_str(),CA_CharEditModel );
					if (pIChildCharacter) 
					{
						CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
						pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
						pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
					}
				}
				if (IsCGF) 
				{
					IStatObj* pIStatObj = gEnv->p3DEngine->LoadStatObj( desc.m_strGeometryFilepath.c_str(),0,0,false );
					if (pIStatObj) 
					{
						CCGFAttachment* pStatAttachment = new CCGFAttachment();
						pStatAttachment->pObj  = pIStatObj;
						pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
					}
				}
				if (pIAttachmentObject != pIAttachment->GetIAttachmentObject())
						pIAttachment->AddBinding(pIAttachmentObject);
			}
			else
			{
				pIAttachment->ClearBinding();
			}
		}

		SimulationParams ap = pIAttachment->GetSimulationParams();
		ap.m_nClampType       = desc.m_simulationParams.m_nClampType;
		if (ap.m_nClampType==SimulationParams::PENDULUM_CONE || ap.m_nClampType==SimulationParams::PENDULUM_HINGE_PLANE || ap.m_nClampType==SimulationParams::PENDULUM_HALF_CONE )
		{
			ap.m_useDebug         = desc.m_simulationParams.m_useDebug;
			ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
			ap.m_useRedirect      = desc.m_simulationParams.m_useRedirect;

			ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;
			ap.m_fMaxDeg          = desc.m_simulationParams.m_fMaxDeg;
			ap.m_fHRotation       = desc.m_simulationParams.m_fHRotation;

			ap.m_fMass            = desc.m_simulationParams.m_fMass;
			ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
			ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
			ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

			ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
			ap.m_vSimulationAxis  = desc.m_simulationParams.m_vSimulationAxis;
			ap.m_vStiffnessTarget	= desc.m_simulationParams.m_vStiffnessTarget;
			ap.m_vCapsule	        = desc.m_simulationParams.m_vCapsule;
		}
		if (ap.m_nClampType==SimulationParams::SPRING_ELLIPSOID)
		{
			ap.m_useDebug         = desc.m_simulationParams.m_useDebug;
			ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
			ap.m_useRedirect      = desc.m_simulationParams.m_useRedirect;

			ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;
			ap.m_fMaxDeg          = desc.m_simulationParams.m_fMaxDeg;
			ap.m_fScaleZN         = desc.m_simulationParams.m_fScaleZN;
			ap.m_fScaleZP         = desc.m_simulationParams.m_fScaleZP;
			ap.m_fHRotation       = desc.m_simulationParams.m_fHRotation;

			ap.m_fMass            = desc.m_simulationParams.m_fMass;
			ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
			ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
			ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

			ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
			ap.m_vStiffnessTarget	= desc.m_simulationParams.m_vStiffnessTarget;
		}
		pIAttachment->SetSimulationParams(ap);



	}


	QuatT engineLocation = pIAttachment->GetAttAbsoluteDefault();
	QuatT editorLocation = desc.m_relativeCharacterPosition;
	if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q)
		pIAttachment->SetAttAbsoluteDefault(editorLocation);	
}




static void ApplyFaceAttachment(IAttachment* pIAttachment, ICharacterInstance* character, ICharacterManager* characterManager, const CharacterAttachment& desc)
{
	if (pIAttachment==0)
		return;

	uint32 type = pIAttachment->GetType();
	if (type == CA_FACE)
	{ 
		IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
		string existingBindingFilename;
		if (pIAttachmentObject)
		{
			if (IStatObj* statObj = pIAttachmentObject->GetIStatObj())
			{
				existingBindingFilename = statObj->GetFilePath();
			}
			else if (ICharacterInstance* attachedCharacter = pIAttachmentObject->GetICharacterInstance())
			{
				existingBindingFilename = attachedCharacter->GetFilePath();
			}
			else if (IAttachmentSkin* attachmentSkin = pIAttachmentObject->GetIAttachmentSkin())
			{
				if (ISkin* skin = attachmentSkin->GetISkin())
					existingBindingFilename = skin->GetModelFilePath();
			}
		}

		if (stricmp(existingBindingFilename.c_str(), desc.m_strGeometryFilepath.c_str()) != 0)
		{
			if (!desc.m_strGeometryFilepath.empty())
			{
				string fileExt = PathUtil::GetExt( desc.m_strGeometryFilepath.c_str() );

				bool IsCDF = (0 == stricmp(fileExt,"cdf"));
				bool IsCHR = (0 == stricmp(fileExt,"chr"));
				bool IsCGA = (0 == stricmp(fileExt,"cga"));
				bool IsCGF = (0 == stricmp(fileExt,"cgf"));
				if (IsCDF || IsCHR || IsCGA) 
				{
					ICharacterInstance* pIChildCharacter = characterManager->CreateInstance( desc.m_strGeometryFilepath.c_str(),CA_CharEditModel );
					if (pIChildCharacter) 
					{
						CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
						pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
						pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
					}
				}
				if (IsCGF) 
				{
					IStatObj* pIStatObj = gEnv->p3DEngine->LoadStatObj( desc.m_strGeometryFilepath.c_str(),0,0,false );
					if (pIStatObj) 
					{
						CCGFAttachment* pStatAttachment = new CCGFAttachment();
						pStatAttachment->pObj  = pIStatObj;
						pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
					}
				}
				if (pIAttachmentObject != pIAttachment->GetIAttachmentObject())
						pIAttachment->AddBinding(pIAttachmentObject);
			}
			else
			{
				pIAttachment->ClearBinding();
			}
		}

		SimulationParams ap = pIAttachment->GetSimulationParams();
		ap.m_nClampType       = desc.m_simulationParams.m_nClampType;
		if (ap.m_nClampType==SimulationParams::PENDULUM_CONE || ap.m_nClampType==SimulationParams::PENDULUM_HINGE_PLANE || ap.m_nClampType==SimulationParams::PENDULUM_HALF_CONE )
		{
			ap.m_useDebug         = desc.m_simulationParams.m_useDebug;
			ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
			ap.m_useRedirect      = 0;

			ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;
			ap.m_fMaxDeg          = desc.m_simulationParams.m_fMaxDeg;
			ap.m_fHRotation       = desc.m_simulationParams.m_fHRotation;

			ap.m_fMass            = desc.m_simulationParams.m_fMass;
			ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
			ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
			ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

			ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
			ap.m_vSimulationAxis  = desc.m_simulationParams.m_vSimulationAxis;
			ap.m_vStiffnessTarget	= desc.m_simulationParams.m_vStiffnessTarget;
			ap.m_vCapsule	        = desc.m_simulationParams.m_vCapsule;
		}
		if (ap.m_nClampType==SimulationParams::SPRING_ELLIPSOID)
		{
			ap.m_useDebug         = desc.m_simulationParams.m_useDebug;
			ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
			ap.m_useRedirect      = 0;

			ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;
			ap.m_fMaxDeg          = desc.m_simulationParams.m_fMaxDeg;
			ap.m_fScaleZN         = desc.m_simulationParams.m_fScaleZN;
			ap.m_fScaleZP         = desc.m_simulationParams.m_fScaleZP;
			ap.m_fHRotation       = desc.m_simulationParams.m_fHRotation;

			ap.m_fMass            = desc.m_simulationParams.m_fMass;
			ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
			ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
			ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

			ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
			ap.m_vStiffnessTarget	= desc.m_simulationParams.m_vStiffnessTarget;
		}

		pIAttachment->SetSimulationParams(ap);
	}

	QuatT engineLocation = pIAttachment->GetAttAbsoluteDefault();
	QuatT editorLocation = desc.m_relativeCharacterPosition;
	if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q )
		pIAttachment->SetAttAbsoluteDefault(desc.m_relativeCharacterPosition);	
}



static bool ApplySkinAttachment(bool* skinChanged, IAttachment* pIAttachment, ICharacterInstance* character, ICharacterManager* characterManager, const CharacterAttachment& desc, int index)
{
	uint32 type = pIAttachment->GetType();
	IAttachmentObject* iattachmentObject = pIAttachment->GetIAttachmentObject();
	IAttachmentManager* attachmentManager =  character->GetIAttachmentManager();
	
	string existingMaterialFilename;
	string existingBindingFilename;
	bool hasExistingAttachment = false;

	if (iattachmentObject)
	{
		if (iattachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_SkinMesh)
		{
			CSKINAttachment* attachmentObject = (CSKINAttachment*)iattachmentObject;
			if (attachmentObject->m_pIAttachmentSkin)
			{
				if (ISkin* skin = attachmentObject->m_pIAttachmentSkin->GetISkin())
				{
					existingBindingFilename = skin->GetModelFilePath();
					if (IMaterial* material = attachmentObject->GetReplacementMaterial(0))
						existingMaterialFilename = material->GetName();
					hasExistingAttachment = true;
				}
			}
		}
	}

	if (!hasExistingAttachment ||  existingBindingFilename != desc.m_strGeometryFilepath || existingMaterialFilename != NormalizeMaterialName(desc.m_strMaterial.c_str()))
	{
		string fileExt = PathUtil::GetExt( desc.m_strGeometryFilepath.c_str() );
		ISkin* pISkin = 0;
		if (!desc.m_strGeometryFilepath.empty())
		{
			bool isSkin = stricmp(fileExt, CRY_SKIN_FILE_EXT) == 0;
			if (isSkin) 
				pISkin = characterManager->LoadModelSKIN(desc.m_strGeometryFilepath.c_str(), CA_CharEditModel);
		}
		if (pISkin==0)
			return false;

		IAttachment* attachmentSkinAttachment = attachmentManager->GetInterfaceByIndex(index);
		IAttachmentSkin* attachmentSkin = attachmentSkinAttachment ? attachmentSkinAttachment->GetIAttachmentSkin() : 0;
		if (!attachmentSkin)
			return false;

		CSKINAttachment* attachmentObject = new CSKINAttachment();
		attachmentObject->m_pIAttachmentSkin = attachmentSkin;

		_smart_ptr<IMaterial> material;
		if (!desc.m_strMaterial.empty())
			material.reset(gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(NormalizeMaterialName(desc.m_strMaterial.c_str()).c_str(), false));
		attachmentObject->SetReplacementMaterial(material, 0);

		// AddBinding can destroy attachmentObject!
		pIAttachment->AddBinding(attachmentObject, pISkin);
		if (!attachmentSkin->GetISkin())
			return false;

		if (skinChanged)
			*skinChanged = true;
	}

	return true;
}




void CharacterDefinition::ApplyToCharacter(bool* skinSetChanged, ICharacterInstance* pICharacterInstance, ICharacterManager* characterManager)
{
	bool attachmentsDestroyed = false;
	IAttachmentManager* pIAttachmentManager = pICharacterInstance->GetIAttachmentManager();
	vector<string> names;
	for (size_t i = 0; i < pIAttachmentManager->GetAttachmentCount(); ++i) 
	{
		IAttachment* attachment = pIAttachmentManager->GetInterfaceByIndex(i);
		names.push_back(attachment->GetName());
	}


	uint32 numAttachments = attachments.size();
	for (uint32 i=0; i<numAttachments; ++i)
	{
		const CharacterAttachment& attachment = attachments[i];
		const char* strSocketName = attachment.m_strSocketName.c_str();
		int index = pIAttachmentManager->GetIndexByName(strSocketName);

		IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(index);
		if (attachment.m_attachmentType==CA_BONE || attachment.m_attachmentType==CA_FACE || attachment.m_attachmentType==CA_SKIN) 
		{
			if (!pIAttachment || pIAttachment->GetType() != attachment.m_attachmentType || (attachment.m_attachmentType==CA_BONE &&  pIAttachment->GetJointID()!=pICharacterInstance->GetIDefaultSkeleton().GetJointIDByName(attachment.m_strJointName.c_str()) ))
			{
				pIAttachment = pIAttachmentManager->CreateAttachment(attachment.m_strSocketName.c_str(), attachment.m_attachmentType, attachment.m_strJointName.c_str(), true/*, CA_ReplaceExistingAttachments|CA_CharEditModel*/);			
				if (pIAttachment)
					index = pIAttachmentManager->GetIndexByName(pIAttachment->GetName());
			}
		}

		if (pIAttachment==0)
		{
			if (index<0 && attachment.m_attachmentType==CA_PROX)
			{
				//physics-proxies are not in the regular attachment-list
				IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByName(strSocketName);
			  if (pIProxy)
				{
					pIProxy->SetProxyParams(attachment.m_ProxyParams);
					pIProxy->SetProxyPurpose(int8(attachment.m_ProxyPurpose) );

					QuatT engineLocation = pIProxy->GetProxyAbsoluteDefault();
					QuatT editorLocation = attachment.m_relativeCharacterPosition;
					if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q )
						pIProxy->SetProxyAbsoluteDefault(editorLocation);
				}
			}
			continue;
		}

		if (pIAttachment->GetType() != attachment.m_attachmentType)
			continue;

		switch (attachment.m_attachmentType)
		{
			case CA_BONE:
			{
	IDefaultSkeleton& rIDefaultSkeleton = pICharacterInstance->GetIDefaultSkeleton();
	int nJointID = rIDefaultSkeleton.GetJointIDByName(attachments[i].m_strJointName.c_str());
	if (attachments[i].m_alignPosition)
		attachments[i].m_relativeCharacterPosition.t = rIDefaultSkeleton.GetDefaultAbsJointByID(nJointID).t;
	if (attachments[i].m_alignRotation)
		attachments[i].m_relativeCharacterPosition.q = rIDefaultSkeleton.GetDefaultAbsJointByID(nJointID).q;

				ApplyBoneAttachment(pIAttachment, pICharacterInstance, characterManager, attachment);
				break;
			}

			case CA_FACE:
			{
				ApplyFaceAttachment(pIAttachment, pICharacterInstance, characterManager, attachment);
				break;
			}

			case CA_SKIN:
			{
				bool result = ApplySkinAttachment(skinSetChanged, pIAttachment, pICharacterInstance, characterManager, attachment, index);
				if (result==0)
					continue;
				break;
			}

			case CA_VCLOTH:
			{
				break;
			}

		}

		//make sure we don't loose the dyanamic flags
		uint32 nMaskForDynamicFlags=0;
		nMaskForDynamicFlags|=FLAGS_ATTACH_VISIBLE;
		nMaskForDynamicFlags|=FLAGS_ATTACH_PROJECTED;
		nMaskForDynamicFlags|=FLAGS_ATTACH_WAS_PHYSICALIZED;
		nMaskForDynamicFlags|=FLAGS_ATTACH_HIDE_MAIN_PASS;
		nMaskForDynamicFlags|=FLAGS_ATTACH_HIDE_SHADOW_PASS;
		nMaskForDynamicFlags|=FLAGS_ATTACH_HIDE_RECURSION;
		nMaskForDynamicFlags|=FLAGS_ATTACH_NEAREST_NOFOV;
		nMaskForDynamicFlags|=FLAGS_ATTACH_NO_BBOX_INFLUENCE;
		nMaskForDynamicFlags|=FLAGS_ATTACH_COMBINEATTACHMENT;
		uint32 flags = pIAttachment->GetFlags() & nMaskForDynamicFlags;
		pIAttachment->SetFlags(attachment.m_nFlags | flags);
		pIAttachment->HideAttachment((attachment.m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0);

		string lowercaseAttachmentName(attachment.m_strSocketName);
		lowercaseAttachmentName.MakeLower(); // lowercase because the names in the list coming from the engine are lowercase
		names.erase(std::remove(names.begin(), names.end(), lowercaseAttachmentName.c_str()), names.end());
	}

	// remove attachments that weren't updated
	for (size_t i = 0; i < names.size(); ++i)
	{
		pIAttachmentManager->RemoveAttachmentByName(names[i].c_str());
		if (skinSetChanged)
			*skinSetChanged = true;
	}

	SynchModifiers(*pICharacterInstance);
}

void CharacterDefinition::SynchModifiers(ICharacterInstance& character)
{
#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
	IAnimationSerializablePtr pPoseModifierSetup = character.GetISkeletonAnim()->GetPoseModifierSetup();
	if (pPoseModifierSetup == modifiers)
		return;

	MemoryOArchive bo;
	modifiers->Serialize(bo);
	MemoryIArchive bi;
	if (bi.open(bo.buffer(), bo.length()))
		pPoseModifierSetup->Serialize(bi);

	modifiers = pPoseModifierSetup;
#endif
}

}
