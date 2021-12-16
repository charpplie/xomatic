////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	28/09/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  high-level loading of characters
/////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_CHAR_MANAGER_HEADER_
#define _CRY_CHAR_MANAGER_HEADER_

#include <ICryAnimation.h>
#include "Model.h"

class CCharacterModel;
class CAnimObjectManager;
class CFacialAnimation;
class CAttachment;

extern float g_YLine;


// Anim prototyping types. Allows extraction of asset names from engine source into the animation prototyping file

struct SAPFootplant	// Asset footplant data loaded from anim prototyping xml.
{
	string m_sAssetName;
	int m_segNo;
	float m_A, m_B, m_C,m_D,m_E,m_F;
};

struct SAPAimPoseMatch	
{
	string m_sMatch1;
	string m_sMatch2;
};


struct CharacterAttachment
{
	CharacterAttachment() : WRotation(IDENTITY), WPosition(ZERO), BodeId(~0), AttFlags(0), Idx(-1), Limit(120.0f), Damping(05.0f)
	{
		memset(PhysInfo, 0, sizeof(PhysInfo));
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(Name);
		pSizer->AddObject(BoneName);
		pSizer->AddObject(BindingPath);
		pSizer->AddObject(Material);
	}
	Quat WRotation;
	Vec3 WPosition;

	uint32 Type;
	uint32 BodeId;
	uint32 AttFlags;

	int		Idx;
	float Limit;
	float Damping;

	string Name;
	string BoneName;
	string BindingPath;
	string Material;

	CryBonePhysics PhysInfo[2];
};


struct CharacterDefinition
{
	CharacterDefinition()
	{
		memset(Morphing, 0, sizeof(Morphing));
		m_nRefCounter=0;
	}

	virtual void AddRef()	
	{
		++m_nRefCounter;
	}
	
	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(FilePath);
		pSizer->AddObject(Name);
		pSizer->AddObject(Model);
		pSizer->AddObject(Material);
		pSizer->AddObject(arrAttachments);		
	}
	float Morphing[8];

	string FilePath;
	string Name;
	string Model;
	string Material;
	int m_nRefCounter;	// Reference count.

	std::vector<CharacterAttachment> arrAttachments;
};

//////////////////////////////////////////////////////////////////////
// This class contains a list of character bodies and list of character instances.
// On attempt to create same character second time only new instance will be created.
// Only this class can create actual characters.
class CharacterManager : public ICharacterManager
{
public:
	VIRTUAL void SyncAllAnimations();
	VIRTUAL void AddAnimationToSyncQueue(ICharacterInstance *pCharInstance);
	VIRTUAL void RemoveAnimationToSyncQueue( ICharacterInstance *pCharInstance );

	// Loads a cgf and the corresponding caf file and creates an animated object, or returns an existing object.
	ICharacterInstance* CreateInstance(const char * szFileName, uint32 IsSkinAtt=0, IAttachment* pIMasterAttachment=0 );

	VIRTUAL void GetLoadedModels( ICharacterModel** pCharacterModels,int &nCount );

	VIRTUAL void ReloadModel(ICharacterModel* pModel);
	VIRTUAL void ReloadAllModels();

	ICharacterInstance* CreateCGAInstance( const char* strPath );
	ICharacterInstance* CreateCHRInstance( const char* strPath, uint32 nFlags = 0, IAttachment* pMasterAttachment=0 );
	void SetFootPlantsFlag(CCharInstance* pCryCharInstance); 

	// Deletes itself
	virtual void Release();

	void RegisterModel (CCharacterModel*);
	void UnregisterModel (CCharacterModel*);

	void AddToEraseList( CSkinInstance* pInstance );
	void CleanEraseList( bool bForceCleanAll );

	// puts the size of the whole subsystem into this sizer object, classified,
	// according to the flags set in the sizer
	void GetMemoryUsage( ICrySizer* pSizer)const;

	// puts the size of ModelCache
	void GetModelCacheSize(class ICrySizer* pSizer)const;
	void GetCharacterInstancesSize(class ICrySizer* pSizer)const;


	// should be called every frame
	void Update();
	void DebugModelCache();
	void DebugAnimUsage();
	void DatabaseUnloading();

	// can be called instead of Update() for UI purposes (such as in preview viewports, etc).
	void DummyUpdate();

	//! Cleans up all resources - currently deletes all bodies and characters (even if there are references on them)
	VIRTUAL void ClearResources( bool bForceCleanup );



	// returns statistics about this instance of character animation manager
	// don't call this too frequently
	void GetStatistics(Statistics& rStats);

	//! Locks all models in memory
	void LockResources();

	//! Unlocks all models in memory
	void UnlockResources();

	// asserts the cache validity
	void ValidateModelCache();

	// deletes all registered bodies; the character instances got deleted even if they're still referenced
	void CleanupModelCache( bool bForceCleanup );


	// locks or unlocks the given body if needed, depending on the hint and console variables
	void DecideModelLockStatus(CCharacterModel* pBody, uint32 nHints);

	ICharacterInstance* LoadCharacterDefinition(const string pathname, uint32 IsSkinAtt=0, CCharInstance* pSkelInstance=0 );
	uint32 SaveCharacterDefinition(ICharacterInstance* ptr, const char* pathname );
	int32 LoadCDF(const char* pathname );
	void ReleaseCDF(const char* pathname);

	void LockModel( CCharacterModel* pModel );

	//////////////////////////////////////////////////////////////////////////
	IFacialAnimation* GetIFacialAnimation();
	IAnimEvents* GetIAnimEvents() { return &g_AnimationManager; };

	CAnimationManager& GetAnimationManager() { return m_AnimationManager; };

	const Vec2& GetScalingLimits() const { return m_scalingLimits; }
	void SetScalingLimits( const Vec2& limits ) { m_scalingLimits = limits; }


	CFacialAnimation* GetFacialAnimation() {return m_pFacialAnimation;}

	//a list with model-names that use "ForceSkeletonUpdates"
	std::vector<string> m_arrSkeletonUpdates;
	std::vector<uint32> m_arrAnimPlaying;
	std::vector<uint32> m_arrForceSkeletonUpdates;
	std::vector<uint32> m_arrVisible;
	uint32 m_nUpdateCounter;
	uint32 m_IsDedicatedServer;


	CharacterManager ();
	~CharacterManager ();

	//--- Anim Prototype data parsed from the AnimPrototype.ap file
	typedef std::vector<SAPFootplant> TFootPlantTable;
	TFootPlantTable m_arrFootPlantTable;

	void LoadAnimPrototypeFile(const char* psFilePath);

private:
	void CreateFootplantTableFromXML(XmlNodeRef & rXml);

	void DumpAssetStatistics();
	f32 GetAverageFrameTime(f32 sec, f32 FrameTime,f32 TimeScale, f32 LastAverageFrameTime); 

	// Finds a cached or model.
	CCharacterModel* CheckIfModelLoaded (const string& strFileName);
	// Finds a cached or creates a new CCharacterModel instance and returns it
	// returns NULL if the construction failed
	CCharacterModel* FetchModel (const string& strFileName,uint32 sw,uint32 la);
	uint32 m_AIF_Initilization;
	void LoadAnimationImageFile( const char* filenameCAF,const char* filenameAIM );
	bool LoadAnimationImageFileCAF( const char* filenameCAF, IChunkFile* pChunkFile );
	bool LoadAnimationImageFileAIM( const char* filenameAIM, IChunkFile* pChunkFile );

	class OrderByFileName
	{
	public:
		bool operator () (const CCharacterModel* pLeft, const CCharacterModel* pRight);
		bool operator () (const string& strLeft, const CCharacterModel* pRight);
		bool operator () (const CCharacterModel* pLeft, const string& strRight);
	};
	// this is the set of all existing instances of crycharbody
	typedef std::vector<CCharacterModel*> CryModelCache;
	std::vector<CCharacterModel*> m_arrModelCache;

	// temporary locked objects with LockResources
	CCharacterModel_AutoArray m_arrTempLock;

	// this is the array of bodies that are kept locked even if the number of characters drops to zero
	CCharacterModel_AutoSet m_setLockedModels;

	CAnimationManager m_AnimationManager;

	CFacialAnimation*		m_pFacialAnimation;

	std::vector<f32> m_arrFrameTimes;
	//f32 m_OldAbsTime;
	//f32 m_NewAbsTime;
	
	Vec2 m_scalingLimits;

	std::vector<CharacterDefinition> m_cdfFiles;
  typedef std::list< std::pair< CCharInstance*, float> > TEraseList;
  TEraseList m_EraseList;

	std::vector<ICharacterInstance*> m_AnimationSyncQueue;	// queue to remember all animations which were started and need a sync

	// Erase list remember at which frame the instance was deleted, should skip at least one render frame to delete, because renderer may use it still.
	bool m_bImmidiateInstanceDeleteMode;
	std::vector< std::pair<int,CSkinInstance*> > m_InstancesEraseList;

};


#endif
