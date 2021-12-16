//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#ifndef __RFXUTIL__H
#define __RFXUTIL__H

#include "resource.h"
#include "NameList.h"
#include "VNormal.h"

#include "shtypes.h"

#include "../xml/xml.h"
#include "../CryShader/MaterialSender.h"
#include "IExportSourceInfo.h"

#include "iFnPub.h"
#include "IProgressMonitor.h"
#include "BoneKey.h"

class ISourceMaterial;
class ISourceObject;
class ISourceMesh;
class ISkinningInfo;
class IVertexAnimation;
class ISceneProperties;
class IExportFlags;
class IController;
class ISkeleton;
class IBreakablePhysicsInfo;
class ErrorReporter;
class MaxExportSource;
class IMorphChannel;
class IMorphData;
class IHelperObject;
class AssetWriter;


//--------------------------------------------------------------------------------------
// Function publishing:

#define CRYEXPORTER_INTERFACE Interface_ID(0x763b6ba1, 0x43110477)
#define GetICryExporterInterface(cd) \
	(ICryExporter*)(cd)->GetInterface(CRYEXPORTER_INTERFACE)

enum {
	FN_EXPORT_NODES,
	FN_CAN_EXPORT_ANIMS,
	FN_EXPORT_ANIMS,
	FN_EXPORT_ANIM,
	FN_REGISTER_EXPORT_CALLBACK,
	FN_SET_NODE_LIST,
	FN_GET_NODE_LIST,
	FN_SET_BONE_LIST,
	FN_GET_BONE_LIST,
	FN_GET_ROOT_PATH,
	FN_GET_VALUE,
	FN_SET_VALUE,
	FN_EXECUTE_COMMAND_LINE,
	FN_COPY_RANGES_TO_TIME_TAGS,
	FN_GET_CUSTOM_FILENAME,
	FN_SET_CUSTOM_FILENAME,
	FN_GET_USE_CUSTOM_FILENAME,
	FN_SET_USE_CUSTOM_FILENAME
};

class CryExporterOps : public FPStaticInterface
{
public:
	virtual void ExportNodes() = 0;
	virtual bool CanExportAnims() = 0;
	virtual void ExportAnims() = 0;
	virtual void ExportAnim(const char* szFileName) = 0;
	virtual void RegisterExportCallback(Value* value) = 0;
	virtual Tab<INode*> GetNodeList() = 0;
	virtual void SetNodeList(Tab<INode*> nodeList) = 0;
	virtual Tab<INode*> GetBoneList() = 0;
	virtual const char* GetRootPath() = 0;
	virtual const char* GetValue(const char* szKey) = 0;
	virtual void SetValue(const char* szKey, const char* szValue) = 0;
	virtual void SetBoneList(Tab<INode*> nodeList) = 0;
	virtual void ExecuteCommandLine(const char* szCommandLine, bool wait) = 0;
	virtual void CopyRangesToTimeTags() = 0;
};

// IMPORTANT:
// The ClassID must be changed whenever a new project
// is created using this skeleton
#define	SKUTIL_CLASSID		Class_ID(0x13fb54aa, 0x13051305)

TCHAR *GetString(int id);
extern ClassDesc* GetCSExportUtilDesc();
StdUVGen *GetTextureStdUVGen(Texmap* tex);


// Paramblock2 name
enum 
{ 
	pbid_Nodes, 
	pbid_Bones, 
	pbid_Options,
	pbid_Timing,
	pbid_Material,
	pbid_Physics,
	pbid_Batch,

	n_pblocks
}; 

// Paramblock2 parameter list
enum 
{ 
	PB_NODES,
	PB_ADD_RELATED,
	PB_ADD_WHOLE_SKEL,
	PB_EXPORT_MAT,
	PB_ALLOW_MULTIUV,
	PB_WRITE_WEIGHTS,
	PB_GENERATE_DEFAULT_UVS,
	PB_WRITE_VCOL,
	PB_LINK_TYPE,
	PB_WRITE_VA,
	PB_VERT_ANIM_RATE,
	PB_BUILDING,
	PB_INDIVIDUAL_FILES,
	PB_EXPORT_GEOM,
	PB_MORPH_MIN_OFFSET,
	PB_EXPORT_TO,
	PB_MERGE_OBJECTS,
	PB_USE_CUSTOM_FILENAME,
	PB_CUSTOM_FILENAME,
	PB_CUSTOM_FILENAME_HISTORY
};

enum 
{ 
	PB_BONES,
	PB_IGNORE_DUMMY,
	PB_SORT_BONES
};

enum 
{ 
	PB_AUTO_SAVE,
	PB_WARN_NONRIGID,
	PB_WARN_ORPHAN_TV,
	PB_WARN_DLL,
	PB_WARN_MULTI_VCOL,
	PB_WARN_OFFAXIS_SCL,
};

enum
{
	PB_MAN_RANGE,
	PB_RANGE_START,
	PB_RANGE_END,
	PB_EXPORT_FILE_PER_SUBRANGE
};

enum
{
	PB_BREAKABLE_PHYSICS,
	PB_GRANULARITY,
	/*
	PB_PH_MAX_FORCE_PUSH,
	PB_PH_MAX_FORCE_PULL,
	PB_PH_MAX_FORCE_SHIFT,
	PB_PH_MAX_TORQUE_TWIST,
	PB_PH_MAX_TORQUE_BEND,
	PB_PH_CRACK_WEAKEN,
	*/
};

/*===========================================================================*\
 |	CSExportUtilClassDesc class defn
\*===========================================================================*/
class CSExportUtilClassDesc:public ClassDesc2 
{
	public:
	int 			IsPublic()					{ return TRUE; }
	void *			Create( BOOL loading );
	const TCHAR *	ClassName()					{ return GetString(IDS_CLASSNAME); }
	SClass_ID		SuperClassID()				{ return UTILITY_CLASS_ID; }
	Class_ID 		ClassID()					{ return SKUTIL_CLASSID; }
	const TCHAR* 	Category()					{ return GetString(IDS_CATEGORY);  }
	BOOL			NeedsToSave()				{ return TRUE; }
	IOResult		Save(ISave *s);
	IOResult		Load(ILoad *l);
	//void			ResetClassParams (BOOL fileReset);

	// Hardwired name, used by MAX Script as unique identifier
	const TCHAR*	InternalName()				{ return _T("CSExport"); }
	HINSTANCE		HInstance()					{ return hInstance; }

};
/*
struct MeshInfo
{
	Tab<Face>		Faces;
	Tab<Point3>	Verts;
	Tab<Point3>	Normals;
	Tab<Point3>	VColors;

	Tab<TVFace> TexFaces;
	Tab<UVVert> TexVerts;
};
*/

class NamedRange
{
public:
	std::string sName;
	int nStart;
	int nEnd;
};

/*===========================================================================*\
 |	CSExportUtility class defn
\*===========================================================================*/
class CSExportUtility : public UtilityObj, public ReferenceMaker, public IProgressMonitor
{
	public:
		IUtil*			iu;
		Interface*		ip;

		HIMAGELIST		ButtonImages;

		IParamBlock2*	pb_nodes;
		IParamBlock2*	pb_bones;
		IParamBlock2*	pb_options;
		IParamBlock2*	pb_timing;
		IParamBlock2*	pb_material;
		IParamBlock2*	pb_physics;
		IParamBlock2*	pb_batch;
		IParamMap2*		pmap_nodes;
		IParamMap2*		pmap_bones;
		HWND			hwnd_nodes;
		HWND			hwnd_bones;
		HWND			hwnd_about;

		NameList			BoneList;
		CHUNK_LIST			ChunkList;
		std::vector<NamedRange>	Ranges;

		std::vector<SBoneLightBind> m_arrLightBinding;

		HWND hProgressWindow;
		HWND hProgressBar;
		TSTR lastSavedFile;
		bool m_bLastSavedIndividualFiles;
		TSTR m_strLastChosenSaveFile;
		std::vector<std::string> m_skippedMorphTargetNames;

		std::string m_sEditorExe;
		std::string m_sRcPath;
		bool bExportNew;
		bool bBuilding;
		bool bCanMergeNodes;

		std::string m_sTestSuitePath;

		LPITEMIDLIST m_pidlLast;
		
	
		//Constructor/Destructor
		CSExportUtility();
		~CSExportUtility();

		void			BeginEditParams		(Interface *ip,IUtil *iu);
		void			EndEditParams		(Interface *ip,IUtil *iu);
		void			DeleteThis			()			{}

		int				NumRefs				()			{ return n_pblocks; }
		RefTargetHandle GetReference		(int i);
		void			SetReference		(int i, RefTargetHandle rtarg);
		RefTargetHandle Clone				(RemapDir& remap = MAX_DEFAULT_REMAP());
		RefResult		NotifyRefChanged	(Interval changeInt,RefTargetHandle hTarget, PartID& partID, RefMessage message);

		int				NumParamBlocks		()			{ return n_pblocks; }			// return number of ParamBlocks in this instance
		IParamBlock2*	GetParamBlock		(int i);
		IParamBlock2*	GetParamBlockByID	(short id);
		void VariantToString(const PROPVARIANT* pProp, TCHAR* szString, int bufSize);

		//Common
		int				SaveFileDlg			(int FileType, char *filename=NULL, const char *title=NULL, const char *def_name=NULL);
		int				SaveDirDlg			(int FileType, char *filename=NULL, char *title=NULL, char *def_name=NULL);
		static int CALLBACK SaveDirDlg_BrowseCallbackProc
		(
			HWND hwnd, 
			UINT uMsg, 
			LPARAM lParam, 
			LPARAM lpData
		)
		{
			return ((CSExportUtility*)lpData)->SaveDirDlgCallback(hwnd, uMsg, lParam);
		}
		int SaveDirDlgCallback (HWND hwnd, UINT uMsg, LPARAM lParam);

		bool			WriteChunkList		(FILE *f, FILE_HEADER &header);
		void			CallExtensionDLLs	(const char *fname, int Flags);
		void			ConfigLoad			(bool autosave);
		void			CleanUpIntTab		(IntTab &tab);
		void			EnableDisableControls();
		void			UpdateProgress		(int percent, char *c, ...);
		bool			SaveController		(FILE* f, IController* pController);
		bool			SaveTiming			(FILE* f, const Interval* pRange = 0);

		void InitConfig()
		{
			ip = GetCOREInterface();
			ConfigLoad(true);
			LoadConfigFromMaxFile();
		}

		void SaveConfigToMaxFile();
		void LoadConfigFromMaxFile();

		//Geom Export
		int				CheckNodeProblems	(ISourceObject* pSourceObject);
		bool      IsChildOf(ISourceObject* pChild,ISourceObject *pParent);
		void			RecursePrepareMtlChunks(ISourceMaterial* pMaterial);
		
		// generates a default material chunk if needed, and returns its index (in the numbering of standard materials)
		int m_nDefaultMtlChunkId;

		INode*			DerObj2Node			(Object *obj);
		void SaveSourceInfo (IExportSourceInfo* pSourceInfo, FILE*f, int ChunkId, std::time_t exportTime);
		enum {
			// all are by default no
			nSMF_DontSaveBoneInfo = 1<< 0,
			nSMF_IsBoneMeshChunk = 1<< 1,	 // should be set if the bone chunk is being saved
			nSMF_SaveMorphTargets = 1<< 2, // should be set if the morph target chunks are also to be saved 
			nSMF_SaveInitBonePos = 1<< 3,  // if set, a chunk with initial bone positions is added
			nSMF_ForceSaveVertexColours = 1 << 4 // Write vertex alpha even if none present (ie write all 0xFF)
		};
		bool			SaveMeshObject		(FILE* f, AssetWriter& writer, IExportFlags* pExportFlags, ISkeleton* pSkeleton, ISourceMesh* pMesh, ISourceMaterial* pMaterial, const Matrix34& transform, const Matrix34& objectOffset, const std::string& sName, int nChunkID, int nVertexAnimChunkID, unsigned nFlags = 0);
		bool			SaveVertAnim		(FILE* f, INode *node, IVertexAnimation* pVertexAnimation);
		bool			SavePatchObject		(FILE* f, Object* der_obj);
		bool			SaveHelperObject	(FILE* f, IHelperObject* pHelperObject, INode* der_obj);
		bool			SaveNode(FILE* f, ISourceObject* pSourceObject);
		bool			SaveSceneProps		(FILE* f);
		bool			SaveSceneProps(FILE* f, ISceneProperties* pSceneProperties);
		bool			SaveExportFlagsChunk(FILE* f, IExportFlags* pExportFlags);
		bool			SaveBreakablePhysics(FILE* f, IBreakablePhysicsInfo* pBreakablePhysicsInfo);
		bool			SaveBoneNameList	(FILE* f, ISkeleton* pSkeleton);
		// saves the light-bone binding info
		bool			SaveMaterialName		(FILE* f, ISourceMaterial* pMaterial, SChunkInfo *pChunkInfo);

		int SaveColladaGeomFile();
		int SaveGeomFile(std::time_t exportTime = -1, bool bRunResourceCompiler = true);
		bool PreviewGeomFile();
		bool PreviewGame( TSTR fileName );
		
		//Bone Export
		void			AddRelatedBones		();
		void			RemoveChildBones	(INode *parent);
		bool			IsFootPrint			(INode *bone);
		bool			IsLegBone			(INode *bone);
		bool			IsBipedBone			(INode *bone);
		bool			IsBipedRoot			(INode *bone);
		INode*			FindRootBone		(INode *node);
		INode*			FindLThigh			(INode *root, bool forward);
		INode*			FindRThigh			(INode *root, bool forward);

		bool			SaveBoneController (const Interval& range, FILE *f, INode* node);
		bool			SaveBoneHierarchy	(FILE *f, ISkeleton* pSkeleton, INodeTab &allnodes);
		
		// Given the list of key times, generate linear BoneKey's for all those times.
		void GetBoneNodeKeys (INode *pNode, const IntTab &tabKeyTimes, Tab<BoneKey> &tabOutKeys);
		
		bool			GetValidChildrenList(INode *node, INodeTab &children, bool recurse);
		bool CanExportBones();
		int SaveBoneAnimFile(const std::string& sFileName);

		TSTR			GetRelativePath( const TSTR &path );

		int				GetTime();

		void AddBoneToList( INode *bone );

		std::string FindDefaultExportFilename();
		std::string FindCustomExportFilename();
		std::string FindDefaultCustomExportParameter();
		std::string FindExportExtension();
		int FindExportFilenameTypesString();

		std::string FindDefaultBoneAnimExportFilename();
		class BoneAnimExportParameters;
		int ExportBoneAnimToFile(const BoneAnimExportParameters& exportParams);

		void SelectCustomExportFilename();

		void RunTestSuite();

		bool bClosingFile;

		void SetPreExportCallback(Value* callback);
		bool RunPreExportCallback();

		void CopyRangesToTimeTags();

		const char* GetCustomFilename();
		void SetCustomFilename(const char *filename);
		bool GetUseCustomFilename();
		void SetUseCustomFilename(bool useCustom);

private:
	static void HandleTestSuiteError(const std::string& sMessage, CSExportUtility* pUtility);

protected:
	bool AddSkinRelatedBones(Modifier *skin, INode *node);
	bool IsHaveBoneInfo( INode* node );

	void PrepareAllBones( INodeTab &allBones );

	bool SaveMorphTargets (FILE* f, ISourceMesh* pMesh, const Matrix34& tm, int nVertexMapSize, int* pVertexMap, int nChunkIdMesh, const Vec3& morphOffset);

	// arrVertexMap maps compacted vertices -> original Max vertices (through the split vertices)
	// returns error bit
	bool SaveMorphTarget (FILE* f, const Matrix34& tm, IMorphData* pMorphData, IMorphChannel* pMorphChannel, int nVertexMapSize, int* pVertexMap, int nChunkIdMesh, const Vec3& morphOffset);

	MaxExportSource* CreateExportDataSource(ErrorReporter* pErrorReporter, std::vector<INode*>& nodes);

	bool RunResourceCompiler( const char *sExportFilename, bool bUseQuota = true, DWORD dwWindow = 0);

	Value* preExportCallback;
};

extern CSExportUtility theCSExportUtility;


/*===========================================================================*\
 |	Dialog Processors
\*===========================================================================*/
class OptionsDlgProc : public ParamMap2UserDlgProc 
{
	public:
	CSExportUtility *tsu;
	
	OptionsDlgProc(CSExportUtility *tsu_in) { tsu = tsu_in; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() {delete this; }
};

class TimingDlgProc : public ParamMap2UserDlgProc 
{
	public:
	CSExportUtility *tsu;
	
	TimingDlgProc(CSExportUtility *tsu_in) { tsu = tsu_in; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() {delete this; }
};


class MaterialDlgProc : public ParamMap2UserDlgProc 
{
public:
	CSExportUtility *tsu;

	MaterialDlgProc(CSExportUtility *tsu_in): m_MatSender(false) { tsu = tsu_in; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() {delete this; }

	HWND CheckAndRunApplication(const char * pWndName, const char * pFlag);

	void XmlNode2Material(XmlNodeRef & data, Mtl * pMtl);
	void Material2XmlNode(Mtl * pMtl, XmlNodeRef & node );

private:
	CMaterialSender m_MatSender;
	std::string m_sEditorExe;
	std::string m_sRcPath;
};


class BatchDlgProc : public ParamMap2UserDlgProc 
{
public:
	CSExportUtility *tsu;

	BatchDlgProc(CSExportUtility *tsu_in) { tsu = tsu_in; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() {delete this; }

	void ExportAllFilesInFolder(const char * folder);
};



class PhysicsDlgProc : public ParamMap2UserDlgProc 
{
public:
	CSExportUtility *tsu;

	PhysicsDlgProc(CSExportUtility *tsu_in){ tsu = tsu_in; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() {delete this; }
};


class NodesDlgProc : public ParamMap2UserDlgProc 
{
	public:
		CSExportUtility *tsu;

		NodesDlgProc() {}
		NodesDlgProc(CSExportUtility *tsu_in) { tsu = tsu_in; }

		INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void DeleteThis() {delete this;  }

		void SetThing(ReferenceTarget *m) 
		{
			tsu = (CSExportUtility*)m;
		}
};

class NodesExportToAccessor : public PBAccessor
{
	public:
		NodesExportToAccessor(CSExportUtility* pUtility): m_pUtility(pUtility) {}
		virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t);

	private:
		NodesExportToAccessor();

		CSExportUtility* m_pUtility;
};

class NodesCustomFilenameAccessor : public PBAccessor
{
public:
	NodesCustomFilenameAccessor(CSExportUtility* pUtility): m_pUtility(pUtility) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t);

private:
	NodesCustomFilenameAccessor();

	CSExportUtility* m_pUtility;
};

class NodesCustomFilenameValidator : public PBValidator
{
public:
	NodesCustomFilenameValidator(CSExportUtility* pUtility): m_pUtility(pUtility) {}
	virtual BOOL Validate(PB2Value& v);

private:
	NodesCustomFilenameValidator();

	CSExportUtility* m_pUtility;
};

class BonesDlgProc : public ParamMap2UserDlgProc 
{
	public:
		CSExportUtility *tsu;

		BonesDlgProc() {}
		BonesDlgProc(CSExportUtility *tsu_in) { tsu = tsu_in; }

		INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void DeleteThis() {delete this;  }

		void SetThing(ReferenceTarget *m) 
		{
			tsu = (CSExportUtility*)m;
		}
};

void ExportColladaButton(CSExportUtility *tsu);
void ExportButton(CSExportUtility *tsu, std::time_t exportTime = 0, bool bRunResourceCompiler = true);

#endif
