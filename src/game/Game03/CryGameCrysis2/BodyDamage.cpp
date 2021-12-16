#include "StdAfx.h"
#include "BodyDamage.h"

#include "Player.h"
#include "GameRules.h"

CBodyDamage::SMaterialMappingEntry::SMaterialMappingEntry()
	: materialsCount(0)
{
	memset(materials, 0, sizeof(materials));
}

CBodyDamage::CEffectiveMaterials::CEffectiveMaterials(CBodyDamage& bodyDamage, ISkeletonPose& skeletonPose, IPhysicalEntity& physicalEntity)
	: m_bodyDamage(bodyDamage)
	, m_skeletonPose(skeletonPose)
	, m_physicalEntity(physicalEntity)
{

}

void CBodyDamage::CEffectiveMaterials::LoadEffectiveMaterials(const XmlNodeRef& parentNode, const char* boneName /*= NULL*/, int boneId /*= -1*/)
{
	for (int index = 0; index < parentNode->getChildCount(); ++index)
	{
		XmlNodeRef node = parentNode->getChild(index);
		if (0 == strcmp(node->getTag(), "EffectiveMaterial"))
		{
			LoadEffectiveMaterial(node, boneName, boneId);
		}
		else if (0 == strcmp(node->getTag(), "Bone"))
		{
			const char* boneName = NULL;
			if (node->getAttr("name", &boneName))
			{
				int boneId = m_skeletonPose.GetJointIDByName(boneName);

				if (boneId >= 0)
					LoadEffectiveMaterials(node, boneName, boneId);
				else
					GameWarning("BodyDamage: Invalid bone name [%s] in effective material", boneName);
			}
		}
	}
}

void CBodyDamage::CEffectiveMaterials::Load(const XmlNodeRef& parentNode)
{
	XmlNodeRef effectiveMaterialsNode = parentNode->findChild("EffectiveMaterials");
	if (effectiveMaterialsNode)
	{
		LoadEffectiveMaterials(effectiveMaterialsNode);
	}
}

void CBodyDamage::CEffectiveMaterials::LoadEffectiveMaterial(const XmlNodeRef& effectiveMaterial, const char* boneName /*= NULL*/, int boneId /*= -1*/)
{
	const char* sourceMaterial = NULL;
	const char* targetMaterial = NULL;

	if (effectiveMaterial->getAttr("source", &sourceMaterial) && effectiveMaterial->getAttr("target", &targetMaterial))
	{
		int sourceMaterialId = g_pGame->GetGameRules()->GetHitMaterialId(sourceMaterial);
		int targetMaterialId = g_pGame->GetGameRules()->GetHitMaterialId(targetMaterial);
		if (sourceMaterialId && targetMaterialId)
		{
			if (boneName && boneId >= 0)
			{
				if (!m_effectiveMaterialsByBone[boneId].insert(std::make_pair(sourceMaterialId, targetMaterialId)).second)
					GameWarning("BodyDamage: EffectiveMaterial source already used [%s] in bone [%s]", sourceMaterial, boneName);
			}
			else
			{
				if (!m_effectiveMaterials.insert(std::make_pair(sourceMaterialId, targetMaterialId)).second)
					GameWarning("BodyDamage: EffectiveMaterial source already used [%s]", sourceMaterial);
			}
		}
		else
		{
			if (!sourceMaterial)
				GameWarning("BodyDamage: Can't find source EffectiveMaterial [%s]", sourceMaterial);

			if (!targetMaterialId)
				GameWarning("BodyDamage: Can't find target EffectiveMaterial [%s]", targetMaterial);
		}
	}
}

void CBodyDamage::CEffectiveMaterials::UpdateMapping(int jointId)
{
	TEffectiveMaterialsByBone::const_iterator effectiveMaterialsByBone = m_effectiveMaterialsByBone.find(jointId);
	if (effectiveMaterialsByBone != m_effectiveMaterialsByBone.end())
		UpdateMapping(jointId, effectiveMaterialsByBone->second);
	else
		UpdateMapping(jointId, m_effectiveMaterials);
}

void CBodyDamage::CEffectiveMaterials::UpdateMapping(int jointId, const TEffectiveMaterials& effectiveMaterials)
{
	for (TEffectiveMaterials::const_iterator it = effectiveMaterials.begin(); it != effectiveMaterials.end(); ++it)
	{
		ISurfaceType* pSourceMaterial = g_pGame->GetGameRules()->GetHitMaterial(it->first);
		ISurfaceType* pTargetMaterial = g_pGame->GetGameRules()->GetHitMaterial(it->second);

		if (pSourceMaterial && pTargetMaterial)
		{
			UpdateMapping(jointId, *pSourceMaterial, *pTargetMaterial);
		}
	}
}

void CBodyDamage::CEffectiveMaterials::UpdateMapping(int jointId, ISurfaceType& sourceMaterial, ISurfaceType& targetMaterial)
{
	int sourceMaterialId = sourceMaterial.GetId();
	int targetMaterialId = targetMaterial.GetId();

	pe_params_part part;
	part.partid = jointId;

	if (m_physicalEntity.GetParams(&part) && part.pMatMapping)
	{
		TMaterialIds& appliedMaterialIds = m_jointIdsApplied.insert(std::make_pair(jointId, TMaterialIds())).first->second;
		SMaterialMappingEntry& mappingEntry = m_bodyDamage.InsertMappingEntry(part);

		for (int materialIndex = 0; materialIndex < mappingEntry.materialsCount; ++materialIndex)
		{
			if (mappingEntry.materials[materialIndex] == sourceMaterialId)
			{
				if (!stl::find(appliedMaterialIds, materialIndex))
				{
					mappingEntry.materials[materialIndex] = targetMaterialId;
					appliedMaterialIds.push_back(materialIndex);

					LogEffectiveMaterialApplied(-1, sourceMaterial.GetName(), targetMaterial.GetName(), jointId, materialIndex);
				}
			}
		}

		UpdatePhysicsPartById(part, mappingEntry, appliedMaterialIds);
	}
}

void CBodyDamage::CEffectiveMaterials::FinalizeMapping()
{
	pe_status_nparts status_nparts;
	int nParts = m_physicalEntity.GetStatus(&status_nparts);
	for (int partIndex = 0; partIndex < nParts; partIndex++)
	{
		pe_params_part part;
		part.ipart = partIndex;

		if (m_physicalEntity.GetParams(&part) && part.partid < CBodyDamage::ATTACHMENT_BASE_ID)
		{
			TMaterialIds& appliedMaterialIds = m_jointIdsApplied.insert(std::make_pair(part.partid, TMaterialIds())).first->second;
			SMaterialMappingEntry& mappingEntry = m_bodyDamage.InsertMappingEntry(part);
			for (int materialIndex = 0; materialIndex < mappingEntry.materialsCount; ++materialIndex)
			{
				if (!stl::find(appliedMaterialIds, materialIndex))
				{
					int gameRulesSourceMaterialId = g_pGame->GetGameRules()->GetHitMaterialIdFromSurfaceId(part.pMatMapping[materialIndex]);
					TEffectiveMaterials::const_iterator gameRulesTargetMaterialIt = m_effectiveMaterials.find(gameRulesSourceMaterialId);
					if (gameRulesTargetMaterialIt != m_effectiveMaterials.end())
					{
						ISurfaceType* pTargetMaterial = g_pGame->GetGameRules()->GetHitMaterial(gameRulesTargetMaterialIt->second);
						if (pTargetMaterial)
						{
							int targetMaterialId = pTargetMaterial->GetId();

							mappingEntry.materials[materialIndex] = targetMaterialId;
							appliedMaterialIds.push_back(materialIndex);

							LogEffectiveMaterialApplied(gameRulesSourceMaterialId, NULL, pTargetMaterial->GetName(), part.partid, materialIndex);
						}
					}
				}
			}

			UpdatePhysicsPartById(part, mappingEntry, appliedMaterialIds);
		}
	}
}

void CBodyDamage::CEffectiveMaterials::UpdatePhysicsPartById(const pe_params_part& part, SMaterialMappingEntry& mappingEntry, const TMaterialIds& appliedMaterialIds)
{
	if (appliedMaterialIds.empty())
	{
		m_bodyDamage.RemoveMappingEntry(part);
	}
	else
	{
		pe_params_part changedPart;
		changedPart.ipart = part.ipart;
		changedPart.pMatMapping = mappingEntry.materials;
		changedPart.nMats = mappingEntry.materialsCount;
		m_physicalEntity.SetParams(&changedPart);
	}
}

CBodyDamage::SMaterialMappingEntry& CBodyDamage::InsertMappingEntry(const pe_params_part& part)
{
	std::pair<std::map<int, SMaterialMappingEntry>::iterator, bool> insertResult = m_effectiveMaterialsMapping.insert(std::make_pair(part.partid, SMaterialMappingEntry()));
	SMaterialMappingEntry& insertedEntry = insertResult.first->second;
	if (insertResult.second)
	{
		if (part.nMats <= SMaterialMappingEntry::MATERIALS_ARRAY_MAX_SIZE)
		{
			insertedEntry.materialsCount = part.nMats;
			memcpy(insertedEntry.materials, part.pMatMapping, sizeof(int) * part.nMats);
		}
		else
		{
			GameWarning("Not enough room to clone materials mapping");
			insertedEntry.materialsCount = SMaterialMappingEntry::MATERIALS_ARRAY_MAX_SIZE;
			memcpy(insertedEntry.materials, part.pMatMapping, sizeof(insertedEntry.materials));
		}
	}
	return insertedEntry;
}

void CBodyDamage::RemoveMappingEntry(const pe_params_part& part)
{
	m_effectiveMaterialsMapping.erase(part.partid);
}

void CBodyDamage::Physicalize(const CPlayer& player)
{
	ISkeletonPose* pSkeletonPose = NULL;
	IAttachmentManager* pAttachmentManager = NULL;
	IPhysicalEntity* pPhysicalEntity = NULL;

	if (GetCharacterInfo(player, pSkeletonPose, pAttachmentManager, pPhysicalEntity))
	{
		std::map<int, SMaterialMappingEntry>::iterator itEnd = m_effectiveMaterialsMapping.end();
		for (std::map<int, SMaterialMappingEntry>::iterator it = m_effectiveMaterialsMapping.begin(); it != itEnd; ++it)
		{
			pe_params_part part;
			part.partid = it->first;
			part.pMatMapping = it->second.materials;
			part.nMats = it->second.materialsCount;
			pPhysicalEntity->SetParams(&part);
		}
	}
}

void CBodyDamage::CEffectiveMaterials::LogEffectiveMaterialApplied(int gameRuleesSourceMaterialId, const char* sourceMaterial, const char* targetMaterial, int jointId, int materialIndex) const
{
	if (CBodyDamageCVars::IsLogEnabled())
	{
		if (!sourceMaterial)
		{
			if (ISurfaceType* pSourceMaterial = g_pGame->GetGameRules()->GetHitMaterial(gameRuleesSourceMaterialId))
				sourceMaterial = pSourceMaterial->GetName();
		}

		CryLog("BodyDamage: Applying effective material Source [%s] Target [%s] BoneName [%s] BoneId [%d] Index [%d]", 
			sourceMaterial, targetMaterial, m_skeletonPose.GetJointNameByID(jointId), jointId, materialIndex);
	}
}

CBodyDamage::CPart::CPart(const char* name, uint32 flags, int id)
	: m_name(name)
	, m_flags(flags)
	, m_id(id)
{

}

int CBodyDamage::CPart::GetNextId()
{
	static int idGenerator = 0;
	return idGenerator++;
}

const CBodyDamage::TMaterialIds* CBodyDamage::CPart::GetMaterialsByJointId(int jointId) const
{
	TJointIds::const_iterator foundMaterial = m_jointIds.find(jointId);
	return foundMaterial != m_jointIds.end() ? &foundMaterial->second : NULL;
}

void CBodyDamage::CPart::LoadElements(const XmlNodeRef& partNode, ISkeletonPose& skeletonPose, IAttachmentManager& attachmentManager, CEffectiveMaterials& effectiveMaterials)
{
	for (int partElemIndex = 0; partElemIndex < partNode->getChildCount(); ++partElemIndex)
	{
		XmlNodeRef partElem = partNode->getChild(partElemIndex);
		if (0 == strcmp("Bone", partElem->getTag()))
		{
			const char* boneName = NULL;
			if (partElem->getAttr("name", &boneName))
				AddBone(partElem, boneName, skeletonPose, effectiveMaterials);
		}
		else if (0 == strcmp("Attachment", partElem->getTag()))
		{
			const char* attachmentName = NULL;
			if (partElem->getAttr("name", &attachmentName))
				AddAttachment(attachmentName, attachmentManager);
		}
	}
}

void CBodyDamage::CPart::AddBone(const XmlNodeRef& boneNode, const char* boneName, ISkeletonPose& skeletonPose, CEffectiveMaterials& effectiveMaterials)
{
	int16 boneId = skeletonPose.GetJointIDByName(boneName);

	if (boneId >= 0)
	{
		std::pair<TJointIds::iterator, bool> insertResult = m_jointIds.insert(std::make_pair(boneId, TMaterialIds()));
		if (insertResult.second)
		{
			TMaterialIds& materialIds = insertResult.first->second;

			IMaterialManager* pMaterialManager = gEnv->p3DEngine->GetMaterialManager();
			CRY_ASSERT(pMaterialManager);
			if (pMaterialManager)
				AddMaterial(boneNode, boneName, boneId, materialIds);

			effectiveMaterials.UpdateMapping(boneId);
		}
		else
			GameWarning("BodyDamage: Bone name [%s] is already available in part [%s]", boneName, m_name.c_str());
	}
	else
		GameWarning("BodyDamage: Invalid bone name [%s] for part [%s]", boneName, m_name.c_str());
}

void CBodyDamage::CPart::AddMaterial(const XmlNodeRef& boneNode, const char* boneName, int boneId, TMaterialIds &materialIds) 
{
	for (int materialNodeIndex = 0; materialNodeIndex < boneNode->getChildCount(); ++materialNodeIndex)
	{
		XmlNodeRef materialNode = boneNode->getChild(materialNodeIndex);
		if (0 == strcmp("Material", materialNode->getTag()))
		{
			const char* materialName = NULL;
			if (materialNode->getAttr("name", &materialName))
			{
				if (int materialId = g_pGame->GetGameRules()->GetHitMaterialId(materialName))
					materialIds.push_back(materialId);
				else
					GameWarning("BodyDamage: Invalid material name [%s] in bone [%s] for part [%s]", materialName, boneName, m_name.c_str());
			}
		}
	}
}

void CBodyDamage::CPart::AddAttachment(const char* attachmentName, IAttachmentManager& attachmentManager)
{
	int32 attachmentId = attachmentManager.GetIndexByName(attachmentName);

	if (attachmentId >= 0)
	{
		if (!m_jointIds.insert(std::make_pair(attachmentId + ATTACHMENT_BASE_ID, TMaterialIds())).second)
			GameWarning("BodyDamage: Attachment name [%s] is already available in part [%s]", attachmentName, m_name.c_str());
	}
	else
		GameWarning("BodyDamage: Invalid attachment name [%s] for part [%s]", attachmentName, m_name.c_str());
}

bool CBodyDamage::GetBodyDamageFileNames(const CPlayer& player, const char*& bodyPartsFileName, const char*& bodyDamageFileName) const
{
	bool result = false;

	SmartScriptTable propertiesTable;
	if (player.GetEntity()->GetScriptTable() && player.GetEntity()->GetScriptTable()->GetValue("Properties", propertiesTable))
	{
		SmartScriptTable damageTable;
		if (propertiesTable->GetValue("Damage", damageTable))
		{
			result = damageTable->GetValue("fileBodyDamageParts", bodyPartsFileName) && damageTable->GetValue("fileBodyDamage", bodyDamageFileName);
		}
	}

	return result;
}

bool CBodyDamage::GetCharacterInfo(const CPlayer& player, ISkeletonPose*& pSkeletonPose, IAttachmentManager*& pAttachmentManager, IPhysicalEntity*& pPhysicalEntity) const
{
	IEntity* pEntity = player.GetEntity();
	CRY_ASSERT(pEntity);
	if (!pEntity)
		return false;

	ICharacterInstance* pCharacterInstance = player.GetEntity()->GetCharacter(0);
	CRY_ASSERT(pCharacterInstance);
	if (!pCharacterInstance)
		return false;

	pSkeletonPose = pCharacterInstance->GetISkeletonPose();
	CRY_ASSERT(pSkeletonPose);
	if (!pSkeletonPose)
		return false;

	pAttachmentManager = pCharacterInstance->GetIAttachmentManager();
	CRY_ASSERT(pAttachmentManager);
	if (!pAttachmentManager)
		return false;

	pPhysicalEntity = pSkeletonPose->GetCharacterPhysics();
	CRY_ASSERT(pPhysicalEntity);
	if (!pPhysicalEntity)
		return false;

	return true;
}

void CBodyDamage::Init(const CPlayer& player, bool loadEffectiveMaterials /*= true */)
{
	ISkeletonPose* pSkeletonPose= NULL;
	IAttachmentManager* pAttachmentManager = NULL;
	IPhysicalEntity* pPhysicalEntity = NULL;

	if (!GetCharacterInfo(player, pSkeletonPose, pAttachmentManager, pPhysicalEntity))
		return;

	const char* bodyPartsFileName = "";
	const char* bodyDamageFileName = "";

	if (!GetBodyDamageFileNames(player, bodyPartsFileName, bodyDamageFileName))
		return;

	if (CBodyDamageCVars::g_bodyDamage_log) 
		CryLog("BodyDamage: Loading XML files BodyParts [%s] BodyDamage [%s]", bodyPartsFileName, bodyDamageFileName);

	XmlNodeRef partsRootNode = LoadXml(bodyPartsFileName);
	XmlNodeRef damageRootNode = LoadXml(bodyDamageFileName);

	if (partsRootNode && damageRootNode)
	{
		CEffectiveMaterials effectiveMaterials(*this, *pSkeletonPose, *pPhysicalEntity);

		if (loadEffectiveMaterials) 
			effectiveMaterials.Load(damageRootNode);

		LoadParts(partsRootNode, *pSkeletonPose, *pAttachmentManager, effectiveMaterials);
		LoadMultipliers(damageRootNode);

		if (loadEffectiveMaterials) 
			effectiveMaterials.FinalizeMapping();
	}
}

XmlNodeRef CBodyDamage::LoadXml(const char* fileName) const
{
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile(fileName);
	if (rootNode && 0 == strcmp(rootNode->getTag(), "BodyDamage"))
		return rootNode;

	return XmlNodeRef();
}

void CBodyDamage::LoadMultipliers(const XmlNodeRef& rootNode)
{
	XmlNodeRef multipliersNode = rootNode->findChild("Multipliers");
	if (multipliersNode)
	{
		for (int multiplierIndex = 0; multiplierIndex < multipliersNode->getChildCount(); ++multiplierIndex)
		{
			XmlNodeRef multiplierNode = multipliersNode->getChild(multiplierIndex);
			LoadMultiplier(multiplierNode);
		}
	}
}

void CBodyDamage::LoadParts(const XmlNodeRef& rootNode, ISkeletonPose& skeletonPose, IAttachmentManager& attachmentManager, CEffectiveMaterials& effectiveMaterials)
{
	XmlNodeRef partsNode = rootNode->findChild("Parts");
	if (partsNode)
	{
		for (int partIndex = 0; partIndex < partsNode->getChildCount(); ++partIndex)
		{
			XmlNodeRef partNode = partsNode->getChild(partIndex);
			const char* partName = NULL;
			if (partNode->getAttr("name", &partName))
			{
				CPart part(partName, LoadPartFlags(partNode), partIndex);
				part.LoadElements(partNode, skeletonPose, attachmentManager, effectiveMaterials);
				m_parts.push_back(part);
			}
		}
		IndexParts();
	}
}

uint32 CBodyDamage::LoadPartFlags(const XmlNodeRef& partNode) const
{
	uint32 flags = PID_None;

	const char* flagString = NULL;
	if (partNode->getAttr("flags", &flagString))
	{
		if(strstr("headshot", flagString))
		{
			flags |= PID_Headshot;
		}
	}
	return flags;
}

void CBodyDamage::LoadMultiplier(const XmlNodeRef& multiplierNode)
{
	const char* partName = NULL;
	float multiplierValue = NULL;
	if (multiplierNode->getAttr("part", &partName) && multiplierNode->getAttr("value", multiplierValue))
	{
		TParts::const_iterator foundPart = std::find_if(m_parts.begin(), m_parts.end(), CPartByNameFunctor(partName));
		if (foundPart != m_parts.end())
			m_partIdsToMultipliers.insert(std::make_pair(foundPart->GetId(), multiplierValue));
		else
			GameWarning("BodyDamage: PartName [%s] not found in Multiplier node", partName);
	}
}

void CBodyDamage::IndexParts()
{
	for (TParts::const_iterator itParts = m_parts.begin(); itParts != m_parts.end(); ++itParts)
	{
		const CPart& part = *itParts;
		for (TJointIds::const_iterator itJointIds = part.GetJointIds().begin(); itJointIds != part.GetJointIds().end(); ++itJointIds)
		{
			int jointId = itJointIds->first;
			const TMaterialIds* materials = part.GetMaterialsByJointId(jointId);
			CRY_ASSERT(materials);
			if (materials)
				m_partsByJointId.insert(std::make_pair(jointId, CPartInfo(part, *materials)));

			CRY_TODO(13, 01, 2010, "Add consistency checks");
		}
	}
}

const CBodyDamage::CPart* CBodyDamage::FindPart(const HitInfo& hitInfo) const
{
	const CPart* part = NULL;

	TPartsByJointIdRange partsRange = m_partsByJointId.equal_range(hitInfo.partId);
	for (; partsRange.first != partsRange.second; ++partsRange.first)
	{
		const CPartInfo& currentPart = partsRange.first->second;

		const TMaterialIds& materialIds = currentPart.GetMaterialIds();
		if (materialIds.empty())
		{
			part = &currentPart.GetPart();
		}
		else
		{
			TMaterialIds::const_iterator foundMaterial = std::find(materialIds.begin(), materialIds.end(), hitInfo.material);
			if (foundMaterial != materialIds.end())
			{
				if (CBodyDamageCVars::g_bodyDamage_log)
					LogFoundMaterial(*foundMaterial, currentPart, hitInfo);

				part = &currentPart.GetPart();
				break;
			}
		}
	}

	return part;
}

float CBodyDamage::GetDamageMultiplier(const CPlayer& player, const HitInfo& hitInfo) const
{
	float result = 1.0f;
	const char* partName = "None";

	if (const CPart* part = FindPart(hitInfo))
	{
		TPartIdsToMultipliers::const_iterator foundMultiplier = m_partIdsToMultipliers.find(part->GetId());
		if (foundMultiplier != m_partIdsToMultipliers.end())
		{
			partName = part->GetName().c_str();
			result = foundMultiplier->second;
		}
	}

	if (CBodyDamageCVars::g_bodyDamage_log)
		LogDamageMultiplier(player, hitInfo, partName, result);

	return result;
}

uint32 CBodyDamage::GetPartFlags(const HitInfo& hitInfo) const
{
	if (const CPart* part = FindPart(hitInfo))
	{
		return part->GetFlags();
	}

	return 0;
}

void CBodyDamage::LogDamageMultiplier(const CPlayer& player, const HitInfo& hitInfo, const char* partName,  float multiplierValue) const
{
	const char* materialName = "";
	if (ISurfaceType* surfaceType = g_pGame->GetGameRules()->GetHitMaterial(hitInfo.material))
		materialName = surfaceType->GetName();

	const char* jointName = "";
	bool isAttachment = hitInfo.partId >= ATTACHMENT_BASE_ID;

	if (ICharacterInstance* pCharacterInstance = player.GetEntity()->GetCharacter(0))
	{
		if (isAttachment)
		{
			IAttachmentManager* pAttachmentManager = pCharacterInstance->GetIAttachmentManager();
			if (IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(hitInfo.partId - ATTACHMENT_BASE_ID))
				jointName = pAttachment->GetName();
		}
		else
		{
			ISkeletonPose* pSkeletonPose = pCharacterInstance->GetISkeletonPose();
			jointName = pSkeletonPose->GetJointNameByID(hitInfo.partId);
		}

		CryLog("BodyDamage: Part [%s] JointId [%d] JointName [%s] IsAttachment [%d] Material [%s] MaterialId [%d] Multiplier [%f]", 
			partName, hitInfo.partId, jointName, isAttachment ? 1 : 0, materialName, hitInfo.material, multiplierValue);
	}
}

void CBodyDamage::LogFoundMaterial(int materialId, const CPartInfo& part, const HitInfo& hitInfo) const
{
	const char* materialName = "";
	if (ISurfaceType* surfaceType = g_pGame->GetGameRules()->GetHitMaterial(materialId))
		materialName = surfaceType->GetName();

	CryLog("BodyDamage: Matched MaterialId [%d] MaterialName [%s] Part [%s] JointId [%d] JointName", 
		materialId, materialName, part.GetPart().GetName().c_str(), hitInfo.partId);
}

void CBodyDamage::Reload(const CPlayer& player)
{
	m_parts.clear();
	m_partsByJointId.clear();
	m_partIdsToMultipliers.clear();

	Init(player, false);
}

void CBodyDamage::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_parts);	
	//pSizer->AddObject(m_partsByJointId);
	pSizer->AddObject(m_partIdsToMultipliers);
	pSizer->AddObject(m_effectiveMaterialsMapping);		
}

int CBodyDamageCVars::g_bodyDamage_log = 0;

void CBodyDamageCVars::RegisterCommands()
{
	REGISTER_COMMAND("g_bodyDamage_reload", Reload, VF_CHEAT, "Reloads bodyDamage for the specified actor, or for everyone if not specified");
}

void CBodyDamageCVars::UnregisterCommands(IConsole* pConsole)
{
	if (pConsole)
	{
		pConsole->RemoveCommand("g_bodyDamage_reload");
	}
}

void CBodyDamageCVars::RegisterVariables()
{
	REGISTER_CVAR(g_bodyDamage_log, 0, 0, "Enables/Disables BodyDamage logging");
}

void CBodyDamageCVars::UnregisterVariables(IConsole* pConsole)
{
	if (pConsole)
	{
		pConsole->UnregisterVariable("g_bodyDamage_log", true);
	}
}

void CBodyDamageCVars::Reload(IActor* actor)
{
	if (CPlayer::GetActorClassType() == actor->GetActorClass())
	{
		CPlayer* player = static_cast<CPlayer*>(actor);
		player->GetBodyDamage()->Reload(*player);
	}
}

void CBodyDamageCVars::Reload(IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() > 1)
	{
		IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(pArgs->GetArg(1));
		if (pEntity)
		{
			IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
			if (pActor)
				Reload(pActor);
		}
	}
	else
	{
		IActorIteratorPtr pIt = g_pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		while (IActor* pActor = pIt->Next())
			Reload(pActor);
	}
}
