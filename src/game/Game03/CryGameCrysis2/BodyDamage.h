#pragma once

#ifndef BODY_DAMAGE_H
#define BODY_DAMAGE_H

struct ISkeletonPose;
struct IAttachmentManager;
struct IPhysicalEntity;
class CPlayer;
struct HitInfo;

class CBodyDamage
{
	typedef std::vector<int> TMaterialIds;
	typedef std::map<int, TMaterialIds> TJointIds;
	typedef std::map<int, int> TEffectiveMaterials;
	typedef std::map<int, TEffectiveMaterials> TEffectiveMaterialsByBone;

	struct SMaterialMappingEntry
	{
		static const int MATERIALS_ARRAY_MAX_SIZE = 24;

		SMaterialMappingEntry();

		int materialsCount;
		int materials[MATERIALS_ARRAY_MAX_SIZE];

		void GetMemoryUsage( ICrySizer *pSizer ) const{}
	};

	class CEffectiveMaterials
	{
	public:
		CEffectiveMaterials(CBodyDamage& bodyDamage, ISkeletonPose& skeletonPose, IPhysicalEntity& physicalEntity);

		void Load(const XmlNodeRef& parentNode);

		void UpdateMapping(int jointId);
		void FinalizeMapping();

	private:
		void UpdateMapping(int jointId, const TEffectiveMaterials& effectiveMaterials);
		void UpdateMapping(int jointId, ISurfaceType& sourceMaterial, ISurfaceType& targetMaterial);

		void LoadEffectiveMaterials(const XmlNodeRef& parentNode, const char* boneName = NULL, int boneId = -1);
		void LoadEffectiveMaterial(const XmlNodeRef& effectiveMaterial, const char* boneName = NULL, int boneId = -1);

		void LogEffectiveMaterialApplied(int gameRuleesSourceMaterialId, const char* sourceMaterial, const char* targetMaterial, int jointId, int materialIndex) const;

		void UpdatePhysicsPartById(const pe_params_part& part, SMaterialMappingEntry& mappingEntry, const TMaterialIds& appliedMaterialIds);

		TEffectiveMaterialsByBone m_effectiveMaterialsByBone;
		TEffectiveMaterials m_effectiveMaterials;

		CBodyDamage& m_bodyDamage;
		ISkeletonPose& m_skeletonPose;
		IPhysicalEntity& m_physicalEntity;

		TJointIds m_jointIdsApplied;
	};

	class CPart
	{
	public:
		CPart(const char* name, uint32 flags, int id);

		const string& GetName() const { return m_name; }
		int GetId() const { return m_id; }
		const TJointIds& GetJointIds() const { return m_jointIds; }
		const TMaterialIds* GetMaterialsByJointId(int jointId) const;
		uint32 GetFlags() const { return m_flags; }

		void LoadElements(const XmlNodeRef& partNode, ISkeletonPose& skeletonPose, IAttachmentManager& attachmentManager, CEffectiveMaterials& effectiveMaterials);

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_name);
		}
	private:
		void AddBone(const XmlNodeRef& boneNode, const char* boneName, ISkeletonPose& skeletonPose, CEffectiveMaterials& effectiveMaterials);

		void AddMaterial(const XmlNodeRef& boneNode, const char* boneName, int boneId, TMaterialIds &materialIds);
		void AddAttachment(const char* attachmentName, IAttachmentManager& attachmentManager);

		static int GetNextId();

		TJointIds m_jointIds;
		string m_name;
		uint32 m_flags;
		int m_id;
	};

	class CPartByNameFunctor : std::unary_function<bool, const CPart&>
	{
	public:
		CPartByNameFunctor(const char* name) : m_name(name) {}
		bool operator()(const CPart& part) const { return 0 == strcmp(m_name, part.GetName().c_str()); }
	private:
		const char* m_name;
	};

	class CPartInfo
	{
	public:
		CPartInfo(const CPart& part, const TMaterialIds& materialIds) : m_part(part) , m_materialIds(materialIds) {}

		const TMaterialIds& GetMaterialIds() const { return m_materialIds; }
		const CPart& GetPart() const { return m_part; }

		void GetMemoryUsage( ICrySizer *pSizer ) const{}
	private:
		const TMaterialIds& m_materialIds;
		const CPart& m_part;
	};

	typedef std::vector<CPart> TParts;
	typedef std::multimap<int, CPartInfo> TPartsByJointId;
	typedef std::map<int, float> TPartIdsToMultipliers;

	typedef TPartsByJointId::const_iterator TPartsByJointIdIterator;
	typedef std::pair<TPartsByJointIdIterator, TPartsByJointIdIterator> TPartsByJointIdRange;

	typedef std::map<int, SMaterialMappingEntry> TMaterialMappingEntries;

public:
	static const int ATTACHMENT_BASE_ID = 1000;

	enum ePID_flags
	{
		PID_None = 0,
		PID_Headshot = BIT(0),
	};

	void Init(const CPlayer& player, bool loadEffectiveMaterials = true);
	void Reload(const CPlayer& player);

	void Physicalize(const CPlayer& player);

	float  GetDamageMultiplier(const CPlayer& player, const HitInfo& hitInfo) const;
	uint32 GetPartFlags(const HitInfo& hitInfo) const;

	SMaterialMappingEntry& InsertMappingEntry(const pe_params_part& part);
	void RemoveMappingEntry(const pe_params_part& part);

	void GetMemoryUsage(ICrySizer *pSizer) const;
private:
	bool GetCharacterInfo(const CPlayer& player, ISkeletonPose*& pSkeletonPose, IAttachmentManager*& pAttachmentManager, IPhysicalEntity*& pPhysicalEntity) const;
	bool GetBodyDamageFileNames(const CPlayer& player, const char*& bodyPartsFileName, const char*& bodyDamageFileName) const;
	XmlNodeRef LoadXml(const char* fileName) const;
	void LoadDamage(const char* bodyDamageFileName);
	void LoadParts(const XmlNodeRef& rootNode, ISkeletonPose& skeletonPose, IAttachmentManager& attachmentManager, CEffectiveMaterials& effectiveMaterials);
	uint32 LoadPartFlags(const XmlNodeRef& partNode) const;
	void LoadMultipliers(const XmlNodeRef& rootNode);
	void LoadMultiplier(const XmlNodeRef& multiplierNode);
	void IndexParts();
	const CPart* FindPart(const HitInfo& hitInfo) const;
	void LogDamageMultiplier(const CPlayer& player, const HitInfo& hitInfo, const char* partName, float multiplierValue) const;
	void LogFoundMaterial(int materialId, const CPartInfo& part, const HitInfo& hitInfo) const;

	TParts m_parts;
	TPartsByJointId m_partsByJointId;
	TPartIdsToMultipliers m_partIdsToMultipliers;
	TMaterialMappingEntries m_effectiveMaterialsMapping;
};

class CBodyDamageCVars
{
	friend class CBodyDamage;
public:
	static void RegisterCommands();
	static void RegisterVariables();
	static void UnregisterCommands(IConsole* pConsole);
	static void UnregisterVariables(IConsole* pConsole);

	static int IsLogEnabled() { return g_bodyDamage_log; }

private:
	static void Reload(IActor* pIActor);
	static void Reload(IConsoleCmdArgs* pArgs);

	static int g_bodyDamage_log;
};

#endif
