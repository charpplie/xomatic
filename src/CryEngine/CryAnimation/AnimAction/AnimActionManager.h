#ifndef __ANIMACTIONMANAGER_H__
#define __ANIMACTIONMANAGER_H__

#include <IAnimAction.h>
#include <NameCRCHelper.h>

//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalHeaderAAC : public CNameCRCHelper
{
	friend class CAnimationManager;
	friend class CAnimationSet;

	GlobalHeaderAAC ()
	{
		m_nRefCount=0;
		m_pAnimActionPrototype = NULL;
		m_nFlags = 0;
	}

	virtual ~GlobalHeaderAAC()
	{
		m_pAnimActionPrototype = NULL;
	};

	const char * GetPathName() const { return GetName(); };
	int GetCRCPathName() { return m_CRC32Name; }
	void SetPathName(const string& name) { /*SetNameChar(name.c_str())*/; SetName(name); };


	ILINE uint32 IsAssetLoaded() const {return m_nFlags&CA_ASSET_LOADED;}
	ILINE void OnAssetLoaded() {m_nFlags |= CA_ASSET_LOADED;}

	ILINE uint32 IsAimpose() const {return m_nFlags&CA_AIMPOSE;}
	ILINE void OnAimpose() {m_nFlags |= CA_AIMPOSE;}

	ILINE uint32 IsAimposeUnloaded() const {return m_nFlags&CA_AIMPOSE_UNLOADED;}
	ILINE void OnAimposeUnloaded() {m_nFlags |= CA_AIMPOSE_UNLOADED;}

	ILINE void ClearAssetLoaded() {m_nFlags &= ~CA_ASSET_LOADED;}

	ILINE uint32 IsAssetCreated() const {return m_nFlags&CA_ASSET_CREATED;}
	ILINE void OnAssetCreated() {m_nFlags |= CA_ASSET_CREATED;}

	ILINE uint32 IsAssetAdditive() const { return m_nFlags&CA_ASSET_ADDITIVE; }
	ILINE void OnAssetAdditive() { m_nFlags |= CA_ASSET_ADDITIVE; }

	ILINE uint32 IsAssetCycle() const {return m_nFlags&CA_ASSET_CYCLE;}
	ILINE void OnAssetCycle() {m_nFlags |= CA_ASSET_CYCLE;}

	ILINE uint32 IsAssetLMG() const {return m_nFlags&CA_ASSET_LMG;}
	ILINE void OnAssetLMG() {m_nFlags |= CA_ASSET_LMG;}
	ILINE uint32 IsAssetLMGValid()const { return m_nFlags&CA_ASSET_LMG_VALID; }
	ILINE void OnAssetLMGValid() { m_nFlags |= CA_ASSET_LMG_VALID; }

	ILINE uint32 IsAssetAAC() const {return m_nFlags&CA_ASSET_AAC;}
	ILINE void OnAssetAAC() {m_nFlags |= CA_ASSET_AAC;}
	ILINE uint32 IsAssetAACValid()const { return m_nFlags&CA_ASSET_AAC_VALID; }
	ILINE void OnAssetAACValid() { m_nFlags |= CA_ASSET_AAC_VALID; }


	ILINE uint32 IsAssetProcessed()const { return m_nFlags&CA_ASSET_PROCESSED; }
	ILINE void OnAssetProcessed() { m_nFlags |= CA_ASSET_PROCESSED; }
	ILINE void ClearAssetProcessed() {m_nFlags &= ~CA_ASSET_PROCESSED;}

	ILINE uint32 IsAssetRequested()const { return m_nFlags&CA_ASSET_REQUESTED; }
	ILINE void OnAssetRequested() { m_nFlags |= CA_ASSET_REQUESTED; }
	ILINE void ClearAssetRequested() {m_nFlags &= ~CA_ASSET_REQUESTED;}

	ILINE uint32 IsAssetOnDemand()const { return m_nFlags&CA_ASSET_ONDEMAND; }
	ILINE void OnAssetOnDemand() { m_nFlags |= CA_ASSET_ONDEMAND; }

	ILINE uint32 IsAssetNotFound()const { return m_nFlags&CA_ASSET_NOT_FOUND; }
	ILINE void OnAssetNotFound() { m_nFlags |= CA_ASSET_NOT_FOUND; }
	ILINE void ClearAssetNotFound() {m_nFlags &= ~CA_ASSET_NOT_FOUND;}

	void AddRef()
	{
		++m_nRefCount;
	}

	void Release()
	{
		if (!--m_nRefCount)
		{
			m_pAnimActionPrototype = NULL;
		}
	}

public:
	_smart_ptr< struct IAnimActionClass > m_pAnimActionPrototype;
	int m_nRefCount;
	uint32 m_nFlags;

} _ALIGN(128);

class CAnimActionManager :
	public IAnimActionManager
{
public:
	CAnimActionManager();

private:
	friend class CAnimActionActor;
	void LoadAnimActionPropertiesFromXML( XmlNodeRef node, const SAnimActionPropertyDesc* descriptor, TAnimActionPropertyValues& properties );
	bool RegisterAAClass(IAnimActionClass* pClass);
	size_t GetAAClassCount() const;
	IAnimActionClass* GetAAClass(size_t nIndex);
	IAnimActionClass* GetAAClassByName(const char* name);
	int32 GetAAClassIndexByName(const char* name);
	IAnimActionClass* CreatePrototypeFromXml( const char *sActionName,XmlNodeRef &node );
//	DynArray<GlobalHeaderAAC>& GetGlobalArray() { return m_arrGlobalAAC;	}
	int CreateGlobalMotionHeaderAAC(const char* strFilePath);
	int GetGlobalIDbyFilePathAAC(const char * sAnimFileName);
	void CreateAAPrototype( const char* szFilePath, const char* szAnimName,uint32 nGlobalAnimID);
	uint32 GetFlagsAAC(uint32 nGlobalAnimID);

	std::vector< _smart_ptr<IAnimActionClass> >	m_AAClasses;
	typedef std::map<const char*, size_t, stl::less_strcmp<const char*> > TClassNameToIndexMap;
	TClassNameToIndexMap	m_classToIndex;
	CNameCRCMap		m_AnimationMap;
	DynArray<GlobalHeaderAAC> m_arrGlobalAAC;
};

#endif //__ANIMACTIONMANAGER_H__
