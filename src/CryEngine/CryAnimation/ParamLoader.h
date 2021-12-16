
/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2010.
-------------------------------------------------------------------------
Description: 

The .chrparams file format is a XML file which stores all the information
that was stored in the .ik, .setup and .cal files before in the sections
<lod>, <IK_Definition> and <AnimationList>
The loading code is partly copy-paste from the old LoaderCHR and CalParser
-------------------------------------------------------------------------
History:
- 08:06:2010   : Created by Benjamin Block

*************************************************************************/

#ifndef PARAMLOADER_H
#define PARAMLOADER_H


struct CAF_ID
{
	int32 m_nCafID;  
	int32 m_nType;  //regular CAF-file or AIM-Pose (which is also a CAF file) 
	CAF_ID( int32 id, int32 type) 
	{
		m_nCafID=id;  
		m_nType=type;
	};
};


// information about an animation to load
struct SAnimFile
{
	char m_FilePathQ[256];
	char m_AnimNameQ[256];
	uint32 m_crcAnim;

	DynArray< string > m_arrAimPoses;
	SAnimFile(const stack_string &szFilePath, const stack_string &szAnimName)
	{
		stack_string tmp = szFilePath;
		CryStringUtils::UnifyFilePath(tmp);
		tmp.MakeLower();

		memset(m_FilePathQ, 0, sizeof(m_FilePathQ));
		uint32 len1=tmp.length();
		for (uint32 i=0; i<len1; i++)
			m_FilePathQ[i] = tmp[i];

		memset(m_AnimNameQ, 0, sizeof(m_AnimNameQ));
		uint32 len2=szAnimName.length();
		for (uint32 i=0; i<len2; i++)
			m_AnimNameQ[i] = szAnimName[i];

		m_crcAnim = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(szAnimName);

	}
};

class CryCHRLoader;

// holds information about a loaded animation list 
// (the <AnimationList> section of a .chrparams file)
// this looks more scary than it is (memory wise)
// for a total level its about 150 kb in memory
struct SAnimListInfo
{
	string fileName;
	DynArray<int> dependencies;

	string faceLibFile;
	string faceLibDir;
	string animEventDatabase;
	DynArray<string> modelTracksDatabases;
	DynArray<string> lockedDatabases;
	DynArray<SAnimFile> arrAnimFiles;
	DynArray<SAnimFile> arrWildcardAnimFiles;
	CAnimationSet::FacialAnimationSet::container_type facialAnimations;

	// has the animation list been parsed and the animations been loaded? 
	// then headers are available and just have to be loaded into an 
	// AnimationSet
	bool headersLoaded;

	// this is the only thing that really takes up memory after loading
	DynArray<ModelAnimationHeader> arrLoadedAnimFiles;

	// Standup animation info
	typedef VectorMap<uint32, string> StandupAnimsDataMap;
	StandupAnimsDataMap standupAnimsData;

	SAnimListInfo(const char* paramFileName)
	{
		fileName.append(paramFileName);
		// reserve a large block to prevent fragmentation
		// it gets deleted once the headers are loaded anyway
		arrAnimFiles.reserve(512);
		arrWildcardAnimFiles.reserve(512);
		arrLoadedAnimFiles.reserve(0);
		standupAnimsData.reserve(12);
		headersLoaded = false;
	}
	void HeadersLoaded()
	{
		arrAnimFiles.clear();
		arrLoadedAnimFiles.reserve(arrLoadedAnimFiles.size());
		headersLoaded = true;
	}
	void PrepareHeaderList()
	{
		arrWildcardAnimFiles.clear();
		arrLoadedAnimFiles.reserve(arrAnimFiles.size());
	}
};

class CParamLoader
{
public:
	CParamLoader();	
	~CParamLoader(void);

	void ClearLists();

	bool LoadXML(CCharacterModel *pModel, string defaultAnimDir, const char* const paramFileName, uint32 isSkinAttachment, DynArray<uint32>& listIDs);
	bool ExpandWildcards(uint32 listID , const bool parseSubFolders);

	const SAnimListInfo& GetParsedList(const uint32 i) { return m_parsedLists[i];};
	uint32 GetParsedListNumber() { return m_parsedLists.size(); };

	void PrepareHeaderList(uint32 listID) { m_parsedLists[listID].PrepareHeaderList(); };
	void SetHeadersLoaded(uint32 listID) { m_parsedLists[listID].HeadersLoaded(); };
	void AddLoadedHeader(uint32 listID, const ModelAnimationHeader& modelAnimationHeader) { m_parsedLists[listID].arrLoadedAnimFiles.push_back(modelAnimationHeader); }
	void AddStandupAnimData(uint32 listID, uint32 animNameCRC32, const char* szStandupAnimType) { assert((int)listID < m_parsedLists.size()); m_parsedLists[listID].standupAnimsData[animNameCRC32] = szStandupAnimType; }

private:
	int LoadAnimList(const XmlNodeRef calNode, const char* paramFileName, string strAnimDirName, bool& parseSubFolders);
	bool BuildDependencyList(int rootListID, DynArray<uint32>&listIDs);
	CAF_ID MemFindFirst( char* strAnimName, const char * szFolder, uint32 crcFolder, int folderLenght, CAF_ID nCafID );

	bool ParseIKDef(const XmlNodeRef ikNode);
	bool LoadIKTargetSetup(const XmlNodeRef limbNode);
	bool LoadIKDefLimbIK(const XmlNodeRef limbNode);
	bool LoadIKDefAnimDrivenIKTargets(const XmlNodeRef adNode);
	bool LoadIKDefFeetLock(const XmlNodeRef limbNode);
	bool LoadIKDefRecoil(const XmlNodeRef aimNode);
	bool LoadIKDefLookIK(const XmlNodeRef aimNode);
	bool LoadIKDefAimIK(const XmlNodeRef aimNode);

	bool LoadLod(const XmlNodeRef lodNode);

	int ListProcessed(const char* paramFileName);

	// helper functions for interfacing SAnimListInfo
	bool AddIfNewAnimationAlias( SAnimListInfo& animList, const char* animName, const char* szFileName);
	bool NoAnimationAliasInDependencies(SAnimListInfo& animList, const char* szFileName);

	bool AddIfNewFacialAnimationAlias( SAnimListInfo& animList,  const char* animName, const char* szFileName);
	bool NoFacialAnimationAliasInDependencies(SAnimListInfo& animList, const char* animName);
	bool AddIfNewModelTracksDatabase(SAnimListInfo& animList, const char* dataBase );
	bool NoModelTracksDatabaseInDependencies(SAnimListInfo&animList, const char* dataBase);

	DynArray<SAnimListInfo> m_parsedLists;
	CCharacterModel* m_pModel;
	string m_defaultAnimDir;
};

#endif