//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#include "stdafx.h"

#include <imtledit.h>

#include "Utility.h"
#include "interpik.h"
#include <iskin.h>
#include <iiksys.h>
#include <notify.h>
#include <Shlobj.h>
#include <set>

#include "Crc32.h"

#include "ModifierUtils.h"

#include "..\CryShader\CryShader.h"
#include "AdjustRotLog.h"

#include "PathUtil.h"


#include "MAXScrpt/MAXScrpt.h"
#include "MAXScrpt/3DMath.h"
#include "MAXScrpt/Numbers.h"
#include "MAXScrpt/Strings.h"
#include "MAXScrpt/Name.h"
#include "FaceReportDlg.h"
#include <set>

#include <sstream>
#include <iomanip>

#include "TestSuite.h"

#include "MAXScrpt/definsfn.h"

#include "NodeUtils.h"
#include "MeshUtils.h"

#include "IBone.h"
#include "IBoneArray.h"
#include "ISkeleton.h"
#include "ISourceMaterial.h"
#include "ISourceMaterialArray.h"
#include "ISourceObjectArray.h"
#include "ISourceObject.h"
#include "IErrorReporter.h"
#include "ISourceMesh.h"
#include "ISkinningInfo.h"
#include "MaxExportSource.h"
#include "MaxObject.h"
#include "MaxSkeleton.h"
#include "MaxVertexAnimation.h"
#include "ISceneProperties.h"
#include "MaxMaterial.h"
#include "MaxController.h"
#include "IControllerKeyHandler.h"
#include "IPhysicsNode.h"
#include "IJointParameters.h"
#include "IPhysicsFrame.h"
#include "IBreakablePhysicsInfo.h"
#include "IExportFlags.h"
#include "MaxHelperObject.h"
#include "AssetWriter.h"
#include "XRef/iXrefObj.h"
#include "FolderBrowser.h"
#include "PropertyHelpers.h"
#include "ResourceCompilerHelper.h"

#include "Export/CBAHelpers.h"
#include "PakSystem.h"

#include "../ExportStub/ExportStub.h"

#include <IFrameTagManager.h> 

def_visible_primitive( CryExp, "CryExp" );

#define CGF_EXTENSION "cgf"
#define CHR_EXTENSION "chr"
#define CGA_EXTENSION "cga"
#define ANM_EXTENSION "anm"
#define CAF_EXTENSION "caf"

enum EExportTo
{
	EXPORT_TO_CGF,
	EXPORT_TO_CHR,
	EXPORT_TO_CGA,
	EXPORT_TO_ANM,
};

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

extern char* BuildNo;	
typedef int (* CryCallbackFunc)(HWND, const char *);
static Class_ID BipedClassID(0x9125,0x0);
static Class_ID stdUVClassID(STDUV_CLASS_ID,0);

const int ID_SKIP_NODE = 0x1001;

class ErrorReporter : public IErrorReporter
{
public:
	ErrorReporter(CSExportUtility* pUtility);
	virtual void Report(ErrorLevel level, const std::string& sMessage);

private:
	CSExportUtility* pUtility;
};

//from uvgen.cpp
//==============
const int PB_UOFFS		= 0;
const int PB_VOFFS		= 1;
const int PB_USCL		= 2;
const int PB_VSCL		= 3;
const int PB_UANGLE		= 4;
const int PB_VANGLE		= 5;
const int PB_WANGLE		= 6;
const int PB_BLUR		= 7;
const int PB_NSAMT		= 8;
const int PB_NSSIZ		= 9;
const int PB_NSLEV		= 10;
const int PB_NSPHS		= 11;
const int PB_BLUROFFS	= 12;

StdUVGen *GetTextureStdUVGen(Texmap* tex)
{
	if(!tex) return NULL;

	//get UVGen
	UVGen		*uvgen	= tex->GetTheUVGen();
	if(!uvgen)							return NULL;
	if(uvgen->ClassID()!=stdUVClassID)	return NULL;
	
	//Get UV pblock
	return (StdUVGen *)uvgen;
};

char s[2048];	//general purpose string buffer
bool bPreviewMode = false;
bool bDontShowMessageBoxes = false;

//flags for DLL Extension Calls
#define DLL_FLAG_GEOM 1
#define DLL_FLAG_ANIM 2

class CSExportUtility::BoneAnimExportParameters
{
public:
	std::string sFilename;
	std::vector<INode*> bones;
	Interval range;
};

int CryMessageBox( HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType)
{
	if(bDontShowMessageBoxes)
		return MB_OK;
	//if (bPreviewMode)
		//return -1;
	return MessageBox( hWnd,lpText,lpCaption,uType );
}

// finds the node in the array, and returns the index of the given node within the array.
// returns -1 if the node was not found
int FindNode (const INodeTab& tabNodes, INode* pNode)
{
	for (int i = 0; i < tabNodes.Count(); ++i)
	{
		if (tabNodes[i] == pNode)
			return i;
	}
	return -1;
}

//====================================================================
//====================================================================
//====================================================================
//! Doxygen text by cuneyt
void Matrix3_to_CryMatrix( float cm[4][4], Matrix3 &m3)
{
	MRow *arow=m3.GetAddr();
/*
	cm.m00 = arow[0][0];
	cm.m01 = arow[0][1];
	cm.m02 = arow[0][2];
	cm.m03 = 0.0f;
	cm.m10 = arow[1][0];
	cm.m11 = arow[1][1];
	cm.m12 = arow[1][2];
	cm.m13 = 0.0f;
	cm.m20 = arow[2][0];
	cm.m21 = arow[2][1];
	cm.m22 = arow[2][2];
	cm.m23 = 0.0f;
	cm.m30 = arow[3][0];
	cm.m31 = arow[3][1];
	cm.m32 = arow[3][2];
	cm.m33 = 1.0f;
*/
	cm[0][0] = arow[0][0];
	cm[0][1] = arow[0][1];
	cm[0][2] = arow[0][2];
	cm[0][3] = 0.0f;
	cm[1][0] = arow[1][0];
	cm[1][1] = arow[1][1];
	cm[1][2] = arow[1][2];
	cm[1][3] = 0.0f;
	cm[2][0] = arow[2][0];
	cm[2][1] = arow[2][1];
	cm[2][2] = arow[2][2];
	cm[2][3] = 0.0f;
	cm[3][0] = arow[3][0];
	cm[3][1] = arow[3][1];
	cm[3][2] = arow[3][2];
	cm[3][3] = 1.0f;
};

void CryMatrix_to_Matrix3(Matrix3 &m3, float cm[4][4] )
{
	MRow *arow=m3.GetAddr();
	/*
	arow[0][0]=cm.m00; 
	arow[0][1]=cm.m01; 
	arow[0][2]=cm.m02; 
	arow[1][0]=cm.m10; 
	arow[1][1]=cm.m11; 
	arow[1][2]=cm.m12; 
	arow[2][0]=cm.m20; 
	arow[2][1]=cm.m21; 
	arow[2][2]=cm.m22; 
	arow[3][0]=cm.m30; 
	arow[3][1]=cm.m31;
	arow[3][2]=cm.m32;
	*/

	arow[0][0]=cm[0][0]; 
	arow[0][1]=cm[0][1]; 
	arow[0][2]=cm[0][2]; 
	arow[1][0]=cm[1][0]; 
	arow[1][1]=cm[1][1]; 
	arow[1][2]=cm[1][2]; 
	arow[2][0]=cm[2][0]; 
	arow[2][1]=cm[2][1]; 
	arow[2][2]=cm[2][2]; 
	arow[3][0]=cm[3][0]; 
	arow[3][1]=cm[3][1]; 
	arow[3][2]=cm[3][2]; 
};
/*===========================================================================*\
 |	Class Descriptor
\*===========================================================================*/
static	CSExportUtilClassDesc	CSExportUtilCD;
CSExportUtility			theCSExportUtility;

ClassDesc*	GetCSExportUtilDesc()							{return &CSExportUtilCD;}
void *		CSExportUtilClassDesc::Create( BOOL loading )	{return &theCSExportUtility;}

CResourceCompilerHelper* g_rch = NULL;

CResourceCompilerHelper* GetResourceCompilerHelper()
{
	std::string moduleName = "CryExport";

	if (g_rch==NULL)
		g_rch = new CResourceCompilerHelper(moduleName.c_str());
	return g_rch;
}

class CryExporterOpsImpl : public CryExporterOps
{
public:

	DECLARE_DESCRIPTOR(CryExporterOpsImpl)

	//Function Map For Interface
	BEGIN_FUNCTION_MAP
		VFN_0(FN_EXPORT_NODES, ExportNodes)
		FN_0(FN_CAN_EXPORT_ANIMS, TYPE_BOOL, CanExportAnims)
		VFN_0(FN_EXPORT_ANIMS, ExportAnims)
		VFN_1(FN_EXPORT_ANIM, ExportAnim, TYPE_STRING)
		VFN_1(FN_REGISTER_EXPORT_CALLBACK, RegisterExportCallback, TYPE_VALUE)
		VFN_1(FN_SET_NODE_LIST, SetNodeList, TYPE_INODE_TAB_BV)
		FN_0(FN_GET_NODE_LIST, TYPE_INODE_TAB_BV, GetNodeList)
		VFN_1(FN_SET_BONE_LIST, SetBoneList, TYPE_INODE_TAB_BV)
		FN_0(FN_GET_BONE_LIST, TYPE_INODE_TAB_BV, GetBoneList)
		FN_0(FN_GET_ROOT_PATH, TYPE_STRING, GetRootPath)
		FN_1(FN_GET_VALUE, TYPE_STRING, GetValue, TYPE_STRING)
		VFN_2(FN_SET_VALUE, SetValue, TYPE_STRING, TYPE_STRING)
		VFN_2(FN_EXECUTE_COMMAND_LINE, ExecuteCommandLine, TYPE_STRING, TYPE_BOOL)
		VFN_0(FN_COPY_RANGES_TO_TIME_TAGS, CopyRangesToTimeTags)
		FN_0(FN_GET_CUSTOM_FILENAME, TYPE_STRING, GetCustomFilename)
		VFN_1(FN_SET_CUSTOM_FILENAME, SetCustomFilename, TYPE_STRING)
		FN_0(FN_GET_USE_CUSTOM_FILENAME, TYPE_BOOL, GetUseCustomFilename)
		VFN_1(FN_SET_USE_CUSTOM_FILENAME, SetUseCustomFilename, TYPE_STRING)
	END_FUNCTION_MAP

	virtual void ExportNodes();
	virtual bool CanExportAnims();
	virtual void ExportAnims();
	virtual void ExportAnim(const char* szString);
	virtual void RegisterExportCallback(Value* value);
	virtual void SetNodeList(Tab<INode*> nodeList);
	virtual Tab<INode*> GetNodeList();
	virtual void SetBoneList(Tab<INode*> nodeList);
	virtual Tab<INode*> GetBoneList();
	virtual const char* GetRootPath();
	virtual const char* GetValue(const char* szKey);
	virtual void SetValue(const char* szKey, const char* szValue);
	virtual void ExecuteCommandLine(const char* szCommandLine, bool wait);
	virtual void CopyRangesToTimeTags();
	virtual const char* GetCustomFilename();
	virtual void SetCustomFilename(const char *filename);
	virtual bool GetUseCustomFilename();
	virtual void SetUseCustomFilename(bool useCustom);
};

//--------------------------------------------------------------------------------------
// Function publishing descriptor for exporter interface.
static CryExporterOpsImpl CryExporterImpl(
	CRYEXPORTER_INTERFACE, _T("export"), -1, &CSExportUtilCD, 0,
		FN_EXPORT_NODES, _T("export_nodes"), -1, TYPE_VOID, 0, 0,
		FN_CAN_EXPORT_ANIMS, _T("can_export_anims"), -1, TYPE_BOOL, 0, 0,
		FN_EXPORT_ANIMS, _T("export_anims"), -1, TYPE_VOID, 0, 0,
		FN_EXPORT_ANIM, _T("export_anim"), -1, TYPE_VOID, 0, 1,
			_T("filename"), -1, TYPE_STRING,
		FN_REGISTER_EXPORT_CALLBACK, _T("register_export_callback"), -1, TYPE_VOID, 0 , 1,
			_T("callback"), -1, TYPE_VALUE,
		FN_SET_NODE_LIST, _T("set_node_list"), -1, TYPE_VOID, 0, 1,
			_T("node_list"), -1, TYPE_INODE_TAB_BV,
		FN_GET_NODE_LIST, _T("get_node_list"), -1, TYPE_INODE_TAB_BV, 0, 0,
		FN_SET_BONE_LIST, _T("set_bone_list"), -1, TYPE_VOID, 0, 1,
			_T("bone_list"), -1, TYPE_INODE_TAB_BV,
		FN_GET_BONE_LIST, _T("get_bone_list"), -1, TYPE_INODE_TAB_BV, 0, 0,
		FN_GET_ROOT_PATH, _T("get_root_path"), -1, TYPE_STRING, 0, 0,
		FN_GET_VALUE, _T("get_value"), -1, TYPE_STRING, 0, 1,
			_T("key"), -1, TYPE_STRING,
		FN_SET_VALUE, _T("set_value"), -1, TYPE_STRING, 0, 2,
			_T("key"), -1, TYPE_STRING,
			_T("value"), -1, TYPE_STRING,
		FN_EXECUTE_COMMAND_LINE, _T("execute_command_line"), -1, TYPE_VOID, 0, 2,
			_T("command_line"), -1, TYPE_STRING,
			_T("wait"), -1, TYPE_BOOL,
		FN_COPY_RANGES_TO_TIME_TAGS, _T("copy_ranges_to_time_tags"), -1, TYPE_VOID, 0, 0,
		FN_GET_CUSTOM_FILENAME, _T("get_custom_filename"), -1, TYPE_STRING, 0, 0,
		FN_SET_CUSTOM_FILENAME, _T("set_custom_filename"), -1, TYPE_VOID, 0, 1,
			_T("custom_filename"), -1, TYPE_STRING,
		FN_SET_USE_CUSTOM_FILENAME, _T("set_use_custom_filename"), -1, TYPE_VOID, 0, 1,
			_T("use_custom_filename"), -1, TYPE_BOOL,
		FN_GET_USE_CUSTOM_FILENAME, _T("get_use_custom_filename"), -1, TYPE_BOOL, 0, 0,

	end);

// Reset all the utility values on File/Reset
/*
void CSExportUtilClassDesc::ResetClassParams (BOOL fileReset) 
{
	DebugPrint("\n\n*************\nReset is called\n****************\n");
}
*/

// -------------------------------------------
// parameter validator - class declaration
// -------------------------------------------
class NodeValidator : public PBValidator
{
public:
	CSExportUtility *util;
	BOOL Validate(PB2Value &v)
	{
		Interval ivalid;

		//Check if it is of one of the supported types
		INode *node=(INode*)(v.r);
		ObjectState os = node->EvalWorldState(0);
		SClass_ID sid=os.obj->SuperClassID();
		Class_ID  cid=os.obj->ClassID();

		switch(sid)
		{
			case GEOMOBJECT_CLASS_ID:
				{
					//check if this is a biped object: if so reject it.	
					if(cid == BipedClassID ) return false;

					//reject light and camera targets
					if(node->IsTarget())
					{
						INode *parent=node->GetLookatNode();
						if(parent)
						{
							ObjectState pos = parent->EvalWorldState(0);
							if(pos.obj)
							{
								SClass_ID psid=pos.obj->SuperClassID();
								if((psid==LIGHT_CLASS_ID) || (psid==CAMERA_CLASS_ID)) return false;
							}
						}
					}
				}
				//fall through
			case LIGHT_CLASS_ID:
			case CAMERA_CLASS_ID:
				break;

			case HELPER_CLASS_ID:
				{
					if(	cid==Class_ID(DUMMY_CLASS_ID, 0) || 
						cid==Class_ID(POINTHELP_CLASS_ID, 0)) 
						break;
				}
				//fall through

			default:
				return false;		
		}

		//check for dublicate entry
		bool nodeexists=false;
		int nNodes=util->pb_nodes->Count(PB_NODES);
		for(int i=0;i<nNodes;i++)
		{
			INode *node = util->pb_nodes->GetINode(PB_NODES,0,i);
			if(!node) continue;
			//DebugPrint("\n%s",node->GetName());
			if(v.r == node) nodeexists=true;
		}
		
		return !nodeexists;
	}
};

class BoneValidator : public PBValidator
{
public:
	CSExportUtility *util;
	INode *LastValidatedNode;
	char LastValidatedName[1024];
	BoneValidator() {LastValidatedNode=NULL;}
	BOOL Validate(PB2Value &v)
	{
		LastValidatedNode=NULL;

		//check for dublicate entry (if one of our parents is in the list, we will not be accepted)
		bool nodeexists=false;
		int nNodes=util->pb_bones->Count(PB_BONES);
		
		INode *node=(INode *)v.r;
		if(!node) return false;

		//DebugPrint("Validating \"%s\" ...",node->GetName());
		while(node && !nodeexists)
		{
			for(int i=0;i<nNodes;i++)
			{
				INode *check = util->pb_bones->GetINode(PB_BONES,0,i);
				if(!check) continue;
				//DebugPrint("\n%s",node->GetName());
				if(node == check)
				{
					nodeexists=true;
					//DebugPrint("x\n");
					break;
				}
			}
			node=node->GetParentNode();
		}
		
		if(!nodeexists)
		{	
			LastValidatedNode=(INode *)v.r;
			if(LastValidatedNode)
			{
				strcpy(LastValidatedName,LastValidatedNode->GetName());
			}
			else LastValidatedName[0]=0;
			//DebugPrint("OK\n");
		}
		return !nodeexists;
	}
};

NodeValidator node_validator;
BoneValidator bone_validator;

static ParamBlockDesc2 nodes_param_blk ( pbid_Nodes, _T("CSNodesParams"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT | P_AUTO_UI | P_CALLSETS_ON_LOAD, 0, 
	//rollout
	IDD_NODES, IDS_OBJECTS, 0, 0, NULL, 
	// params
	PB_NODES,			_T("Nodes"),		TYPE_INODE_TAB, 0, P_AUTO_UI ,	IDS_NONAME,
		p_ui,			TYPE_NODELISTBOX,	IDC_OBJ_LIST,IDC_NODE_ADD,0,IDC_NODE_REMOVE,
		p_validator,	&node_validator,
		end,
	PB_ADD_RELATED,		_T("AddRelatedBones"),	TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_ADD_RELATED_BONES, 
		p_enable_ctrls,	1,PB_ADD_WHOLE_SKEL,
		end, 
	PB_ADD_WHOLE_SKEL,	_T("AddWholeSkeleton"),	TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_ADD_WHOLE_SKEL, 
		end, 
	PB_ALLOW_MULTIUV,	_T("AllowMultiUV"),		TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_ALLOW_MULTIUV, 
		end, 
	PB_WRITE_VCOL,		_T("WriteVertexColors"),TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_WRITE_VCOL, 
		end, 
	PB_WRITE_WEIGHTS,	_T("WriteWeights"),		TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_WRITE_WEIGHTS, 
		p_enable_ctrls,	1,PB_LINK_TYPE,
		end, 
	PB_LINK_TYPE,		_T("LinkType"),			TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_RADIO, 2, 	IDC_1LINK, IDC_NLINKS,
		end, 
	PB_GENERATE_DEFAULT_UVS, _T("GenerateDefaultUVs"), TYPE_BOOL, 0, IDS_NONAME,
		p_default,    1,
		p_ui,       TYPE_SINGLECHEKBOX, IDC_GENERATE_DEFAULT_UVS,
		end,
	PB_INDIVIDUAL_FILES, _T("ExportToIndividualFiles"), TYPE_BOOL, 0, IDS_NONAME,
		p_default, 0,
		p_ui, TYPE_SINGLECHEKBOX, IDC_INDIVIDUALFILES,
		end,
	PB_MERGE_OBJECTS, _T("MergeObjects"), TYPE_BOOL, 0, IDS_NONAME,
		p_default, 1,
		p_ui, TYPE_SINGLECHEKBOX, IDC_MERGE_NODES,
		end,
	PB_WRITE_VA,		_T("WriteVertexAnim"),	TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		0, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_WRITE_VERT_ANIM, 
		//p_enabled,		FALSE,
		p_enable_ctrls,	1, PB_VERT_ANIM_RATE,
		end,
	PB_VERT_ANIM_RATE,	_T("VertexAnimRate"),		TYPE_INT,		0,	IDS_NONAME,
		p_default,		1,
		p_range, 		1, 10000, 
		p_ui,			TYPE_SPINNER, EDITTYPE_INT, IDC_VERT_ANIM_FR_EDIT, IDC_VERT_ANIM_FR_SPIN, SPIN_AUTOSCALE,
		end,
	PB_BUILDING,		_T("Building"),	TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		0, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_BUILDING, 
		end,
	PB_EXPORT_GEOM,	_T("ExportGeometry"),	TYPE_BOOL, 		0,	IDS_NONAME,
		p_default, 		TRUE, 
		p_ui, 			TYPE_SINGLECHEKBOX, 	IDC_WRITE_GEOMETRY,
		end, 
	PB_MORPH_MIN_OFFSET,	_T("MorphMinOffset"),		TYPE_FLOAT,		0,	IDS_NONAME,
		p_default,		0.1f,
		p_range, 		0.0f, 100.0f, 
		p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MORPH_MIN_OFFSET_EDIT, IDC_MORPH_MIN_OFFSET_SPIN, SPIN_AUTOSCALE,
		end,
	PB_EXPORT_TO,	_T("ExportTo"),		TYPE_INT,		0,	IDS_NONAME,
		p_default,	0,
		p_range, 		0, 10,
		end,
	PB_USE_CUSTOM_FILENAME,	_T("UseCustomFilename"),	TYPE_BOOL,	0,	IDS_NONAME,
		p_default,		FALSE,
		p_ui,			TYPE_SINGLECHEKBOX,	IDC_USECUSTOMFILENAME,
		end,
	PB_CUSTOM_FILENAME,	_T("CustomFilename"),	TYPE_FILENAME,	0,	IDS_NONAME,
		p_ui,			TYPE_FILESAVEBUTTON,	IDC_CUSTOMFILENAME,
		p_file_types,	IDS_CGF_FILETYPE_STRING,
		end,
	end
	);

static ParamBlockDesc2 bones_param_blk ( pbid_Bones, _T("CSBonesParams"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 1, 
	//rollout
	IDD_BONES, IDS_EXPORT, 0, 0, NULL, 
	// params
	PB_BONES,			_T("Bones"),			TYPE_INODE_TAB, 0, P_AUTO_UI,	IDS_NONAME,
		p_ui,			TYPE_NODELISTBOX,		IDC_BONE_LIST,IDC_BONE_ADD,0,IDC_BONE_REMOVE,
		p_validator,	&bone_validator,
		end,
	PB_IGNORE_DUMMY,	_T("IgnoreDummyBones"),	TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_IGNORE_MESH_BONES,
		end,
	PB_SORT_BONES, _T("SortBones"), TYPE_BOOL, 0, IDS_NONAME,
		p_default,    0,
		p_ui,       TYPE_SINGLECHEKBOX, IDC_SORT_BONES,
		end,
	end
	);

static ParamBlockDesc2 timing_param_blk ( pbid_Timing, _T("CSTiming"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 3, 
	//rollout
	IDD_TIMING, IDS_TIMING, 0, 0, NULL, 
	// params
	PB_MAN_RANGE,		_T("ManualRange"),		TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		0, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_MANUAL_RANGE,
		p_enable_ctrls,	2, PB_RANGE_START, PB_RANGE_END,
		end, 
	PB_RANGE_START,		_T("RangeStart"),		TYPE_INT,		0,	IDS_NONAME,
		p_default,		0,
		p_range, 		-10000, 10000, 
		p_ui,			TYPE_SPINNER, EDITTYPE_INT, IDC_ANIM_START_EDIT, IDC_ANIM_START_SPIN, SPIN_AUTOSCALE,
		end,
	PB_RANGE_END,		_T("RangeEnd"),		TYPE_INT,			0,	IDS_NONAME,
		p_default,		100,
		p_range, 		-10000, 10000, 
		p_ui,			TYPE_SPINNER, EDITTYPE_INT, IDC_ANIM_END_EDIT, IDC_ANIM_END_SPIN, SPIN_AUTOSCALE,
		end,
	PB_EXPORT_FILE_PER_SUBRANGE,	_T("ExportFilePerSubRange"),		TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		FALSE, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_FILE_PER_SUBRANGE,
		end, 
	end
	);



static ParamBlockDesc2 material_param_blk ( pbid_Material, _T("CSMaterial"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 4, 
	//rollout
	IDD_MATERIAL, IDS_MATERIAL, 0, 0, NULL, 
	// params
	end
	);

static ParamBlockDesc2 physics_param_blk ( pbid_Physics, _T("CSPhisics"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 5, 
	//rollout
	IDD_PHYSICS, IDS_PHYSICS, 0, 0, NULL, 
	// params

	PB_BREAKABLE_PHYSICS, _T("BerakablePhysics"), TYPE_BOOL, 0, IDS_NONAME,
		p_default, 0,
		p_ui, TYPE_SINGLECHEKBOX, IDC_BREAKABLE_PHYSICS,
		end,


	PB_GRANULARITY,	_T("Granularity"),		TYPE_INT,		0,	IDS_NONAME,
		p_default,	0,
		p_range, 		0, 10,
		end,


/*
	PB_PH_MAX_FORCE_PUSH, _T("MaxForcePush"), TYPE_FLOAT, 0, IDS_NONAME,
		p_default,    0.0f,
		p_range,      0.0f, 1000.0f,
		p_ui,         TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MAX_FORCE_PUSH, IDC_MAX_FORCE_PUSH_SPIN, 0.1f,
		end,

	PB_PH_MAX_FORCE_PULL, _T("MaxForcePull"), TYPE_FLOAT, 0, IDS_NONAME,
		p_default,    0.0f,
		p_range,      0.0f, 1000.0f,
		p_ui,         TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MAX_FORCE_PULL, IDC_MAX_FORCE_PULL_SPIN, 0.1f,
		end,

	PB_PH_MAX_FORCE_SHIFT, _T("MaxForceShift"), TYPE_FLOAT, 0, IDS_NONAME,
		p_default,    0.0f,
		p_range,      0.0f, 1000.0f,
		p_ui,         TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MAX_FORCE_SHIFT, IDC_MAX_FORCE_SHIFT_SPIN, 0.1f,
		end,

	PB_PH_MAX_TORQUE_TWIST, _T("MaxTorqueTwist"), TYPE_FLOAT, 0, IDS_NONAME,
		p_default,    0.0f,
		p_range,      0.0f, 1000.0f,
		p_ui,         TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MAX_TORQUE_TWIST, IDC_MAX_TORQUE_TWIST_SPIN, 0.1f,
		end,

	PB_PH_MAX_TORQUE_BEND, _T("MaxTorqueBend"), TYPE_FLOAT, 0, IDS_NONAME,
		p_default,    0.0f,
		p_range,      0.0f, 1000.0f,
		p_ui,         TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MAX_TORQUE_BEND, IDC_MAX_TORQUE_BEND_SPIN, 0.1f,
		end,

	PB_PH_CRACK_WEAKEN, _T("CrackWeaken"), TYPE_FLOAT, 0, IDS_NONAME,
		p_default,    0.0f,
		p_range,      0.0f, 1.0f,
		p_ui,         TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CRACK_WEAKEN, IDC_CRACK_WEAKEN_SPIN, 0.01f,
		end,
*/
	end
	);

static ParamBlockDesc2 batch_param_blk ( pbid_Batch, _T("CSBatch"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 6, 
	//rollout
	IDD_BATCH, IDS_BATCH, 0, 0, NULL, 
	// params
	end
	);


static ParamBlockDesc2 options_param_blk ( pbid_Options, _T("CSOptions"),  0, &CSExportUtilCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 2, 
	//rollout
	IDD_OPTIONS, IDS_OPTIONS, 0, APPENDROLL_CLOSED, NULL, 
	// params
	//PB_AUTO_SAVE,		_T("AutoSaveConfig"),	TYPE_INT, 		0,	IDS_NONAME,
	//	p_default, 		0, 
	//	p_ui, 			TYPE_SINGLECHEKBOX,		IDC_AUTO_SAVE,
	//	end, 
	PB_WARN_NONRIGID,	_T("WarnNonRigid"),		TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_WARN_NONRGID,
		end, 
	PB_WARN_ORPHAN_TV,	_T("WarnOrphanTV"),		TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_WARN_ORPHAN_TV,
		end, 
	PB_WARN_DLL,		_T("WarnDLLErrors"),	TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_WARN_DLL,
		end, 
	PB_WARN_MULTI_VCOL,	_T("WarnMultiVColor"),	TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_WARN_MULTI_VCOL,
		end, 
	PB_WARN_OFFAXIS_SCL,_T("OffAxisScaling"),	TYPE_INT, 		0,	IDS_NONAME,
		p_default, 		1, 
		p_ui, 			TYPE_SINGLECHEKBOX,		IDC_WARN_OFFAXIS_SCL,
		end, 
	end
	);

//ProgressDlgProc  ___________________________________________________________________________________________
INT_PTR CALLBACK ProgressDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) 
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			break;

		case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_MOUSEMOVE:
			return FALSE;

		default: 
			return FALSE;
	}
	return TRUE;
}

void ResetReadOnlyFlag (const char* filename)
{
	DWORD fileflags = GetFileAttributes(filename);
	
	if ((fileflags != 0xffffffff)  && (fileflags & FILE_ATTRIBUTE_READONLY))
	{
		if (MessageBox(NULL,"This file is marked read-only. Do you still want to overwrite it?","Read Only Warning",MB_YESNO) == IDYES)
		{
			fileflags &= ~FILE_ATTRIBUTE_READONLY;
			SetFileAttributes(filename,fileflags);
		}
	}
}

void CreateDirectoryForFile (const char* szFileName)
{
	int nLen = strlen (szFileName);
	char* szTmp = new char[nLen];
	for (int i = 0; i < nLen; ++i)
	{
		if (szFileName[i] == '\\' || szFileName [i] == '/')
		{
			memcpy (szTmp, szFileName, i);
			szTmp[i] = '\0';
			if (GetFileAttributes (szTmp) == INVALID_FILE_ATTRIBUTES)
				CreateDirectory (szTmp, NULL);
		}
	}
	delete []szTmp;
}

void WarningFileNotSaved (HWND hParent, const TSTR& strFileName)
{
	MessageBox (hParent, (const char*)(strFileName + " file was not saved"), "Warning", MB_OK|MB_ICONASTERISK);
}

//OptionsDlgProc  ___________________________________________________________________________________________
INT_PTR OptionsDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			sprintf(s,"Build No: %s",BuildNo);
			SetWindowText(GetDlgItem(hWnd,IDC_BUILD_NO),s );
			break;

		case WM_COMMAND :
			switch (LOWORD(wParam)) 			// Handle Panel's controls
			{
			case IDC_SETUPRC: 
				{
					CResourceCompilerHelper* rch = GetResourceCompilerHelper();
					rch->ResourceCompilerUI(theCSExportUtility.ip->GetMAXHWnd());
				}
				break;
			}

		//case WM_COMMAND:
		//	switch (LOWORD(wParam)) 
		//	{
		//		case IDC_LOAD:
		//			tsu->ConfigLoad(false);
		//			break;

		//		case IDC_SAVE:
		//			tsu->ConfigSave(false);
		//			break;

		//		case IDC_RESET:
		//			if(IDYES==MessageBox(hWnd,"This command will set all of the parameters to the factory defaults\nAre you sure ?","Are you Sure?",MB_YESNO | MB_ICONQUESTION))
		//			{
		//				CSExportUtilCD.Reset(tsu);
		//			}
		//			break;

		//		case IDC_SAVE_DEFAULT:
		//			int old=tsu->pb_options->GetInt(PB_AUTO_SAVE);
		//			tsu->pb_options->SetValue(PB_AUTO_SAVE,0,1);
		//			tsu->ConfigSave(true);
		//			tsu->pb_options->SetValue(PB_AUTO_SAVE,0,old);
		//			break;
		//	}
		//	break;

		default: 
			return FALSE;
	}
	return TRUE;
}



INT_PTR CALLBACK NamedRangesDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//TimingDlgProc  ___________________________________________________________________________________________
INT_PTR TimingDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND dlgwnd=NULL;
	switch (msg) 
	{
	case WM_INITDIALOG:
		//SetDlgItemText( hWnd,IDC_ANIM_NAME,tsu->animationName );
		break;

 	case  WM_DESTROY:
		if(dlgwnd) DestroyWindow(dlgwnd);
		dlgwnd=NULL;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
			case IDC_EDIT_NAMED_SEG:
				if(!dlgwnd)
				{
					HWND parent = tsu->ip->GetMAXHWnd();
					//int res=DialogBoxParam(hInstance,  MAKEINTRESOURCE(IDD_NAMED_RANGES),parent, NamedRangesDlgProc, LPARAM (tsu));
					//if(res==-1) res=GetLastError();
					dlgwnd=CreateDialogParam(hInstance,  MAKEINTRESOURCE(IDD_NAMED_RANGES),parent, NamedRangesDlgProc, LPARAM (tsu));
				}
				ShowWindow(dlgwnd,SW_SHOW);
				break;
			case IDC_EXPORTNEW:
				//Timur[2/5/2002] tsu->bExportNew = IsDlgButtonChecked( hWnd,IDC_EXPORTNEW )!=0;
				break;
			case IDC_ANIM_NAME:
				/*
				if (HIWORD(wParam) == EN_CHANGE)
				{
					char animName[1024];
					GetDlgItemText( hWnd,IDC_ANIM_NAME,animName,sizeof(animName) );
					tsu->animationName = animName;
				}
				if (HIWORD(wParam) == EN_SETFOCUS)
				{
					DisableAccelerators();
				}
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					EnableAccelerators();
				}
				*/
				break;
		}
		break;

	case WM_APP + 10013:
		{
			if (dlgwnd)
			{
				extern void FillList(HWND hWnd, CSExportUtility* util, int active_id);
				CSExportUtility* pUtility = (CSExportUtility*)lParam;
				FillList(GetDlgItem(dlgwnd,IDC_RANGE_LIST), pUtility, 0);
			}
		}
		break;

	default: 
		return FALSE;
	}
	return TRUE;
}


HWND MaterialDlgProc::CheckAndRunApplication(const char * pWndName, const char * pFlag)
{
	CResourceCompilerHelper* rch = GetResourceCompilerHelper();
	return (HWND)rch->CallEditor( tsu->ip->GetMAXHWnd(), pWndName, pFlag );
}

//////////////////////////////////////////////////////////////////////////
void MaterialDlgProc::XmlNode2Material(XmlNodeRef & node, Mtl * pMtl)
{
	Interface * pInterface = tsu->ip;
	//TimeValue curTime = pInterface->GetTime();
	TimeValue curTime = 0;

	bool save_is = false;
	bool save_isPhys = false;
	bool save_IsSH = false;
	bool save_HasAmbientSH = false;
	bool save_IsSH2Sided = false;
	float save_SHOpacity = 1;
	TCHAR save_surfName[128];


	{
		StdMat2 *pStdMat = (StdMat2 *)pMtl;
		if(pStdMat->GetShader()->ClassID()==CrytekShaderClassID)
		{
			CrytekShader *pShader = (CrytekShader*)pStdMat->GetShader();
			save_isPhys = pShader->IsPhysicalize();
			strcpy (save_surfName, pShader->GetSurfaceName());
			save_IsSH = pShader->IsSH();
			save_HasAmbientSH = pShader->HasAmbientSH();
			save_IsSH2Sided = pShader->IsSH2Sided();
			save_SHOpacity = pShader->GetSHOpacity(0);
			save_is = true;
		}
	}

	pMtl->Reset();

	if(strlen(node->getAttr("Name"))>0)
		pMtl->SetName(_T(node->getAttr("Name")));

	if (pMtl->ClassID() != Class_ID(DMTL_CLASS_ID, 0))
		return;

	bool bTransluent = false;

	Vec3 vColor;

	StdMat2 *pStdMat = (StdMat2 *)pMtl;

	// Make sure we always use CryShader shader.
	pStdMat->SwitchShader( CrytekShaderClassID );
	CrytekShader *pShader = (CrytekShader*)pStdMat->GetShader();

	if(save_is)
	{
		pShader->SetPhysicalize(save_isPhys);
		pShader->SetSurfaceName(save_surfName);
		pShader->SetIsSH(save_IsSH );
		pShader->SetHasAmbientSH(save_HasAmbientSH);
		pShader->SetIsSH2Sided(save_IsSH2Sided );
		pShader->SetSHOpacityLevel(save_SHOpacity,0);
	}


	pStdMat->LockAmbDiffTex( FALSE );

	if (pShader)
	{
		pShader->SetLockAD(FALSE);
		pShader->SetLockDS(FALSE);
		pShader->SetShaderName( (TCHAR*)node->getAttr("Shader") );
		//pShader->SetSurfaceName((TCHAR*)node->getAttr("SurfaceType") );
	}

	vColor = Vec3(0,0,0);
	node->getAttr( "Ambient", vColor );
	pMtl->SetAmbient(Color(vColor.x, vColor.y, vColor.z), curTime);

	vColor = Vec3(0,0,0);
	node->getAttr( "Diffuse", vColor );
	pMtl->SetDiffuse(Color(vColor.x, vColor.y, vColor.z), curTime);

	vColor = Vec3(0,0,0);
	node->getAttr( "Specular", vColor );
	pMtl->SetSpecular(Color(vColor.x, vColor.y, vColor.z), curTime);
	pStdMat->SetShinStr( 1,TimeValue(0) );

	float f;

	f=0.0f;
	node->getAttr( "Shininess", f );
	pMtl->SetShininess(f/100, curTime);

	f=0.0f;
	node->getAttr( "Opacity", f );
	pStdMat->SetOpacity( f, curTime);
	if (f != 1.0f)
		bTransluent = true;


	f=0.0f;
	node->getAttr( "AlphaTest", f );
	if (f != 0)
		bTransluent = true;

	if (pShader)
		pShader->SetAlphaTest(f);


	if(node->haveAttr( "Flag_Wire"))
		pStdMat->SetWire( true );
	else
		pStdMat->SetWire( false );

	if(node->haveAttr( "Flag_2Sided"))
		pStdMat->SetTwoSided( true );
	else
		pStdMat->SetTwoSided( false );

	vColor = Vec3(0,0,0);
	node->getAttr( "Emissive", vColor );
	pStdMat->SetSelfIllumColor(Color(vColor.x, vColor.y, vColor.z), curTime);

	XmlNodeRef texturesNode = node->findChild( "Textures" );

	// Clear all texture slots.
	pMtl->SetSubTexmap(CH_DI,0);
	pMtl->SetSubTexmap(CH_BU,0);
	pMtl->SetSubTexmap(CH_BN,0);
	pMtl->SetSubTexmap(CH_SP,0);
	pMtl->SetSubTexmap(CH_OP,0);

	BitmapTex *pDiffuseTex = 0;
	bool bHaveOpacitySlot = false;
	if(texturesNode)
	{
		for (int i = 0; i < texturesNode->getChildCount(); i++)
		{
			XmlNodeRef texNode = texturesNode->getChild(i);
			const	char * pMap = texNode->getAttr( "Map");
			int nType = -1;
			if(!strcmp(pMap, "Diffuse"))
				nType = CH_DI;
			if(!strcmp(pMap, "Specular"))
				nType = CH_SP;
			if(!strcmp(pMap, "Bumpmap"))
				nType = CH_BU;
			if(!strcmp(pMap, "Normalmap"))
				nType = CH_BN;
			if(!strcmp(pMap, "Opacity"))
			{
				nType = CH_OP;
				bHaveOpacitySlot = true;
			}

			if(nType!=-1)
			{
				char pName[256];
				strcpy(pName, texNode->getAttr("File"));
				BitmapTex * pTex = NewDefaultBitmapTex();

				if (nType == CH_DI)
				{
					pDiffuseTex = pTex;
					pMtl->SetActiveTexmap(pTex);
				}

				pTex->SetMapName(pName);
				pTex->SetName(pName);
				pTex->ReloadBitmapAndUpdate();
				pMtl->SetSubTexmap(nType, pTex);
			}
		}
	}

	if (bTransluent && pDiffuseTex != 0 && !bHaveOpacitySlot)
	{
		// Also apply diffuse texture as an opacity map.
		BitmapTex * pTex = NewDefaultBitmapTex();

		pTex->SetMapName( pDiffuseTex->GetMapName() );
		pTex->SetName( pDiffuseTex->GetName() );
		pTex->SetAlphaSource(ALPHA_FILE);
		pTex->SetAlphaAsMono(TRUE);
		pTex->ReloadBitmapAndUpdate();
		pMtl->SetSubTexmap(CH_OP, pTex);
	}

	if (pDiffuseTex)
	{
		pStdMat->SetMtlFlag( MTL_DISPLAY_ENABLE_FLAGS,TRUE );
		pStdMat->ClearMtlFlag(MTL_TEX_DISPLAY_ENABLED);
		//pStdMat->ActivateTexDisplay(TRUE);
		GetCOREInterface()->ActivateTexture( 0,pMtl );
	}

	Interval iv;
	pMtl->Update( 0,iv );
}

inline Vec3 ColorToVec3(const Color & col)
{
	return Vec3(col.r, col.g, col.b);
}

void MaterialDlgProc::Material2XmlNode(Mtl * pMtl, XmlNodeRef & node )
{
	Interface * pInterface = tsu->ip;
	TimeValue curTime = pInterface->GetTime();

	node->setAttr( "Name", pMtl->GetName());

	StdMat * pStdMat = (StdMat *)pMtl;
	Shader * pShader = ((StdMat2 *)pMtl)->GetShader();

	Vec3 vec = ColorToVec3(pStdMat->GetAmbient(curTime));
	Vec3 vSpecular = ColorToVec3( pStdMat->GetSpecular(curTime) );

	// Multiply specular level with specular color.
	float fSpecularLevel = pStdMat->GetShinStr(curTime);
	vSpecular *= fSpecularLevel;
	float fShininess = pStdMat->GetShininess(curTime)*100.0f;

	node->setAttr( "Ambient", vec);
	node->setAttr( "Diffuse", ColorToVec3(pStdMat->GetDiffuse(curTime)));
	node->setAttr( "Specular", vSpecular );
	node->setAttr( "Shininess", fShininess );
	node->setAttr( "Emissive", ColorToVec3(pStdMat->GetSelfIllumColor()));

	node->setAttr( "Opacity", pStdMat->GetOpacity(curTime));
	if(pStdMat->GetWire())
		node->setAttr( "Flag_Wire",1 );
	if(pStdMat->GetTwoSided())
		node->setAttr( "Flag_2Sided",1 );

	XmlNodeRef texturesNode = node->newChild( "Textures" );

	

	int shp[]={ID_DI, ID_SP, ID_BU};

	if(pShader->ClassID()==CrytekShaderClassID)
	{
		shp[0] = CH_DI;
		shp[1] = CH_SP;
		shp[2] = CH_BU;

		CrytekShader *pCryShader = (CrytekShader*)pShader;
		node->setAttr( "AlphaTest",pCryShader->GetAlphaTest() );
	}

	if(texturesNode)
	{
		Texmap * pTex;

		if(pTex = pMtl->GetSubTexmap(shp[0]))
		{
			BitmapTex * pBMTex = (BitmapTex *)pTex;
			XmlNodeRef texNode = texturesNode->newChild( "Texture" );
			texNode->setAttr( "Map", "Diffuse");
			texNode->setAttr( "File",  pBMTex->GetMapName());
		}
		if(pTex = pMtl->GetSubTexmap(shp[1]))
		{
			BitmapTex * pBMTex = (BitmapTex *)pTex;
			XmlNodeRef texNode = texturesNode->newChild( "Texture" );
			texNode->setAttr( "Map", "Specular");
			texNode->setAttr( "File",  pBMTex->GetMapName());
		}
		if(pTex = pMtl->GetSubTexmap(shp[2]))
		{
			BitmapTex * pBMTex = (BitmapTex *)pTex;
			XmlNodeRef texNode = texturesNode->newChild( "Texture" );
			texNode->setAttr( "Map", "Bumpmap");
			texNode->setAttr( "File",  pBMTex->GetMapName());
		}

		if(pShader->ClassID()==CrytekShaderClassID)
		{
			if(pTex = pMtl->GetSubTexmap(CH_BN))
			{
				BitmapTex * pBMTex = (BitmapTex *)pTex;
				XmlNodeRef texNode = texturesNode->newChild( "Texture" );
				texNode->setAttr( "Map", "Normalmap");
				texNode->setAttr( "File",  pBMTex->GetMapName());
			}
			if(pTex = pMtl->GetSubTexmap(CH_DT))
			{
				BitmapTex * pBMTex = (BitmapTex *)pTex;
				XmlNodeRef texNode = texturesNode->newChild( "Texture" );
				texNode->setAttr( "Map", "Detail");
				texNode->setAttr( "File",  pBMTex->GetMapName());
			}
			if(pTex = pMtl->GetSubTexmap(CH_DC))
			{
				BitmapTex * pBMTex = (BitmapTex *)pTex;
				XmlNodeRef texNode = texturesNode->newChild( "Texture" );
				texNode->setAttr( "Map", "Decal");
				texNode->setAttr( "File",  pBMTex->GetMapName());
			}
			if(pTex = pMtl->GetSubTexmap(CH_SSS))
			{
				BitmapTex * pBMTex = (BitmapTex *)pTex;
				XmlNodeRef texNode = texturesNode->newChild( "Texture" );
				texNode->setAttr( "Map", "SubSurface");
				texNode->setAttr( "File",  pBMTex->GetMapName());
			}

		}
	}
}



//MaterialDlgProc  ___________________________________________________________________________________________
INT_PTR MaterialDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
	case WM_INITDIALOG:
		break;

	case  WM_DESTROY:
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_EXP_MAT_SYNC:
			{
				HWND hWnd = 0;
				m_MatSender.GetMessage();
				if(m_MatSender.m_h.GetEditorHWND() && IsWindow(m_MatSender.m_h.GetEditorHWND()))
					hWnd = m_MatSender.m_h.GetEditorHWND();
				else
					hWnd = CheckAndRunApplication("Material Editor (Sandbox)", "-MatEdit");

				if(!hWnd)
					break;

				m_MatSender.SetupWindows(hwndDlg, hWnd);

				XmlNodeRef data = new CXmlNode("Material");
				m_MatSender.SendMessage(eMSM_GetSelectedMaterial, data);
			}
			break;
		case IDC_EXP_MAT_SYNC_TO_EDITOR:
			{
				Interface * pInterface = tsu->ip;

				HWND hWnd = 0;
				m_MatSender.GetMessage();
				if(m_MatSender.m_h.GetEditorHWND() && IsWindow(m_MatSender.m_h.GetEditorHWND()))
					hWnd = m_MatSender.m_h.GetEditorHWND();
				else
					hWnd = CheckAndRunApplication("Material Editor (Sandbox)", "-MatEdit");
				if(!hWnd )
					break;

				
				m_MatSender.SetupWindows(hwndDlg, hWnd);

				IMtlEditInterface* pMtlEdit= GetMtlEditInterface ();
				MtlBase * pMtl = pMtlEdit->GetCurMtl();
				if(!pMtl)
				{
					MessageBox(pInterface->GetMAXHWnd(), "You need to select the material first.", "Sync failed", MB_OK | MB_ICONERROR);
					break;
				}

				if(IsTex(pMtl))
				{
					MessageBox(pInterface->GetMAXHWnd(), "Currently selected texture in Max Material Editor Slot. Please, select the material. May be you need Go to Parent.", "Sync failed", MB_OK | MB_ICONERROR);
					break;
				}

				bool IsErr = true;

				if(IsMtl(pMtl))
				{
					if(pMtl->IsMultiMtl())
						IsErr = false;
					else if(pMtl->ClassID()==Class_ID(DMTL_CLASS_ID,0))
						IsErr = false;
					else if(pMtl->ClassID()==Class_ID(DMTL2_CLASS_ID,0))
						IsErr = false;
				}

				if(IsErr)
				{
					MessageBox(pInterface->GetMAXHWnd(), "Currently selected undefined material type. Please, select standard material or multi material. May be you need select up level.", "Sync failed", MB_OK | MB_ICONERROR);
					break;
				}

				XmlNodeRef data = new CXmlNode("ExportMaterial");
				data->setAttr("Name", pMtl->GetName());
				char *sMaxFilename = GetCOREInterface()->GetCurFilePath();
				data->setAttr( "MaxFile",sMaxFilename );

				XmlNodeRef node = data->newChild("Material");

				if(pMtl->IsMultiMtl())
				{
					data->setAttr("IsMulti", 1);

					XmlNodeRef childsNode = node->newChild( "SubMaterials" );
					for(int i=0; i<((Mtl*)pMtl)->NumSubMtls(); i++)
					{
						Mtl * pSubMtl = ((Mtl*)pMtl)->GetSubMtl(i);
						if(pSubMtl )
						{
							XmlNodeRef mtlNode = childsNode->newChild( "Material" );
							mtlNode->setAttr( "Name",pSubMtl->GetName() );
							Material2XmlNode(pSubMtl, mtlNode);
						}
					}
				}
				else
				{
					data->setAttr("IsMulti", 0);
					Material2XmlNode((Mtl*)pMtl, node);
				}
				
				SetForegroundWindow(hWnd);
				m_MatSender.SendMessage(eMSM_Create, data);
			}
			break;
		}

		break;
	
	case WM_MATEDITSEND:
		{
			if(m_MatSender.GetMessage())
			{
				if(m_MatSender.m_h.msg==eMSM_GetSelectedMaterial)
				{
					Interface * pInterface = tsu->ip;
					XmlNodeRef data = m_MatSender.m_node;

					if(!data)
					{
						MessageBox(pInterface->GetMAXHWnd(), "Sync failed", "1", MB_OK | MB_ICONERROR);
						break;
					}

					Mtl * pMtl = 0;

					char pName[256]={0};

					const char * pCh = data->getAttr("Name");
					if(pCh)
						strcpy(pName, pCh);

					const char * pFileName = data->getAttr("FileName");
					const char * pCurFile = pInterface->GetCurFilePath();
					
					
					if(pCurFile && *pCurFile && pFileName && *pFileName && *pName)
					{
						const char * ch1 = strrchr(pCurFile, '\\');
						const char * ch2 = strrchr(pFileName, '\\');
						if(ch1 && ch2 && !strnicmp(pCurFile, pFileName, ch1-pCurFile))
						{
							char * ch = strrchr(pName, '\\');
							if(!ch)
								ch = strrchr(pName, '/');
							if(ch)
								strcpy(pName, ch+1);
						}
					}

					MtlBaseLib * MatLib = pInterface->GetSceneMtls();
					if(!MatLib)
					{
						MessageBox(pInterface->GetMAXHWnd(), "Sync failed", "2", MB_OK | MB_ICONERROR);
						break;
					}

					TSTR name = _T(pName);
					int id = MatLib->FindMtlByName(name);
					if(id==-1)
					{
						IMtlEditInterface* pMtlEdit= GetMtlEditInterface ();
						if(!pMtlEdit)
						{
							MessageBox(pInterface->GetMAXHWnd(), "Sync failed", "3", MB_OK | MB_ICONERROR);
							break;
						}
						int i=0;
						MtlBase* pSelectedMaterial=0;
						MtlBase* pTmpMtl=0;
						for(int i=0; i<24; i++)
						{
							pTmpMtl = pMtlEdit->GetTopMtlSlot(i);
							if(!pTmpMtl)
								break;
							if(!strcmp(pTmpMtl->GetName(), name))
							{
								pSelectedMaterial = pTmpMtl;
								break;
							}
						}

						if(!pSelectedMaterial)
							pSelectedMaterial = pMtlEdit->GetCurMtl();
						//const int ActiveMaterialSlot= pMtlEdit->GetActiveMtlSlot();
						if(pSelectedMaterial)
						{
							if(!strcmp(pSelectedMaterial->GetName(), name) || MessageBox(pInterface->GetMAXHWnd(), "There is no material in scene with a selected name. Do you want to sync with a current material?", pName, MB_YESNO)==IDYES)
							{
								pMtl = (Mtl * )pSelectedMaterial;
							}
						}
					}
					else
					{
						MtlBaseHandle pMtlH = (*MatLib)[id];
						pMtl = (Mtl * )pMtlH;
					}

					if(!pMtl)
					{
						MessageBox(pInterface->GetMAXHWnd(), "Sync failed", "Error", MB_OK | MB_ICONERROR);
						break;
					}

					XmlNodeRef node = data->findChild( "Material" );
					if(!node)
					{
						MessageBox(pInterface->GetMAXHWnd(), "Sync failed", "4", MB_OK | MB_ICONERROR);
						break;
					}


					int IsMulti=0;
					//data->getAttr("IsMulti", IsMulti);
					XmlNodeRef childsNode = node->findChild( "SubMaterials" );
					if (childsNode)
						IsMulti=1;

					if(IsMulti)
					{
						int nNumSubMtls;
						//data->getAttr("NumSubMtls", nNumSubMtls);
						nNumSubMtls = childsNode->getChildCount();
						
						//int nNumChild = data->getChildCount();
						if(!pMtl->IsMultiMtl())
						{
							pMtl = NewDefaultMultiMtl();
							pMtl->SetName(name);
							IMtlEditInterface* pMtlEdit= GetMtlEditInterface ();
							int ActiveMaterialSlot= pMtlEdit->GetActiveMtlSlot();
							pMtlEdit->PutMtlToMtlEditor(pMtl, ActiveMaterialSlot);
						}

						pMtl->SetName(name);
						if(pMtl->NumSubMtls()!=nNumSubMtls)
						{
							((MultiMtl *)pMtl)->SetNumSubMtls(nNumSubMtls);
						}

						for(int i=0; i<nNumSubMtls; i++)
						{
							/*
							XmlNodeRef child = data->getChild(i);
							int id;
							child->getAttr("id", id);
							Mtl * pSubMtl = pMtl->GetSubMtl(id);
							XmlNode2Material(child, pSubMtl);
							*/
							XmlNodeRef mtlNode = childsNode->getChild(i);
							if(!mtlNode)
							{
								MessageBox(pInterface->GetMAXHWnd(), "Sync failed", "5", MB_OK | MB_ICONERROR);
								break;
							}

							if (mtlNode->isTag("Material"))
							{
								Mtl * pSubMtl = pMtl->GetSubMtl(i);
								XmlNode2Material(mtlNode, pSubMtl);
							}
						}
						//strcpy(pName, data->getAttr("Name"));
					}
					else
					{
						if(pMtl->IsMultiMtl())
						{
							pMtl = NewDefaultStdMat();
							pMtl->SetName(name);
							IMtlEditInterface* pMtlEdit= GetMtlEditInterface ();
							int ActiveMaterialSlot= pMtlEdit->GetActiveMtlSlot();
							pMtlEdit->PutMtlToMtlEditor(pMtl, ActiveMaterialSlot);
						}
						pMtl->SetName(name);
						XmlNode2Material(node, pMtl);
					}


					/*
					HWND hWnd ;
					hWnd = GetDlgItem(hwmEdit, 0x400);
					if(hWnd)
						SetWindowText(hWnd, pName);

					hWnd = GetDlgItem(hwmEdit, 0x3E9);
					if(hWnd)
						SetWindowText(hWnd, pName);

					if(data->getAttr("dif_tex"))
					{
						strcpy(pName, data->getAttr("dif_tex"));
						BitmapTex * pTex = NewDefaultBitmapTex();

						pTex->SetMapName(pName);
						pTex->SetName(pName);
						pTex->ReloadBitmapAndUpdate();
						pMtl->SetSubTexmap(CH_DI, pTex);
					}
					*/


					//NotifyChanged();
					//UpdateMtlDisplay();
					//ReloadDialog();
				}
			}
			else
				MessageBox(NULL, "Null message", "", MB_OK);
			break;
		}

	default: 
		return FALSE;
	}
	return TRUE;
}

//PhysicsDlgProc  ___________________________________________________________________________________________
INT_PTR PhysicsDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
	case WM_INITDIALOG:
		{
			HWND hComboBox = GetDlgItem(hwndDlg,IDC_GRANULARITY);
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Very coarse") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Coarse") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Moderate") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Fine") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Very fine") );
			int nCurSel = tsu->pb_physics->GetInt(PB_GRANULARITY);
			SendMessage( hComboBox,CB_SETCURSEL,nCurSel,0 );

			hComboBox = GetDlgItem(hwndDlg,IDC_MODE);
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Automatic") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Handle") );
			SendMessage( hComboBox,CB_SETCURSEL,0,0 );
		}
		break;

	case  WM_DESTROY:
		break;

	case WM_ERASEBKGND:
		{
			// Refresh.
			HWND hComboBox = GetDlgItem(hwndDlg,IDC_GRANULARITY);
			int nCurSel = tsu->pb_physics->GetInt(PB_GRANULARITY);
			SendMessage( hComboBox,CB_SETCURSEL,nCurSel,0 );
		}


	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_GRANULARITY:
			{
				HWND hComboBox = GetDlgItem(hwndDlg,IDC_GRANULARITY);
				int nCurSel = SendMessage( hComboBox,CB_GETCURSEL,0,0 );
				if (nCurSel != CB_ERR)
				{
					tsu->pb_physics->SetValue(PB_GRANULARITY,0,nCurSel);
				}
				break;
			}
		}
		break;

	default: 
		return FALSE;
	}
	return TRUE;
}


void BatchDlgProc::ExportAllFilesInFolder(const char * folder)
{
	char path[1024];
	strcpy(path, folder);
	strcat(path, "\\*.*");

	WIN32_FIND_DATA fd;
	HANDLE hff = FindFirstFile(path, &fd);
	BOOL bIsFind = TRUE;
	char newpath[1024];
	while(hff && bIsFind)
	{
		if(fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			if(strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
			{
				sprintf(newpath, "%s\\%s", folder, fd.cFileName);
				ExportAllFilesInFolder(newpath);
			}
		}
		else
		{
			if(strlen(fd.cFileName)>4 && !strcmp(&fd.cFileName[strlen(fd.cFileName)-4], ".max"))
			{
				sprintf(newpath, "%s\\%s", folder, fd.cFileName);
				if(tsu->ip->IsMaxFile(newpath))
				{
					tsu->ip->LoadFromFile(newpath);

					tsu->ConfigLoad(true);
					tsu->LoadConfigFromMaxFile();

					bool bPrev = bDontShowMessageBoxes;
					bDontShowMessageBoxes = true;
					ExportButton(tsu);
					bDontShowMessageBoxes = bPrev;
				}
			}
		}
		bIsFind = FindNextFile(hff, &fd);
	}
}


//BatchDlgProc  ___________________________________________________________________________________________
INT_PTR BatchDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
	case WM_INITDIALOG:
		{
		}
		break;

	case  WM_DESTROY:
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_BATCH:
			{
				char path[1024]="";
				strcpy(path, tsu->ip->GetCurFilePath());
				tsu->ip->ChooseDirectory(tsu->ip->GetMAXHWnd(), "Choose folder", path);
				if(strlen(path)>0)
				{
					char out[512];
					sprintf(out, "Do you want to export all max files from folder %s and from all sub folders?", path);
					if(MessageBox(tsu->ip->GetMAXHWnd(), out, "Batch export", MB_YESNO | MB_ICONQUESTION)==IDYES)
						ExportAllFilesInFolder(path);
				}
				//MessageBox(NULL, path, "", MB_OK);
			}
			break;

		case IDC_TESTSUITE:
			{
				tsu->RunTestSuite();
			}
			break;
		}
		break;

	default: 
		return FALSE;
	}
	return TRUE;
}


//NodesDlgProc  ___________________________________________________________________________________________
INT_PTR NodesDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam);
	switch (msg) 
	{
	case WM_INITDIALOG:
		{
			//tsu->Init(hWnd);
			ICustButton *but=GetICustButton(GetDlgItem(hWnd,IDC_EXPORT_NODES));
			but->SetImage(tsu->ButtonImages,0,0,0,0,125,20);
			ReleaseICustButton(but);

			HWND hComboBox = GetDlgItem(hWnd,IDC_EXPORT_TO);
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Geometry (*.cgf)") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Character (*.chr)") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Animated Geometry (*.cga)") );
			SendMessage( hComboBox,CB_ADDSTRING,0,(LPARAM)_T("Animation for cga (*.anm)") );
			int nCurSel = tsu->pb_nodes->GetInt(PB_EXPORT_TO);
			SendMessage( hComboBox,CB_SETCURSEL,nCurSel,0 );
		}
		break;

	case WM_DESTROY:
		//tsu->Destroy(hWnd);
		break;

	case WM_ERASEBKGND:
		{
			// Refresh.
			HWND hComboBox = GetDlgItem(hWnd,IDC_EXPORT_TO);
			int nCurSel = tsu->pb_nodes->GetInt(PB_EXPORT_TO);
			SendMessage( hComboBox,CB_SETCURSEL,nCurSel,0 );
		}

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_EXPORT_NODES:
			ExportButton(tsu);
			break;

		case IDC_EXPORT_NODES_COLLADA:
			ExportColladaButton(tsu);
			break;

		case IDC_PREVIEW_NODES:
			{
				tsu->hProgressWindow= CreateDialog(hInstance, MAKEINTRESOURCE(IDD_PROGRESS), tsu->hwnd_nodes, ProgressDlgProc);
				tsu->hProgressBar	= GetDlgItem(tsu->hProgressWindow, IDC_PROGRESS);
				RECT maxrect, myrect;
				GetWindowRect(tsu->ip->GetMAXHWnd(), &maxrect);
				GetWindowRect(tsu->hProgressWindow, &myrect);
				SetWindowPos(tsu->hProgressWindow, HWND_TOP, (maxrect.right+maxrect.left-myrect.right+myrect.left)>>1, (maxrect.bottom+maxrect.top-myrect.bottom+myrect.top)>>1,0,0,SWP_NOSIZE);
				ShowWindow(tsu->hProgressWindow, SW_SHOW);
				
				bPreviewMode = true;
				int nSaved = tsu->SaveGeomFile();
				bPreviewMode = false;
				
				DestroyWindow(tsu->hProgressWindow);
				
				tsu->hProgressBar = tsu->hProgressWindow = NULL;
				
				switch (nSaved)
				{
				case IDYES:
					tsu->PreviewGeomFile();
					break;
				case IDCANCEL:
					MessageBox (tsu->ip->GetMAXHWnd(),
						(const char*)(tsu->lastSavedFile + " file was not saved because the user cancelled the action"), "Warning", MB_OK|MB_ICONASTERISK);
					break;
					break;
				case IDNO:
					WarningFileNotSaved (tsu->ip->GetMAXHWnd(), tsu->lastSavedFile);
					break;
				default:
					{
						char szError[100];
						sprintf (szError, "%d", nSaved);
					MessageBox (tsu->ip->GetMAXHWnd(),
						(const char*)(tsu->lastSavedFile + " file was not saved because of unknown error " + szError), "Warning", MB_OK|MB_ICONASTERISK);
					}
					break;
				}
			}
			break;

		case IDC_CUSTOMFILENAMEENTER:
			{
				tsu->SelectCustomExportFilename();
			}
			break;

		case IDC_PREVIEW_GAME:
			{
				tsu->hProgressWindow= CreateDialog(hInstance, MAKEINTRESOURCE(IDD_PROGRESS), tsu->hwnd_nodes, ProgressDlgProc);
				tsu->hProgressBar	= GetDlgItem(tsu->hProgressWindow, IDC_PROGRESS);
				RECT maxrect, myrect;
				GetWindowRect(tsu->ip->GetMAXHWnd(), &maxrect);
				GetWindowRect(tsu->hProgressWindow, &myrect);
				SetWindowPos(tsu->hProgressWindow, HWND_TOP, (maxrect.right+maxrect.left-myrect.right+myrect.left)>>1, (maxrect.bottom+maxrect.top-myrect.bottom+myrect.top)>>1,0,0,SWP_NOSIZE);
				ShowWindow(tsu->hProgressWindow, SW_SHOW);
				
				bPreviewMode = true;
				int nSaved = tsu->SaveGeomFile();
				bPreviewMode = false;
				
				DestroyWindow(tsu->hProgressWindow);
				
				tsu->hProgressBar = tsu->hProgressWindow = NULL;
				
				switch (nSaved)
				{
				case IDYES:
					tsu->PreviewGame( tsu->lastSavedFile );
					break;
				case IDNO:
					WarningFileNotSaved (tsu->ip->GetMAXHWnd(), tsu->lastSavedFile);
					break;
				}
			}
			break;
		
		case IDC_PREVIEW_EXPLORE:
			{
				char filename[512];
				if(tsu->ip->GetCurFilePath())
				{
					strcpy(filename, tsu->ip->GetCurFilePath());
					if(strlen(filename)>0)
					{
						char * ch;
						if(ch = strrchr(filename, '\\'))
						{
							*(ch+1)=0;
							ShellExecute(NULL, "open",filename, "", NULL, SW_SHOWNORMAL);
						}
					}
				}
			}
			break;

		case IDC_NODE_CLEAR:
			tsu->pb_nodes->ZeroCount(PB_NODES);
			break;

		case IDC_NODE_ADD_SEL:
			{
				int n=tsu->ip->GetSelNodeCount();
				for(int i=0;i<n;i++)
				{
					INode *node=tsu->ip->GetSelNode(i);
					PB2Value v;
					v.r=node;
					if(node_validator.Validate(v))
					{
						tsu->pb_nodes->Append(PB_NODES,1,&node);
					}
				}
			}
			break;
		
		case IDC_EXPORT_TO:
			{
				HWND hComboBox = GetDlgItem(hWnd,IDC_EXPORT_TO);
				int nCurSel = SendMessage( hComboBox,CB_GETCURSEL,0,0 );
				if (nCurSel != CB_ERR)
				{
					tsu->pb_nodes->SetValue(PB_EXPORT_TO,0,nCurSel);
				}
				break;
			}
		}
		break;
	}
	return FALSE;
}

//BonesDlgProc  ___________________________________________________________________________________________
INT_PTR BonesDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam);
	switch (msg) 
	{
	case WM_INITDIALOG:
		{
			ICustButton *but=GetICustButton(GetDlgItem(hWnd,IDC_EXPORT_BONES));
			but->SetImage(tsu->ButtonImages,1,1,1,1,125,20);
			ReleaseICustButton(but);
		}
		break;

	case WM_DESTROY:
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_EXPORT_BONES:
		{
			tsu->hProgressWindow= CreateDialog(hInstance, MAKEINTRESOURCE(IDD_PROGRESS), tsu->hwnd_bones, ProgressDlgProc);
			tsu->hProgressBar	= GetDlgItem(tsu->hProgressWindow, IDC_PROGRESS);
			RECT maxrect, myrect;
			GetWindowRect(tsu->ip->GetMAXHWnd(), &maxrect);
			GetWindowRect(tsu->hProgressWindow, &myrect);
			SetWindowPos(tsu->hProgressWindow, HWND_TOP, (maxrect.right+maxrect.left-myrect.right+myrect.left)>>1, (maxrect.bottom+maxrect.top-myrect.bottom+myrect.top)>>1,0,0,SWP_NOSIZE);
			ShowWindow(tsu->hProgressWindow, SW_SHOW);
			
			int nResult = tsu->SaveBoneAnimFile("");

			DestroyWindow(tsu->hProgressWindow);

			tsu->hProgressBar = tsu->hProgressWindow = NULL;

			switch (nResult)
			{
			case IDNO:
				WarningFileNotSaved (tsu->ip->GetMAXHWnd(), "");
				break;
			}
		}

		break;
		
		case IDC_BONE_CLEAR:
			tsu->pb_bones->ZeroCount(PB_BONES);
			break;

		case IDC_BONE_ADD_SEL:
			{
				int n=tsu->ip->GetSelNodeCount();
				for(int i=0;i<n;i++)
				{
					INode *node=tsu->ip->GetSelNode(i);
					PB2Value v;
					v.r=node;
					if(bone_validator.Validate(v))
					{
						tsu->AddBoneToList(node);
					}
				}
			}
			break;
		}
		break;

	}
	return FALSE;
}

/*===========================================================================*\
 |  Utility implimentations
\*===========================================================================*/

static int once = 1;

static void FileOpenNotifyProc(void* param, NotifyInfo* pInfo)
{
	CSExportUtility *expUtil = (CSExportUtility*)param;
	expUtil->bClosingFile = false;
	expUtil->LoadConfigFromMaxFile();
	IParamMap2* pParamMapTiming = expUtil->pb_timing->GetMap();
	HWND hTimingPanel = pParamMapTiming->GetHWnd();
	::SendMessage(hTimingPanel, WM_APP + 10013, 0, (LPARAM)expUtil);
}

static void FilePreOpenNotifyProc(void* param, NotifyInfo* pInfo)
{
	CSExportUtility *expUtil = (CSExportUtility*)param;
	expUtil->bClosingFile = true;
}

CSExportUtility::CSExportUtility()
{
	this->bClosingFile = false;

	node_validator.util=this;
	bone_validator.util=this;
	
	iu			=NULL;
	ip			=NULL;
	pb_nodes	=NULL;
	pb_bones	=NULL;
	pmap_nodes	=NULL;
	pmap_bones	=NULL;
	hwnd_nodes	=NULL;
	hwnd_bones	=NULL;
	hwnd_about	=NULL;
	ChunkList.Reset();

	hProgressWindow	=NULL;
	hProgressBar	=NULL;
	bExportNew = false;
	//ActiveNode		=NULL;
	bCanMergeNodes = false;

	m_pidlLast = NULL;
	preExportCallback = 0;
}

CSExportUtility::~CSExportUtility()
{
	node_validator.util=NULL;
	bone_validator.util=NULL;
}

int	CSExportUtility::GetTime()
{
	// Always Export at 0 time
	return 0;

	//return ip->GetTime();
}

//  ______________________________________________________________________________________
void CSExportUtility::BeginEditParams(Interface *ip,IUtil *iu) 
{
	RegisterNotification(FileOpenNotifyProc, this, NOTIFY_FILE_POST_OPEN);
	RegisterNotification(FilePreOpenNotifyProc, this, NOTIFY_FILE_PRE_OPEN);

	ButtonImages=ImageList_Create(125,20, ILC_COLOR24 | ILC_MASK  , 2, 2);
	assert(ButtonImages);
	HBITMAP hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BUTTONS));
	HBITMAP hMask	= LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BUTTONS_MASK));
	ImageList_Add(ButtonImages, hBitmap, hMask);
	DeleteObject(hBitmap);
	DeleteObject(hMask);

	//CSExportUtilCD.MakeAutoParamBlocks(this);

	int wasUninitialised = once;
	if (once)
		CSExportUtilCD.MakeAutoParamBlocks(this);
	once = 0;

	this->iu = iu;
	this->ip = ip;

	//CSExportUtilCD.BeginEditParams((IObjParam*)ip, this, 0, this);
	CSExportUtilCD.BeginEditParams((IObjParam*)ip, this, 0, 0);

	bones_param_blk.SetUserDlgProc(new BonesDlgProc(this));
	nodes_param_blk.GetParamDef(PB_EXPORT_TO).accessor = new NodesExportToAccessor(this);
	nodes_param_blk.GetParamDef(PB_CUSTOM_FILENAME).accessor = new NodesCustomFilenameAccessor(this);
	nodes_param_blk.GetParamDef(PB_CUSTOM_FILENAME).validator = new NodesCustomFilenameValidator(this);
	nodes_param_blk.SetUserDlgProc(new NodesDlgProc(this));
	options_param_blk.SetUserDlgProc(new OptionsDlgProc(this));
	timing_param_blk.SetUserDlgProc(new TimingDlgProc(this));
	material_param_blk.SetUserDlgProc(new MaterialDlgProc(this));
	physics_param_blk.SetUserDlgProc(new PhysicsDlgProc(this));
	batch_param_blk.SetUserDlgProc(new BatchDlgProc(this));

	pmap_nodes	=pb_nodes->GetMap();
	pmap_bones	=pb_bones->GetMap();

	hwnd_nodes	=pmap_nodes->GetHWnd();
	hwnd_bones	=pmap_bones->GetHWnd();

	if (wasUninitialised)
	{
		// Set a default custom export filename.
		pb_nodes->SetValue(PB_CUSTOM_FILENAME, 0, const_cast<TCHAR*>(FindDefaultCustomExportParameter().c_str()));
	}

	//hwnd_about	=ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_ABOUT), AboutDlgProc, "About");

	EnableDisableControls();

/*
#ifdef PROTECTION_ENABLED
#pragma message ("===========Protection in BeginEditParams Enabled===========")
	if(time_Bomb_fired || !CheckIntegrity())
	{
		ip->RegisterTimeChangeCallback((TimeChangeCallback *)13051972);
	}
#endif
*/

	if (wasUninitialised)
	{
		ConfigLoad(true);
		LoadConfigFromMaxFile();
	}

	// Guard against the custom export filename being corrupt - it may be a corrupt value coming from
	// the saved settings.
	const TCHAR* szFilename = pb_nodes->GetStr(PB_CUSTOM_FILENAME);
	if (szFilename == 0 || PathUtil::IsAbsolutePath(szFilename))
		pb_nodes->SetValue(PB_CUSTOM_FILENAME, 0, const_cast<TCHAR*>(FindDefaultCustomExportParameter().c_str()));
}
	
//  ______________________________________________________________________________________
void CSExportUtility::EndEditParams(Interface *ip,IUtil *iu) 
{
	UnRegisterNotification(FilePreOpenNotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(FileOpenNotifyProc, this, NOTIFY_FILE_POST_OPEN);

	//====================
	//Correct the MAX BUG: 
	//====================
	//Nuke the command mode if we are in Pick... state.
	//This is a MAX BUG: MAX fails to remove that command mode and it crashes.
	CommandMode* cmdMode = GetCOREInterface()->GetCommandMode();
	if((!cmdMode) || cmdMode->ID()==CID_STDPICK) GetCOREInterface()->SetStdCommandMode(CID_OBJSELECT);
		
	//ConfigSave(true);
	SaveConfigToMaxFile();

	if(hwnd_about) ip->DeleteRollupPage(hwnd_about);

	CSExportUtilCD.EndEditParams((IObjParam*)ip, this, END_EDIT_REMOVEUI, this);

	this->iu = NULL;
	this->ip = NULL;

	pmap_nodes=pmap_bones=NULL;
	hwnd_about=hwnd_nodes=hwnd_bones=NULL;

	ImageList_Destroy(ButtonImages);
}

//  ______________________________________________________________________________________
IParamBlock2* CSExportUtility::GetParamBlock(int i)
{ 
	switch(i)
	{
	case 0: return pb_nodes;
	case 1: return pb_bones;
	case 2: return pb_options;
	case 3: return pb_timing;
	case 4: return pb_material;
	case 5: return pb_physics;
	case 6: return pb_batch;
	}
	return NULL;
} // return i'th ParamBlock


//  ______________________________________________________________________________________
IParamBlock2* CSExportUtility::GetParamBlockByID(short id) 
{
	switch(id)
	{
	case pbid_Nodes:	return pb_nodes;
	case pbid_Bones:	return pb_bones;
	case pbid_Options:	return pb_options;
	case pbid_Timing:	return pb_timing;
	case pbid_Material:	return pb_material;
	case pbid_Physics:	return pb_physics;
	case pbid_Batch:	return pb_batch;
	}
	return NULL; 
} // return ParamBlock given ID


/*===========================================================================*\
 |	Subanim & References support
\*===========================================================================*/

RefTargetHandle CSExportUtility::GetReference(int i)
{
	switch (i) 
	{
		case 0: return pb_nodes;
		case 1: return pb_bones;
		case 2: return pb_options;
		case 3: return pb_timing;
		case 4: return pb_material;
		case 5: return pb_physics;
		case 6: return pb_batch;
		default: return NULL;
	}
}

//  ______________________________________________________________________________________
void CSExportUtility::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) 
	{
		case 0:
			pb_nodes	= (IParamBlock2*)rtarg;
			break;

		case 1:
			pb_bones	= (IParamBlock2*)rtarg;
			break;

		case 2: pb_options	= (IParamBlock2*)rtarg; break;
		case 3: pb_timing	= (IParamBlock2*)rtarg; break;
		case 4: pb_material	= (IParamBlock2*)rtarg; break;
		case 5: pb_physics	= (IParamBlock2*)rtarg; break;
		case 6: pb_batch = (IParamBlock2*)rtarg; break;
	}
}

//  ______________________________________________________________________________________
RefResult CSExportUtility::NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget,PartID& partID,  RefMessage message) 
{
	static bool avoid_recurse=false;

	switch (message) 
	{
		case REFMSG_CHANGE:
			nodes_param_blk.InvalidateUI();
			bones_param_blk.InvalidateUI();

			EnableDisableControls();

			if(hTarget == pb_nodes)
			{
				int last_id=pb_nodes->LastNotifyParamID();
				if((last_id == PB_NODES) || (last_id == PB_ADD_WHOLE_SKEL) || (last_id == PB_ADD_RELATED))
				{
					if(pb_nodes->GetInt(PB_ADD_RELATED))
					{
						AddRelatedBones();
					}
				}
			}
			else if(hTarget == pb_bones)
			{
				int last_id=pb_bones->LastNotifyParamID();
				if(last_id == PB_BONES) 
				{
					if(int n=pb_bones->Count(PB_BONES))
					{
						INode *LastNode=pb_bones->GetINode(PB_BONES,0,n-1);
						if(LastNode)
						{
							char *s=LastNode->GetName();
							if(!avoid_recurse)
							{
								avoid_recurse=true;
								RemoveChildBones(LastNode);
								avoid_recurse=false;
							}
						}
					}
				}
			}

			break;
	}
	return REF_SUCCEED;
}

inline bool IsEqualPoint3Epsilon( Point3 &p1,Point3 &p2,float epsilon )
{
	if (fabs(p1.x-p2.x) < epsilon && fabs(p1.y-p2.y) < epsilon && fabs(p1.z-p2.z) < epsilon)
		return true;

	return false;
}

bool CSExportUtility::IsChildOf(ISourceObject* pChild,ISourceObject *pParent)
{
	ISourceObject *pObj = pChild->GetParent();
	while (pObj && pObj != pParent)
	{
		pObj = pObj->GetParent();
	}
	return pObj == pParent;
}

//  ______________________________________________________________________________________
// IDYES - continue export
// IDNO - stop export, warn the user
// IDCANCEL - stop export, the user doesn't need to be warned
// ID_SKIP_NODE - continue the export, but skip the node
int CSExportUtility::CheckNodeProblems(ISourceObject* pSourceObject)
{
	INode *node = pSourceObject->GetMaxNode();

	TimeValue time=GetTime();

	if (!node) return IDYES;
												 
	//get necessary node properties
	char		name[256];
	AffineParts ap;
	Point3		decomposed_p;
	Quat		decomposed_r;
	Point3		decomposed_s;
	Matrix3		tm				= node->GetObjTMAfterWSM(time);
	decomp_affine(tm,&ap);
	DecomposeMatrix(tm, decomposed_p, decomposed_r, decomposed_s);
	strcpy(name,node->GetName());
	
	//==================================
	//Check non-uniform off-axis scaling
	//==================================
	float EPSILON=1e-8f;
	float l=max(EPSILON ,Length(decomposed_s));
	if((Length(ap.k-decomposed_s)/l)>0.001f && pb_options->GetInt(PB_WARN_OFFAXIS_SCL))
	{
		if (!bPreviewMode)
		{
			int res=CryMessageBox(hwnd_nodes,"Node has off-axis non-uniform scaling\nDo you still want to export this node",name,MB_OKCANCEL);
			if(res==IDCANCEL) return IDNO;
		}
	}

	//get mesh if this is a geom object
	Object		*der_obj= node->GetObjectRef();
	Object		*obj	= der_obj->Eval(time).obj;
	SClass_ID	sid		= obj->SuperClassID();
	Class_ID	cid		= obj->ClassID();

	Matrix3 tmo = node->GetObjectTM(0);

	if(sid != GEOMOBJECT_CLASS_ID) return IDYES;

	if(!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
	{
		if (bPreviewMode)
			return IDYES;
		int res=MessageBox(hwnd_nodes,"Node contains a geometric object but cannot be converted to mesh\nDo you still want to export this node",name,MB_YESNOCANCEL);
		switch (res)
		{
		case IDYES:
			return IDYES;
		case IDNO:
			return ID_SKIP_NODE;
		case IDCANCEL:
			return IDCANCEL;
		}
	}

	TriObject	*tri	= (TriObject *) obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
	BOOL		needDel = (obj != tri);
	Mesh		*mesh	= &tri->mesh;
	if (!mesh)
	{
		if (bPreviewMode)
			return IDYES;
		int res=MessageBox(hwnd_nodes,"Serious Warning: Node's triangle mesh could not be obtained\nDo you still want to export this node",name,MB_YESNOCANCEL);
		switch (res)
		{
		case IDYES:
			return IDYES;
		case IDNO:
			return ID_SKIP_NODE;
		case IDCANCEL:
			return IDCANCEL;
		}
	}

	int nVerts = mesh->getNumVerts();
	int nFaces = mesh->getNumFaces();
	int nTVerts= mesh->getNumTVerts();
	int nCVerts= mesh->getNumVertCol();

	Face		*Faces	  = mesh->faces;
	Point3		*Verts	  = mesh->verts;
	TVFace		*TexFaces = mesh->tvFace;
	UVVert		*TexVerts = mesh->tVerts;
	VertColor	*ColVerts = mesh->vertCol;
	TVFace		*ColFaces = mesh->vcFace;

	//======================
	//Check non-rigid links
	//======================
	Modifier *phy_mod=FindPhysiqueModifier(der_obj);
	//ms->NonRigid_Links =false;
	if(phy_mod)
	{
		IPhysiqueExport *phyInterface = (IPhysiqueExport *)( phy_mod->GetInterface(I_PHYINTERFACE));
		if(phyInterface)
		{

			IPhyContextExport *phyContex = phyInterface->GetContextInterface(node);
			if(phyContex)
			{
				//phyContex->ConvertToRigid(false);
				//phyContex->AllowBlending(true);

				Matrix3 tm = node->GetObjTMAfterWSM(time);

				int n_vert=phyContex->GetNumberVertices();
				for(int i=0;i<n_vert;i++)
				{
					UpdateProgress(100*i/n_vert,"\"%s\" checking bone weights", name);

					Point3 vertex_at_world=tm*Verts[i];

					IPhyVertexExport *vert=phyContex->GetVertexInterface(i);
					int vert_type=vert->GetVertexType();
					phyContex->ReleaseVertexInterface(vert);
					
					if((vert_type != RIGID_NON_BLENDED_TYPE) && (vert_type != RIGID_BLENDED_TYPE) && pb_options->GetInt(PB_WARN_NONRIGID))
					{
						//ms->NonRigid_Links=true;
						sprintf(s,"Some of the vertices of \"%s\" are using DEFORMABLE Physique links."
							"\nOnly RIGID links can be exported. Although the exporter can convert these DEFORMABLE vertices to RIGID ones,"
							"\nthis process may cause loss of precision and the resulting mesh deformation might be different from the one seen in MAX"
							"\nWe STRONGLY RECOMMEND you to re-initialize the Physique so that only RIGID links are used."
							"\nyou can see what the exported animation will look like by setting the Pysique \"Viewport Skin Update\" parameter to \"Rigid\""
							"\n\nDo you still want to export this node?",name);
						int res=CryMessageBox(hwnd_nodes, s, "IMPORTANT WARNING!!!", MB_OKCANCEL | MB_ICONHAND);
						if(res==IDCANCEL)
						{
							phyInterface->ReleaseContextInterface(phyContex);
							phy_mod->ReleaseInterface(I_PHYINTERFACE, phyInterface);
							return IDCANCEL;
						}
						break;
					}
				}			
				phyInterface->ReleaseContextInterface(phyContex);
			}
			phy_mod->ReleaseInterface(I_PHYINTERFACE, phyInterface);
		}							
	}

	{
		//===================
		//Check orphan tverts
		//===================
		//ms->misshared_tverts=false;
		//ms->orphan_tverts	=false;
		bool misshared_tverts	= false;
		bool orphan_tverts		= false;
		if(nTVerts)
		{
			int *tvert_used=(int *)calloc(nTVerts, sizeof(int));
			int *tvert_vert=(int *)calloc(nTVerts, sizeof(int));
			assert(tvert_used);
			assert(tvert_vert);

			IntTab Degen_Faces;
			IntTab Degen_VertIdx;
			for(int i=0;i<nFaces;i++)
			{
				for(int j=0;j<3;j++)
				{
					int na=tvert_used[TexFaces[i].t[j]]++;
					if(na)
					{
						if(tvert_vert[TexFaces[i].t[j]] != Faces[i].v[j])
						{
							//ms->misshared_tverts=true;
							misshared_tverts=true;
							DebugPrint("tvert %d is used by vert %d and vert %d (face %d)\n",TexFaces[i].t[j], tvert_vert[TexFaces[i].t[j]], Faces[i].v[j],i);
							Degen_Faces.Append(1,&i);
							Degen_VertIdx.Append(1,&j);
						}
					}
					tvert_vert[TexFaces[i].t[j]] = Faces[i].v[j];
				}
			}

			IntTab orphan_tverts_tab;
			bool orphan_tverts =false;
			for(i=0;i<nTVerts;i++)
			{
				if(!tvert_used[i]) orphan_tverts_tab.Append(1,&i);
			}

			if(orphan_tverts_tab.Count())
			{
				//ms->orphan_tverts	=true;
				orphan_tverts	=true;
				DebugPrint("%d tverts are orphaned\n",orphan_tverts_tab.Count());
			}

			free(tvert_used);
			tvert_used=NULL;
		}


		if((misshared_tverts || orphan_tverts) && pb_options->GetInt(PB_WARN_ORPHAN_TV))
		{
			int res=CryMessageBox(hwnd_nodes, "Node's mesh has some degenerate texture coordinates\n"
				"Do you still want to export this node?", name, MB_YESNOCANCEL);
			switch (res)
			{
			case IDNO:
				return ID_SKIP_NODE;
			case IDCANCEL:
				return IDCANCEL;
			}
		}
	}

	//============================
	//Check multiple Vertex Colors
	//============================
	if(pb_nodes->GetInt(PB_WRITE_VCOL))
	{
		if(pb_options->GetInt(PB_WARN_MULTI_VCOL))
		{
			if(nCVerts && (nVerts != nCVerts))
			{
				int res=CryMessageBox(hwnd_nodes, "Node has multiple vertex colors per vertex.\n"
					"If you continue exporting, average of these vertex colors will be used for such vertices.\n\n"
					"Do you still want to continue exporting this node", name, MB_YESNOCANCEL);
				switch (res)
				{
				case IDNO:
					return ID_SKIP_NODE;
				case IDCANCEL:
					return IDCANCEL;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Check for Degenerate triangles.
	//////////////////////////////////////////////////////////////////////////
	{
		IntTab Degen_Faces;
		IntTab Degen_Vertices;
		for(int i=0;i<nFaces;i++)
		{
			if (Faces[i].v[0] == Faces[i].v[1] || Faces[i].v[0] == Faces[i].v[2] || Faces[i].v[1] == Faces[i].v[2])
			{
				// Bad triangle.
				Degen_Faces.Append(1,&i);
				int nDegenerateVertex = 0;
				Degen_Vertices.Append(1, &nDegenerateVertex);
			}
			Point3 p0 = Verts[Faces[i].v[0]];
			Point3 p1 = Verts[Faces[i].v[1]];
			Point3 p2 = Verts[Faces[i].v[2]];
			int nDegenerateVertex = -1;
			static const float sk_fFloatEpsilon = 0.008f;
			if (IsEqualPoint3Epsilon(p0,p1,sk_fFloatEpsilon))
				nDegenerateVertex = 0;
			if (IsEqualPoint3Epsilon(p0,p2,sk_fFloatEpsilon))
				nDegenerateVertex = 0;
			if (IsEqualPoint3Epsilon(p1,p2,sk_fFloatEpsilon))
				nDegenerateVertex = 1;
			if (nDegenerateVertex != -1)
			{
				Degen_Faces.Append(1,&i);
				Degen_Vertices.Append(1, &nDegenerateVertex);
			}
		}

		if (Degen_Faces.Count() > 0)
		{
			//char str[1024];
			//int nCount = Degen_Faces.Count();
			//sprintf( str, "Mesh in Node %s has %d degenerate Faces!\r\nCheck your model for errors.",name,nCount );
			//int res = CryMessageBox(hwnd_nodes, str,"Error",MB_OK|MB_ICONERROR);
			//return ID_SKIP_NODE;
			FaceReportDlg faceDlg;
			faceDlg.SetCaption("Degenerate Faces");
			std::ostringstream msg;
			msg << "Node " << name << " has " << Degen_Faces.Count() << " degenerate faces at the following locations:";
			faceDlg.SetMessage(msg.str());
			faceDlg.SetNode(node);
			Matrix3 mat = node->GetObjTMAfterWSM(time);
			for (int nFace = 0; nFace < Degen_Faces.Count(); ++nFace)
			{
				int nFaceIndex = Degen_Faces[nFace];
				int nVertexIndex = Faces[nFaceIndex].v[Degen_Vertices[nFace]];
				Point3 pos = Verts[nVertexIndex];
				pos = mat * pos;
				faceDlg.AddFace(nFaceIndex, pos.x, pos.y, pos.z);
			}
			faceDlg.DoModal(hwnd_nodes);
		}
	}



	return false;
}

// SaveVertAnim ______________________________________________________________________________________
bool CSExportUtility::SaveVertAnim(FILE* f, INode *node, IVertexAnimation* pVertexAnimation)
{
	Object *der_obj = node->GetObjectRef();
	if (!der_obj)
		return true;

	TimeValue	time	= GetTime();
	Object		*obj	= der_obj->Eval(time).obj;
	SClass_ID	sid		= obj->SuperClassID();
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		return false;

	TriObject	*tri	= (TriObject *) obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
	BOOL		needDel = (obj != tri);
	Mesh		*mesh	= &tri->mesh;
	if (!mesh)
		return true;
	
	//check if mesh is suitable
	int nVerts = pVertexAnimation->GetNumVerts();
	int nFaces = pVertexAnimation->GetNumFaces();

	//Get Parameters
	int	StartFrame = pb_timing->GetInt(PB_RANGE_START);
	int EndFrame = pb_timing->GetInt(PB_RANGE_END);
	int	SampleRate = pb_nodes->GetInt(PB_VERT_ANIM_RATE);

	//Calc # of keys
	int nKeys = 1+((EndFrame-StartFrame)/SampleRate);

	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id=ChunkList.GetID(node,ChunkType_VertAnim);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_VertAnim at pos %d)",ch_ent.FileOffset);

	VERTANIM_CHUNK_DESC chunk;
	memset(&chunk, 0, sizeof(chunk));
	chunk.chdr				= ch_ent;
	chunk.nFaces			= nFaces;
	chunk.nVerts			= nVerts;
	chunk.nKeys				= nKeys;
	chunk.GeomID			= ChunkList.GetID(node, ChunkType_Mesh);
	int res=fwrite(&chunk,sizeof(chunk),1,f);
	if (res!=1)
		return true;

	if (needDel)
		tri->DeleteThis();
	tri=NULL;
	mesh=NULL;

	for(int i=0;i<nKeys;i++)
	{
		UpdateProgress(100*i/nKeys,"Writing vertex animation keys", "");
		time = (StartFrame + i*SampleRate)*GetTicksPerFrame();

		IVertexAnimationFrame* pFrame = pVertexAnimation->GetFrame(time);

		//UpdateProgress(100*i/nKeys,"\"%s\" Writing vertex animation key %d", ActiveNode->GetName(),i);
		obj	= der_obj->Eval(time).obj;
		sid		= obj->SuperClassID();
		if(!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))  return false;
		tri		= (TriObject *) obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
		needDel = (obj != tri);
		mesh	= &tri->mesh;
		if (!mesh) return true;

		nVerts = pFrame->GetNumVertices();
		nFaces = pFrame->GetNumFaces();
		Face* Faces = pFrame->GetFaces();
		Point3* Verts = pFrame->GetVertices();

		if (nVerts != chunk.nVerts)
		{
			CryMessageBox(hwnd_nodes,"number of vertices of node \"%s\" changes in time\nVertex animation for objects with dynamic topologies can not be exported","Error",MB_OK);
			return true;
		}

		//=====================
		// Write Key Header
		//=====================
		KEY_HEADER kh;
		kh.KeyTime = time;
		int res=fwrite(&kh,sizeof(kh),1,f);
		if(res!=1) return true;

		//=====================
		// Write Vertices
		//=====================
		//calc vertex normals
		BOOL ConsiderSmooth = true;
		Tab<VNormal> vnorms;
		MeshUtils::CalcVertexNormals(vnorms, mesh, ConsiderSmooth);

		for(int j=0;j<nVerts;j++)
		{
			Point3 v = Verts[j];
			Point3 n = vnorms[j].GetNormal(1);

			CryVertex vert;
			vert.p.x=v.x; 
			vert.p.y=v.y; 
			vert.p.z=v.z;

			vert.n.x=n.x;
			vert.n.y=n.y;
			vert.n.z=n.z;
			res=fwrite(&vert,sizeof(vert),1,f);
			if(res!=1) return true;
		}

		if(needDel) tri->DeleteThis();
		tri=NULL; mesh=NULL;
		delete pFrame;
	}

	return false;
}
// SaveHelperNode ______________________________________________________________________________________
bool CSExportUtility::SaveHelperObject(FILE* f, IHelperObject* pHelperObject, INode *node)
{
	Object *der_obj = node->GetObjectRef();
	if(!der_obj) return true;

	TimeValue time=GetTime();
	Object *obj = der_obj->Eval(time).obj;
	
	HelperTypes type = pHelperObject->GetType();
	Vec3 size(0.0f, 0.0f, 0.0f);
	Class_ID cid=obj->ClassID();

	switch (type)
	{
	case HP_XREF:
		{
			Box3 box;
			obj->GetLocalBoundBox(time,node,0,box );
			Point3 p=box.Width();
			size.x = p.x;	size.y = p.y;	size.z = p.z;
		}
	case HP_DUMMY:
		{
			Box3 box=((DummyObject *)obj)->GetBox();
			Point3 p=box.Width();
			size.x = p.x;	size.y = p.y;	size.z = p.z;
		}
	default:
		break;
	}

	//==================
	//write chunk header
	//==================
	UpdateProgress(100,"Writing helper Object");
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id=ChunkList.GetID(node,ChunkType_Helper);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_Helper at pos %d)",ch_ent.FileOffset);
	
	HELPER_CHUNK_DESC hcd;
	memset(&hcd, 0, sizeof(hcd));
	hcd.chdr = ch_ent;
	hcd.type = type;
	hcd.size = size;
	int res=fwrite(&hcd,sizeof(hcd),1,f);
	if(res!=1) return true;

	return false;
}

class ExportKeyHandler : public IControllerKeyHandler
{
public:
	ExportKeyHandler(FILE* pFile);

	virtual void LinearInterpolationFloat(CryLin1Key& key);
	virtual void LinearInterpolationPosition(CryLin3Key& key);
	virtual void LinearInterpolationScale(CryLin3Key& key);
	virtual void LinearInterpolationRotation(CryLinQKey& key);
	virtual void HybridInterpolationFloat(CryBez1Key& key);
	virtual void HybridInterpolationPosition(CryBez3Key& key);
	virtual void HybridInterpolationPoint3(CryBez3Key& key);
	virtual void HybridInterpolationScale(CryBez3Key& key);
	virtual void HybridInterpolationRotation(CryBezQKey& key);
	virtual void TCBInterpolationFloat(CryTCB1Key& key);
	virtual void TCBInterpolationPosition(CryTCB3Key& key);
	virtual void TCBInterpolationPoint3(CryTCB3Key& key);
	virtual void TCBInterpolationScale(CryTCB3Key& key);
	virtual void TCBInterpolationRotation(CryTCBQKey& key);

private:
	FILE* pFile;
};

ExportKeyHandler::ExportKeyHandler(FILE* pFile)
:	pFile(pFile)
{
}

void ExportKeyHandler::LinearInterpolationFloat(CryLin1Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::LinearInterpolationPosition(CryLin3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::LinearInterpolationScale(CryLin3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::LinearInterpolationRotation(CryLinQKey& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::HybridInterpolationFloat(CryBez1Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::HybridInterpolationPosition(CryBez3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::HybridInterpolationPoint3(CryBez3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::HybridInterpolationScale(CryBez3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::HybridInterpolationRotation(CryBezQKey& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::TCBInterpolationFloat(CryTCB1Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::TCBInterpolationPosition(CryTCB3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::TCBInterpolationPoint3(CryTCB3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::TCBInterpolationScale(CryTCB3Key& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

void ExportKeyHandler::TCBInterpolationRotation(CryTCBQKey& key)
{
	fwrite(&key, sizeof(key), 1, pFile);
}

bool CSExportUtility::SaveController(FILE* f, IController* pController)
{
	if (!pController) 
		return true;

	if (pController->KeyCount() == 0) 
		return false;
		
	TimeValue	time	= GetTime();
	int res;

	//==================
	//Write chunk header
	//==================
	UpdateProgress(100,"Writing controller object");
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id=ChunkList.GetID(pController ,ChunkType_Controller);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_Controller at pos %d)", ch_ent.FileOffset);

	CtrlTypes type = pController->GetType();

	if (type == CTRL_NONE) 
		return true;
	
	CONTROLLER_CHUNK_DESC_0826 chunk;
	memset(&chunk, 0, sizeof(chunk));
	chunk.chdr		= ch_ent;
	chunk.nKeys		= pController->KeyCount();
	chunk.type		= type;
	chunk.nFlags	= pController->GetFlags();
	chunk.nControllerId = chunk.chdr.ChunkID;

	res=fwrite(&chunk,sizeof(chunk),1,f); 

	if(res!=1) 
		return true;

	ExportKeyHandler keyHandler(f);
	pController->HandleKeys(&keyHandler);
	
	return false;
}

// SaveGeomNode ______________________________________________________________________________________
bool CSExportUtility::SavePatchObject(FILE* f, Object *obj)
{
	return false;
}

void CSExportUtility::SaveSourceInfo (IExportSourceInfo* pSourceInfo, FILE*f, int ChunkId, std::time_t exportTime)
{
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id = ChunkList.GetID(NULL, ChunkId);
	if(chunk_id == -1)
		return;
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	std::string sFilePath = pSourceInfo->GetFileName();
	fwrite(sFilePath.c_str(), sFilePath.length() + 1, 1, f);
	
	if (exportTime == -1)
		exportTime = pSourceInfo->GetExportTime();
	const char* szTime = asctime(localtime(&exportTime));
	fwrite (szTime, strlen(szTime)+1, 1, f);

	char szName[0x100];
	DWORD dwSize = sizeof(szName);
  if(!GetUserName (szName, &dwSize))
	{
		dwSize = 0;
		*szName = '\0';
	}
	fwrite (szName, strlen(szName), 1, f);
	fwrite ("@", 1,1,f);

	if (!GetComputerName (szName, &dwSize))
	{
		dwSize = 0;
		*szName = '\0';
	}
	fwrite (szName, strlen(szName)+1, 1, f);

	fgetpos(f,&fpos);
	// pad to have 4-byte-aligned data
	if (fpos&3)
	{
		long nPad = 0;
		fwrite (&nPad, 1, 4-(fpos&3), f);
	}
}

// SaveGeomNode ______________________________________________________________________________________
// bIsBoneMeshChunk - true if the bone chunk is being saved
bool CSExportUtility::SaveMeshObject(FILE* f, AssetWriter& writer, IExportFlags* pExportFlags, ISkeleton* pSkeleton, ISourceMesh* pMesh, ISourceMaterial* pMaterial, const Matrix34& transform, const Matrix34& objectOffset, const std::string& sName, int nChunkID, int nVertexAnimChunkID, unsigned nFlags)
{
	if (pMesh == 0)
		return false;

	BOOL HasBoneInfo = pExportFlags->ShouldWriteWeights() && ((nFlags&(nSMF_DontSaveBoneInfo|nSMF_IsBoneMeshChunk))==0);
	if (pMesh->GetSkinningInfo() == 0)
		HasBoneInfo = false;

	BOOL WriteVCol = pExportFlags->ShouldWriteVertexColours();
	BOOL AllowMultiUV = pExportFlags->ShouldAllowMultipleUVs();
	// we don't want to generate default UVs for bones

	//turn off vertex color export if mesh do not have them - unless we are being forced
	// to write out vertex colours.
	if (!(nFlags & nSMF_ForceSaveVertexColours) && !(pMesh->HasVertexColours() || pMesh->HasVertexAlpha()))
		WriteVCol=FALSE;

	//Check if MultiUV per vertex
	if((!AllowMultiUV) && pMesh->UVCount() && (pMesh->UVCount() != pMesh->VertexCount()))
	{
		CryMessageBox(hwnd_nodes,"There are multiple UV coordinates per vertex\nEither check your mapping or turn on 'Allow Multi UV/vert' option", sName.c_str(), MB_OK);
		return true;
	}

	//==================
	//Write chunk header
	//==================
	UpdateProgress(100,"\"%s\" Writing chunk header", sName.c_str());
	fpos_t fpos;
	fgetpos(f,&fpos);

	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(nChunkID));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_Mesh at pos %d)",ch_ent.FileOffset);

	MESH_CHUNK_DESC chunk;
	memset(&chunk, 0, sizeof(chunk));
	chunk.chdr				= ch_ent;
	chunk.flags1 = 0;
	chunk.flags2 = 0;
	chunk.flags1 |= (HasBoneInfo)? MESH_CHUNK_DESC::FLAG1_BONE_INFO : 0;
	chunk.flags2 |= (WriteVCol && pMesh->HasVertexColours()) ? MESH_CHUNK_DESC::FLAG2_HAS_VERTEX_COLOR : 0;
	chunk.flags2 |= (WriteVCol && pMesh->HasVertexAlpha()) ? MESH_CHUNK_DESC::FLAG2_HAS_VERTEX_ALPHA : 0;
	chunk.nFaces			= pMesh->FaceCount();
	chunk.nTVerts			= pMesh->UVCount();
	chunk.nVerts			= pMesh->VertexCount();
	chunk.VertAnimID		= nVertexAnimChunkID;

	chunk.nFaces = pMesh->FaceCount();
	chunk.nVerts = pMesh->GetVertMapSize();

	int res=fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1) 
		return true;

	//======================
	//Write vertex positions
	//======================

	UpdateProgress(60, "\"%s\" Writing optimized coordinates", sName.c_str());
	for (int i = 0; i < pMesh->GetVertMapSize(); ++i)
	{
		res = fwrite(&pMesh->GetVertices()[pMesh->GetVertMap()[i]], sizeof(CryVertex), 1, f);
		if (res != 1)
			return true;
	}

	UpdateProgress(65, "\"%s\" Writing optimized faces", sName.c_str());
	res = fwrite(pMesh->GetFaces(), sizeof(pMesh->GetFaces()[0]), pMesh->FaceCount(), f);
	if (res != pMesh->FaceCount())
		return true;

	if (pMesh->UVCount())
	{
		// write to the disk
		UpdateProgress (75, "\"%s\" Writing texture vertices", sName.c_str());
		res = fwrite(pMesh->GetUVs(), sizeof(pMesh->GetUVs()[0]), pMesh->UVCount(), f);
		if (res != pMesh->UVCount())
			return true;

		UpdateProgress (85, "\"%s\" Writing texture faces", sName.c_str());
		res = fwrite(pMesh->GetTextureFaces(), sizeof(pMesh->GetTextureFaces()[0]), pMesh->TextureFaceCount(), f);
		if (res != pMesh->TextureFaceCount())
			return true;
	}

	if (HasBoneInfo)
	{
		UpdateProgress(90, "\"%s\" Writing bone info", sName.c_str());
		//we are sure that only "one" node uses this derived_object
		ISkinningInfo* pSkinningInfo = pMesh->GetSkinningInfo();
		if (pSkinningInfo != 0)
		{
			DebugPrint("\nsaving physique info for \"%s\"",sName.c_str());

			if (writer.SaveBoneInfo(pSkeleton, pSkinningInfo, transform, sName))
				return true;
			size_t pos = ftell(f);
			pos = pos;
		}
	}

	if (WriteVCol)
	{
		UpdateProgress(100,"\"%s\" Writing Vertex Colors", sName.c_str());

		//===================
		//Write Vertex Colors
		//===================
		if (pMesh->HasVertexColours())
		{
			for (i = 0; i < pMesh->GetVertMapSize(); ++i)
			{
				res = fwrite(pMesh->GetVertexColours() + pMesh->GetVertMap()[i], sizeof(CryIRGB), 1, f);
				if (res != 1)
					return true;
			}
		}

		if (pMesh->HasVertexAlpha())
		{
			for (i = 0; i < pMesh->GetVertMapSize(); ++i)
			{
				res = fwrite(pMesh->GetVertexAlpha() + pMesh->GetVertMap()[i], sizeof(unsigned char), 1, f);
				if (res != 1)
					return true;
			}
		}
	}

	if (nFlags & nSMF_SaveMorphTargets)
	{
		Vec3 morphOffset = transform.GetTranslation();

		// If we export individual files, we should zero out the node position.
		// Note I don't think this will work for hierarchies so well, but that will be fiddly to solve.
		// If that need comes up, we should refactor the chunks so that morphs are exported as deltas rather
		// than absolute positions - would be much cleaner.
		if (!pb_nodes->GetInt(PB_INDIVIDUAL_FILES))
			morphOffset = Vec3(ZERO);

		if (SaveMorphTargets(f, pMesh, objectOffset, pMesh->GetVertexForBoneSavingRoutinesMapSize(), pMesh->GetVertsForBoneSavingRoutinesMap(), ch_ent.ChunkID, morphOffset))
			return true; // error
	}

	if ((nFlags & nSMF_SaveInitBonePos) && HasBoneInfo)
	{
		if (writer.SaveBoneInitialPos(pMesh->GetSkinningInfo(), pSkeleton, ch_ent.ChunkID, &ChunkList))
			return true; // error
	}

	return false; // success, no error
} 

bool CSExportUtility::SaveMaterialName(FILE* f, ISourceMaterial* pMaterial, SChunkInfo *pChunkInfo)
{
	if (!pMaterial)
		return false;

	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	//append to chunk list
	int chunk_id=ChunkList.GetID(pMaterial, ChunkType_MtlName);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_MtlName at pos %d)",ch_ent.FileOffset);

	bool bMultiMat = (pMaterial->GetSubMaterials() != 0);

	MTL_NAME_CHUNK_DESC_EXPORTER chunk;
	memset(&chunk,0,sizeof(chunk));
	chunk.chdr = ch_ent;

	if (pMaterial->GetName().length() >= sizeof(chunk.name))
	{
		char str[1024];
		sprintf(str, "Material name %s is too long, must be no more then 128 characters.", pMaterial->GetName().c_str());
		MessageBox( NULL,str,"Warning!!!",MB_OK );
	}
	strncpy(chunk.name, pMaterial->GetName().c_str(), sizeof(chunk.name));
	chunk.name[sizeof(chunk.name)-1] = 0;

	chunk.nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;

	CrytekShader* pCryShader = pMaterial->GetCrytekShader();
	if (pCryShader != 0)
	{
		if (pCryShader->IsPhysicalize())
		{
			chunk.nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;
			const TCHAR* surfName = pCryShader->GetSurfaceName();
			if (stricmp(surfName,"Default") == 0)
				chunk.nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;
			else if (stricmp(surfName,"Physical Proxy (NoDraw)") == 0)
			{
				chunk.nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT_PROXY;
			}
			else if (stricmp(surfName,"No Collide") == 0)
			{
				chunk.nPhysicalizeType = PHYS_GEOM_TYPE_NO_COLLIDE;
			}
			else if (stricmp(surfName,"Obstruct") == 0)
			{
				chunk.nPhysicalizeType = PHYS_GEOM_TYPE_OBSTRUCT;
			}
		}
		else
		{
			chunk.nPhysicalizeType = PHYS_GEOM_TYPE_NONE;
		}
		if (pCryShader->IsSH())
			chunk.nFlags |= MTL_NAME_CHUNK_DESC_0800::FLAG_SH_COEFFS;
		if (pCryShader->HasAmbientSH())
			chunk.nFlags |= MTL_NAME_CHUNK_DESC_0800::FLAG_SH_AMBIENT;
		if (pCryShader->IsSH2Sided())
			chunk.nFlags |= MTL_NAME_CHUNK_DESC_0800::FLAG_SH_2SIDED;
		chunk.sh_opacity = min(max(pCryShader->GetSHOpacity(GetTime()), 0.0f),1.0f);
	}
	else
	{
		chunk.sh_opacity = 0.f;//ignore material for sh processing
	}

	if (pChunkInfo->flags & CHUNK_FLAG_SUB_MATERIAL)
	{
		chunk.nFlags |= MTL_NAME_CHUNK_DESC_EXPORTER::FLAG_SUB_MATERIAL;
	}
	else if (bMultiMat)
	{
		chunk.nFlags |= MTL_NAME_CHUNK_DESC_EXPORTER::FLAG_MULTI_MATERIAL;
		int nNumSubMaterials = 0;
		for (int i = 0; i < min(pMaterial->GetSubMaterials()->Count(), MAX_SUB_MATERIALS); i++)
		{
			ISourceMaterial* pSubMaterial = pMaterial->GetSubMaterials()->Get(i);
			if (pSubMaterial)
			{
				chunk.nSubMatChunkId[nNumSubMaterials] = ChunkList.GetID(pSubMaterial, ChunkType_MtlName);
				++nNumSubMaterials;
			}
		}
		chunk.nSubMaterials = nNumSubMaterials;
	}

	int res = fwrite(&chunk,sizeof(chunk),1,f);
	if (res!=1)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSExportUtility::SaveSceneProps(FILE* f)
{
	//=======================
	//Write SceneProperties
	//=======================
	int numProps = ip->GetNumProperties(PROPSET_USERDEFINED);
	if(numProps)
	{
		//==================
		//Write chunk header
		//==================
		fpos_t fpos;
		fgetpos(f,&fpos);
		//append to chunk list
		int chunk_id=ChunkList.GetID(NULL,ChunkType_SceneProps);
		assert(chunk_id != -1);
		CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
		ch_ent.FileOffset	 = int(fpos);
		DebugPrint("\n(ChunkType_SceneProps at pos %d)",ch_ent.FileOffset);


		SCENEPROPS_CHUNK_DESC chunk;
		memset(&chunk, 0, sizeof(chunk));
		chunk.chdr				= ch_ent;
		chunk.nProps			= numProps;
		int res=fwrite(&chunk,sizeof(chunk),1,f);
		if(res!=1) return true;

		//==================
		//Write bone names
		//==================
		for (int i=0; i<numProps; i++) 
		{
			UpdateProgress(100*i/numProps,"Writing SeceneProps...");
			const PROPSPEC*		pPropSpec	= ip->GetPropertySpec(PROPSET_USERDEFINED, i);
			const PROPVARIANT*	pPropVar	= ip->GetPropertyVariant(PROPSET_USERDEFINED, i);

			SCENEPROP_ENTITY item;
			if (pPropSpec->ulKind == PRSPEC_LPWSTR) //we support only the string
			{
				//_tcscpy(item.name, TSTR(pPropSpec->lpwstr));
				std::copy(pPropSpec->lpwstr, pPropSpec->lpwstr + wcslen(pPropSpec->lpwstr), item.name);
				VariantToString(pPropVar, item.value, sizeof(item.value));
			}
			else
			{
				_tcscpy(item.name ,"<<ERROR>>");
				_tcscpy(item.value, "Invalid Property Type");
			}
			int res=fwrite(&item,sizeof(item),1,f);
			if(res!=1) return true;
		}
	}

	return false;
}

bool CSExportUtility::SaveSceneProps(FILE* f, ISceneProperties* pSceneProperties)
{
	//=======================
	//Write SceneProperties
	//=======================
	int numProps = pSceneProperties->PropertyCount();
	if (numProps)
	{
		//==================
		//Write chunk header
		//==================
		fpos_t fpos;
		fgetpos(f,&fpos);
		//append to chunk list
		int chunk_id=ChunkList.GetID(NULL,ChunkType_SceneProps);
		assert(chunk_id != -1);
		CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
		ch_ent.FileOffset	 = int(fpos);
		DebugPrint("\n(ChunkType_SceneProps at pos %d)",ch_ent.FileOffset);


		SCENEPROPS_CHUNK_DESC chunk;
		memset(&chunk, 0, sizeof(chunk));
		chunk.chdr				= ch_ent;
		chunk.nProps			= numProps;
		int res=fwrite(&chunk,sizeof(chunk),1,f);
		if (res!=1)
			return true;

		//==================
		//Write bone names
		//==================
		for (int i = 0; i < numProps; i++) 
		{
			UpdateProgress(100*i/numProps,"Writing SeceneProps...");

			SCENEPROP_ENTITY property;
			std::string sName;
			std::string sValue;
			pSceneProperties->GetProperty(i, sName, sValue);
			strncpy(property.name, sName.c_str(), sizeof(property.name));
			strncpy(property.value, sValue.c_str(), sizeof(property.value));

			int res = fwrite(&property, sizeof(property), 1, f);
			if (res!=1)
				return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSExportUtility::SaveExportFlagsChunk(FILE* f, IExportFlags* pExportFlags)
{
	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id = ChunkList.GetID(NULL,ChunkType_ExportFlags);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	
	EXPORT_FLAGS_CHUNK_DESC chunk;
	memset(&chunk,0,sizeof(chunk));
	chunk.chdr				= ch_ent;
	chunk.flags = 0;
	if (pExportFlags->ShouldMergeObjects() && bCanMergeNodes)
	{
		chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::MERGE_ALL_NODES;
	};

	int res = fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSExportUtility::SaveBreakablePhysics(FILE* f, IBreakablePhysicsInfo* pBreakablePhysicsInfo)
{
	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id = ChunkList.GetID(NULL,ChunkType_BreakablePhysics);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);

	BREAKABLE_PHYSICS_CHUNK_DESC chunk;
	memset(&chunk,0,sizeof(chunk));
	chunk.chdr				= ch_ent;

	chunk.granularity = pBreakablePhysicsInfo->GetGranularity();
	chunk.nMode = 0;

	int res = fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSExportUtility::SaveTiming(FILE* f, const Interval* pRange)
{
	//=======================
	//Write TimeRanges
	//=======================
	int anim_start;
	int anim_end;

	if (pRange != 0)
	{
		anim_start = pRange->Start() / GetTicksPerFrame();
		anim_end = pRange->End()/ GetTicksPerFrame();
	}
	else
	{
		int	manualrange = pb_timing->GetInt(PB_MAN_RANGE);

		if(manualrange)
		{
			anim_start	= pb_timing->GetInt(PB_RANGE_START);
			anim_end	= pb_timing->GetInt(PB_RANGE_END);
		}
		else
		{
			Interval ii =ip->GetAnimRange();
			anim_start	=ii.Start() / GetTicksPerFrame();
			anim_end	=ii.End()   / GetTicksPerFrame();
		}
	}

	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	//append to chunk list
	int chunk_id=ChunkList.GetID(NULL,ChunkType_Timing);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_Timing at pos %d)",ch_ent.FileOffset);

	TIMING_CHUNK_DESC chunk;
	memset(&chunk, 0, sizeof(chunk));
	chunk.chdr					= ch_ent;
	chunk.global_range.start	= anim_start;
	chunk.global_range.end		= anim_end;
	chunk.SecsPerTick			= TicksToSec(1);
	chunk.TicksPerFrame			= GetTicksPerFrame();
	strcpy(chunk.global_range.name,"GlobalRange");
	chunk.nSubRanges = 0;

	chunk.nSubRanges			= 0;

	int res=fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1) return true;

	return false;
}

bool CSExportUtility::SaveBoneNameList(FILE* f, ISkeleton* pSkeleton)
{
	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	//append to chunk list
	int chunk_id=ChunkList.GetID(NULL,ChunkType_BoneNameList);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_BoneNameList at pos %d)",ch_ent.FileOffset);


	UpdateProgress(100,"Writing bone name list");

	BONENAMELIST_CHUNK_DESC_0745 chunk;
	chunk.numEntities = pSkeleton->GetBones()->Count();
	int res=fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1) return true;

	//==================
	//Write bone names
	//==================
	for (int i = 0; i < chunk.numEntities; ++i)
	{
		const std::string strName = pSkeleton->GetBones()->Get(i)->GetBoneName();
		int res = fwrite (strName.c_str(),strName.length()+1,1,f);
		if(res!=1) return true;
	}
	// write the terminating double-\0
	res = fwrite ("\0", 1, 1, f);
	if (res != 1) return true;

	// move the file pointer position so that it's 4-byte aligned
	fgetpos(f,&fpos);
	unsigned long nPad = 0; // pad bytes

	if (fpos & 3)
	{
		// unaligned pos
		fwrite (&nPad, 1, size_t(4- (fpos&3)), f);
	}

	return false;
}

inline Vec3& operator << (Vec3& left, Point3& right)
{
	left.x = right.x;
	left.y = right.y;
	left.z = right.z;
	return left;
}

bool CSExportUtility::SaveNode (FILE* f, ISourceObject* pSourceObject)
{
	INode* node = pSourceObject->GetMaxNode();

	if(!node) return true;
	char nodeName[1024];
	strcpy( nodeName,node->GetName() );

	TimeValue time=GetTime();

	{
		Object *obj = node->GetObjectRef();
		if (obj)
		{
			//obj = obj->Eval(time).obj;
			if (obj->SuperClassID()==SYSTEM_CLASS_ID && obj->ClassID()==Class_ID(XREFOBJ_CLASS_ID))
			{
				// Copy external xref object name to helpers node name.
				IXRefObject *ix = (IXRefObject *)obj;
				strcat( nodeName,"/" );
				strcat( nodeName,ix->GetObjName() );
			}
		}
	}

	// PETAR: Pivot matrix ignored ??
	Matrix3 pivot;
	Point3 pivotpos = node->GetObjOffsetPos();
	Quat pivotrot = node->GetObjOffsetRot();
	ScaleValue pivotscale = node->GetObjOffsetScale();

	pivot.IdentityMatrix();
	pivot.PreTranslate(pivotpos);
	PreRotateMatrix(pivot,pivotrot);
	ApplyScaling(pivot,pivotscale);

	// END PETAR ----------------------

	// determine whether we export the parent of this node; if we don't, then export it in the world
	// coordinate frame (if the corresponding fixup option is set)
	int nParentChunkId = -1;
	if (pSourceObject->GetParent() != 0)
		nParentChunkId = ChunkList.GetID(pSourceObject->GetParent(), ChunkType_Node);

	Matrix3 rtm; // relative to parent
	/*
	//Get Transform Matrix44
	Matrix3 atm=node->GetNodeTM(time);  // world TM
	atm=pivot*atm; // PETAR - pivot ignored
	Matrix3 ptm=node->GetParentNode()->GetNodeTM(time); // parent world tm

	rtm = atm*Inverse(ptm);
	*/

		// Node Local TM.
	rtm = node->GetNodeTM(time) * Inverse(node->GetParentTM(time));

	if (pb_nodes->GetInt (PB_INDIVIDUAL_FILES) && nParentChunkId == -1)
	{
		// the top of the hierarchy is always exported non-displaced
		rtm.SetRow(3,Point3(0,0,0));
	}
/*
	if (pb_nodes->GetInt (PB_ROTATE_TO_WORLD_CS))
	{
		if (nParentChunkId == -1)
			// rotate the matrix so that it's the same as in world coordinates
			for (int i = 0; i < 3; ++i)
				rtm.SetRow(i, atm.GetRow(i));
	}
*/
	Point3	decomposed_p;
	Quat	decomposed_r;
	Point3	decomposed_s;
	DecomposeMatrix(rtm, decomposed_p, decomposed_r, decomposed_s);
	//MRow *rrow =atm.GetAddr();
	//AffineParts rap;
	//decomp_affine(rtm,&rap);

	//CHECK CONTROLLERS
	int pos_cont_id=-1, rot_cont_id=-1, scl_cont_id=-1;
	if (pSourceObject->GetPositionController() != 0)
		pos_cont_id = ChunkList.GetID(pSourceObject->GetPositionController(), ChunkType_Controller);
	if (pSourceObject->GetRotationController() != 0)
		rot_cont_id = ChunkList.GetID(pSourceObject->GetRotationController(), ChunkType_Controller);
	if (pSourceObject->GetScaleController() != 0)
		scl_cont_id = ChunkList.GetID(pSourceObject->GetScaleController(), ChunkType_Controller);

	//==================
	//Write chunk header
	//==================
	fpos_t fpos;
	fgetpos(f,&fpos);
	//append to chunk list
	int chunk_id=ChunkList.GetID(pSourceObject,ChunkType_Node);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	DebugPrint("\n(ChunkType_Node at pos %d for \"%s\")",ch_ent.FileOffset, node->GetName());


	UpdateProgress(100,"\"%s\" Writing node chunk", nodeName);

	//Get # of children that will be saved
	IntTab children;
	for(int i=0;i<node->NumberOfChildren();i++)
	{
		INode *child_node=node->GetChildNode(i);
		int childchunkID=ChunkList.GetID(child_node,ChunkType_Node);
		if(childchunkID !=-1) children.Append(1,&childchunkID);
	}

	TSTR PropStr; 
	node->GetUserPropBuffer(PropStr);

	NODE_CHUNK_DESC ncd;
	memset(&ncd, 0, sizeof(ncd));
	ncd.chdr				= ch_ent;
	strncpy(ncd.name,nodeName,64);
	ncd.MatID				= ChunkList.GetID(pSourceObject->GetMaterial(), ChunkType_MtlName);
	ncd.nChildren			= children.Count();
	ncd.ObjectID			= ChunkList.GetID_Xept_type(node, ChunkType_Node);
	ncd.ParentID			= nParentChunkId;

	ncd.IsGroupHead			= node->IsGroupHead()?true:false;
	ncd.IsGroupMember		= node->IsGroupMember()?true:false;

	Matrix3_to_CryMatrix(ncd.tm, rtm);
	ncd.pos_cont_id			= pos_cont_id;
	ncd.rot_cont_id			= rot_cont_id;
	ncd.scl_cont_id			= scl_cont_id;
	ncd.pos					= Vec3(decomposed_p.x, decomposed_p.y, decomposed_p.z);
	ncd.rot					= CryQuat(decomposed_r.w,decomposed_r.x, decomposed_r.y, decomposed_r.z).GetNormalized();
	ncd.scl					= Vec3(decomposed_s.x, decomposed_s.y, decomposed_s.z);
	ncd.PropStrLen			= PropStr.Length();

	//if (fabs((ncd.rot|ncd.rot)-1) > 0.01)
//		ncd.rot = CryQuat (Matrix33(ncd.tm));

	int res=fwrite(&ncd,sizeof(ncd),1,f);
	if(res!=1) return true;
	
	//================
	//Write PropString
	//================
	UpdateProgress(100,"\"%s\" Writing properties", nodeName);
	if(ncd.PropStrLen)
	{
		int res=fwrite(PropStr.data(),PropStr.Length(),1,f);
		if(res!=1) return true;
		int nPad = 0;
		// align
		if (PropStr.Length() & 3)
		{
			res = fwrite (&nPad, 4 - (PropStr.Length() & 3),1,f);
			if (res != 1) return true;
		}
	}

	//======================
	//Write Child Chunk ID's
	//======================
	UpdateProgress(100,"\"%s\" Writing children", nodeName);
	if(ncd.nChildren)
	{
		for(int i=0;i<ncd.nChildren;i++)
		{
			int childchunkid=children[i];
			int res=fwrite(&childchunkid,sizeof(childchunkid),1,f);
			if(res!=1) return true;					
		}
	}

	return false;
};

////////////////////////////////////////////////////////////////////////////////////////////
// CSExportUtility::GetBoneNodeKeys
//
// Given the list of key times, generate linear BoneKey's for all those times.
// The generated linear keys go to the keys table.
// The keys may be optimized, in which case the keys table will contain less keys than there are positions in the keytime table.
// The optimization is a simple linear reduction, no tricks.
//
// PARAEMTERS:
//   pNode         - the node for which to generate the keys
//   tabKeyTimes   - array of times at which to generate keys.
//   tabOutKeys [OUTPUT] - upon output contains an array of keys defining the animation
//              NOTE: the keys may be optimized
// ASSUMES:
//   tabKeyTimes must be ordered and must not contain any duplicates
////////////////////////////////////////////////////////////////////////////////////////////
void CSExportUtility::GetBoneNodeKeys(INode *pNode, const IntTab &tabKeyTimes, Tab<BoneKey> &tabOutKeys)
{
	tabOutKeys.SetCount(tabKeyTimes.Count());

	for(int i=0; i<tabKeyTimes.Count(); i++)
	{
		assert (!i || tabKeyTimes[i] > tabKeyTimes[i-1]);

		UpdateProgress(100*i/tabKeyTimes.Count(),"\"%s\" Preparing bone keys %d/%d", pNode->GetName(), i, tabKeyTimes.Count());

		// the time of the key
		TimeValue time = tabKeyTimes[i];

		Matrix3 atm = pNode->GetNodeTM(time);
		Matrix3 ptm = pNode->GetParentNode()->GetNodeTM(time);
		
		atm.NoScale();
		ptm.NoScale();
		Matrix3 rtm = atm*Inverse(ptm);

		BoneKey* pKey = tabOutKeys.Addr(i);

		pKey->time = time;

		// Decompose and inner access to abs and rel matrices
		AffineParts aap,rap;
		decomp_affine (atm,&aap);
		decomp_affine (rtm,&rap);

		pKey->abspos.x = aap.t.x;
		pKey->abspos.y = aap.t.y;
		pKey->abspos.z = aap.t.z;

		pKey->relpos.x = rap.t.x;
		pKey->relpos.y = rap.t.y;
		pKey->relpos.z = rap.t.z;

		pKey->absquat.v.x = aap.q.x;
		pKey->absquat.v.y = aap.q.y;
		pKey->absquat.v.z = aap.q.z;
		pKey->absquat.w   = aap.q.w;

		pKey->relquat.v.x = rap.q.x;
		pKey->relquat.v.y = rap.q.y;
		pKey->relquat.v.z = rap.q.z;
		pKey->relquat.w   = rap.q.w;
	}
}

// GetValidChildrenList ______________________________________________________________________________________
bool CSExportUtility::GetValidChildrenList(INode *node, INodeTab &children, bool recurse)
{
	NodeUtils::ValidChildrenListKeepDummySetting eKeepDummy = (pb_bones->GetInt(PB_IGNORE_DUMMY) ? NodeUtils::ValidChildrenListIgnoreDummy : NodeUtils::ValidChildrenListKeepDummy);
	NodeUtils::ValidChildrenListSortSetting eSort = (pb_bones->GetInt(PB_SORT_BONES) ? NodeUtils::ValidChildrenListSort : NodeUtils::ValidChildrenListNoSort);
	NodeUtils::ValidChildrenListRecurseSetting eRecurse = (recurse ? NodeUtils::ValidChildrenListRecurse : NodeUtils::ValidChildrenListNoRecurse);

	std::vector<INode*> childVector;
	bool bResult = NodeUtils::GetValidChildrenList(node, childVector,	eRecurse, eKeepDummy, eSort);

	for (std::vector<INode*>::iterator itChild = childVector.begin(); itChild != childVector.end(); ++itChild)
	{
		INode* pChild = *itChild;
		children.Append(1, &pChild);
	}

	return bResult;
}

void AddKeyTimes(IntTab &a, IntTab &b)
{
	for(int i=0;i<b.Count();i++)
	{
		int val=b[i];
		a.Append(1,&val);
	}
}

float DotProd (const CryQuat& q, const CryQuat& p)
{
	return q.w*p.w + q.v*p.v;
}


// SaveBoneController ______________________________________________________________________________________
bool CSExportUtility::SaveBoneController (const Interval& range, FILE *f, INode *node)
{
	Interval ivalid;
	TimeValue time=GetTime();
	char *name = node->GetName();

	//----------------
	//Get Own Key times
	//----------------
	IntTab keytimes;
	keytimes.ZeroCount();
	int endtime = range.End();
	for (int t = range.Start(); t < endtime; t += GetTicksPerFrame())
		keytimes.Append(1, &t);
	keytimes.Append(1,&endtime);

	//------------
	//Get the keys
	//------------
	Tab<BoneKey> keys;
	GetBoneNodeKeys (node, keytimes, keys);

	//------------------------
	//Get 1st level child list
	//------------------------
	INodeTab children;
	children.ZeroCount();
	if (GetValidChildrenList(node, children, false))
		return true;

	//==================
	//Write chunk header
	//==================
	UpdateProgress(100,"\"%s\" Writing chunk header", name);
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id=ChunkList.GetID(node,ChunkType_Controller);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	
	DebugPrint(" (ChunkType_Controller at pos %d)",ch_ent.FileOffset);
	CONTROLLER_CHUNK_DESC_0827 chunk;
	chunk.numKeys				= keys.Count();
	chunk.nControllerId = Crc32Gen::GetCRC32( name );
	int	res=fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1) return true;

	// [MichaelS] Decide whether to export relative or absolute transforms.
	//
	// ==========
	// Back Story
	// ==========
	//
	// Ivo and Alexey introduced a special controller into the animations that they use to
	// describe the logical entity movement represented by the animation. This is therefore
	// used to control the entity position while the animation is playing. This must be in
	// world space, although the rest of the controllers are in parent space.
	//
	// The initial plan was for this controller to be outside the normal hierarchy. Doing this
	// worked fine and achieved the desired world space transforms. However, in order for the
	// controller to be saved in the BIP file, the controller must be part of the hierarchy,
	// so we need to force it to be in world space explicitly here. The RC currently looks for
	// the controller by name, so we are hard-coding the same name here (oooogh :().
	bool bExportAbsoluteKeyTransforms = (name && stricmp(name, "Locator_Locomotion") == 0);
	Vec3 BoneKey::*pPositionMember = (bExportAbsoluteKeyTransforms ? &BoneKey::abspos : &BoneKey::relpos);
	CryQuat BoneKey::*pQuatMember = (bExportAbsoluteKeyTransforms ? &BoneKey::absquat : &BoneKey::relquat);

	//==============
	//Write the keys
	//==============
	CryQuat qLast;
	qLast.SetIdentity();
	CryKeyPQLog keyLast;
	keyLast.nTime = 0;
	keyLast.vPos = Vec3(0,0,0);
	keyLast.vRotLog = Vec3(0,0,0);
	for(int i=0;i<keys.Count();i++)
	{
		if (DotProd(qLast,keys[i].relquat) >= 0)
			qLast =  (keys[i].*pQuatMember);
		else
			qLast = -(keys[i].*pQuatMember);

		CryKeyPQLog key;
		key.nTime = keys[i].time;
		key.vPos = (keys[i].*pPositionMember);

		key.vRotLog = log (qLast);
		AdjustRotLog (key.vRotLog, keyLast.vRotLog);

		keyLast = key;

		int res=fwrite(&key,sizeof(key),1,f);
		if(res!=1) return true;
	}
	
	//-------
	//recurse
	//-------
	for(int i=0;i<children.Count();i++)
	{
		int res;
		res=SaveBoneController(range, f, children[i]);
		
		if(res) return res?true:false;
	}
	return false;
}

bool CSExportUtility::SaveBoneHierarchy(FILE *f, ISkeleton* pSkeleton, INodeTab &allnodes)
{
	if(!allnodes.Count()) return false;

	UpdateProgress(100,"Writing bone anim header");

	//===========================
	//Write BoneAnim chunk header
	//===========================
	fpos_t fpos;
	fgetpos(f,&fpos);
	int chunk_id=ChunkList.GetID(allnodes[0],ChunkType_BoneAnim);
	assert(chunk_id != -1);
	CHUNK_HEADER &ch_ent = *(ChunkList.ch_list.Addr(chunk_id));
	ch_ent.FileOffset	 = int(fpos);
	
	BONEANIM_CHUNK_DESC chunk;
	memset(&chunk, 0, sizeof(chunk));
	chunk.chdr				= ch_ent;
	chunk.nBones			= allnodes.Count();
	int res=fwrite(&chunk,sizeof(chunk),1,f);
	if(res!=1) return true;

	int n=allnodes.Count(),i;
	for(i=0;i<n;i++)
	{
		char physname[64]; sprintf(physname,"%s Phys",allnodes[i]->GetName());
		INode *node = GetCOREInterface()->GetINodeByName(physname);
		if (node)
			break; // at least one bone has a " Phys" counterpart 
	}
	bool bAlwaysExportMesh = i==n; // if " Phys" conterparts are not used, assume that all bones have geometry

	std::set<unsigned int> controllerIds;

	//===============
	//Write The Bones
	//===============
	for(i=0;i<n;i++)
	{
		IBone* pBone = pSkeleton->GetBones()->Get(i);

		INode *node=allnodes[i];
		if(!node) continue;

		//INodeTab children;
		//if (GetValidChildrenList(node, children, false))
		//	return true;

		BONE_ENTITY bone;
		memset(&bone, 0, sizeof(bone));
		bone.BoneID			= BoneList.GetID(node,false);
    if (node->GetParentNode() != ip->GetRootNode() && !node->IsRootNode())
		  bone.ParentID = BoneList.GetID(node->GetParentNode(), false);
    else
      bone.ParentID = -1;

		//bone.ControllerID	= ChunkList.GetID(node,	ChunkType_Controller);
		int ctrlId = Crc32Gen::GetCRC32( node->GetName() );
		
		if (controllerIds.find(ctrlId) != controllerIds.end())
		{
			// Same Controller ID generated for different bones.
			char str[1024];
			sprintf( str,"Same Controller ID generated for different bones\r\nChange names of colliding bones %s.",(const char*)node->GetName() );
			MessageBox( NULL,str,"Warning!!!",MB_OK );
		}
		controllerIds.insert(ctrlId);
		bone.ControllerID	= ctrlId;
		bone.nChildren = pBone->GetChildren()->Count();
		bone.prop[0] = 0;

		bool bExportMesh = false;
		JointParams *jp;
		IPhysicsNode* pPhysicsNode = pBone->GetPhysicsNode();

		if (pPhysicsNode == NULL)
			bone.phys.flags = -1;
		else if (pPhysicsNode->GetJointParameters())
		{
			IJointParameters* pJointParameters = pPhysicsNode->GetJointParameters();
			jp = pJointParameters->GetMaxJointParams();

			bone.phys.flags = pJointParameters->GetFlags();
			for (int j=0; j < pJointParameters->DegreeOfFreedomCount(); j++)
			{
				IJointParameters::DegreeOfFreedomInfo dof;
				pJointParameters->GetDegreeOfFreedomInfo(j, dof);

				bone.phys.min[j] = dof.fMin;
				bone.phys.max[j] = dof.fMax;
				bone.phys.spring_angle[j] = dof.fSpringAngle;
				bone.phys.spring_tension[j] = dof.fSpringTension;
				bone.phys.damping[j] = dof.fDamping;
			}

			if (pPhysicsNode->GetPhysicsFrame() != 0)
				pPhysicsNode->GetPhysicsFrame()->ReadFrameMatrix(bone.phys.framemtx);
			else
				bone.phys.framemtx[0][0] = 100.0f;

			std::string sProperty = pPhysicsNode->GetUserProperty();
			strncpy(bone.prop, sProperty.c_str(), 32);
			bone.prop[31] = 0;
			bExportMesh = true;
		} 
		if (bAlwaysExportMesh && !bExportMesh)
		{
			if (!pBone->MarkedNonPhysical())
				bExportMesh = true;
		}

		if (bExportMesh) {
			CHUNK_HEADER new_ch_ent;
			new_ch_ent.ChunkType = ChunkType_BoneMesh;
			new_ch_ent.ChunkVersion	= MESH_CHUNK_DESC_VERSION;
			new_ch_ent.FileOffset = -1;
			ChunkList.Append(node,&new_ch_ent,true);
			bone.phys.nPhysGeom = ChunkList.GetID(node, ChunkType_BoneMesh);
		} else
			bone.phys.nPhysGeom = -1;

		if (bone.prop[0]==0) {
			strncpy(bone.prop,pBone->GetUserProperty().c_str(),32);
			bone.prop[sizeof(bone.prop)-1] = 0;
		}
		int res=fwrite(&bone,sizeof(bone)/*-sizeof(BONE_PHYSICS)*/,1,f);
		if(res!=1) return true;
	}

	fflush(f);

	return false;
}

bool CSExportUtility::CanExportBones()
{
	// Make a list of the bones - ignore any deleted ones.
	return pb_bones->Count(PB_BONES) > 0;
}

// SaveBoneAnimFile ______________________________________________________________________________________
int CSExportUtility::SaveBoneAnimFile(const std::string& sFileName)
{
	SaveConfigToMaxFile();

	if (!RunPreExportCallback())
		return IDNO;

	Interval ivalid;
	TimeValue time=GetTime();

	// Create an object that describes the export process.
	CSExportUtility::BoneAnimExportParameters exportParams;

	//===================================
	//check if there are bones to export
	//===================================
	// Make a list of the bones - ignore any deleted ones.
	for (int nBone = 0; nBone < pb_bones->Count(PB_BONES); ++nBone)
	{
		INode* pBone = pb_bones->GetINode(PB_BONES, time, nBone);
		if (pBone != 0) {
			TCHAR* name = pBone->GetName();

			if (name[0] != '_')
				exportParams.bones.push_back(pBone);
		}
	}

	// Make sure there are some bones to export.
	if (exportParams.bones.size() == 0)
	{
		CryMessageBox(hwnd_nodes,"Bone list is Empty\nPlease Add one or more bone to the list which will be exported","No bones to Export",MB_OK | MB_ICONEXCLAMATION);
		return IDCANCEL;
	}
	else
	{
		// Check whether the user wants us to export a single animation, or whether he wants to
		// export one CAF file per sub-range.
		bool bFilePerSubRange = pb_timing->GetInt(PB_EXPORT_FILE_PER_SUBRANGE) ? true : false;
		if (bFilePerSubRange)
		{
			// If no sub-ranges have been defined, then we can export nothing.
			if (Ranges.size() == 0)
			{
				CryMessageBox(hwnd_nodes, "No animation sub-ranges have been defined. Use the Edit Sub-Ranges button to define sub-ranges.", "No Sub-Ranges", MB_OK | MB_ICONEXCLAMATION);
			}
			else
			{
				std::string directory;

				// Check whether we should ask the user for a filename.
				if (bDontShowMessageBoxes)
					directory = PathUtil::GetPath(GetCOREInterface()->GetCurFilePath().data());
				else
					directory = BrowseForFolder(hwnd_nodes, "Select export directory:", PathUtil::GetPath(GetCOREInterface()->GetCurFilePath().data()).c_str());

				// Loop through all the sub-ranges defined in the dialog box.
				std::vector<std::string> filesWithErrors;
				for (int nSubRange = 0; nSubRange < Ranges.size(); ++nSubRange)
				{
					// Load the range for this animation.
					exportParams.range.SetStart(Ranges[nSubRange].nStart * GetTicksPerFrame());
					exportParams.range.SetEnd(Ranges[nSubRange].nEnd * GetTicksPerFrame());

					// Choose the filename for the animation.
					exportParams.sFilename = PathUtil::Make(directory, Ranges[nSubRange].sName, CAF_EXTENSION);

					// Export the file.
					int nExportResult = ExportBoneAnimToFile(exportParams);
					if (nExportResult != IDYES)
						filesWithErrors.push_back(exportParams.sFilename);
					else
					{
						// Run RC here!!!
						std::string sFile;

						PakSystem pakSystem;
						std::string cbaPath = CBAHelpers::FindCBAFileForFile(exportParams.sFilename, &pakSystem);
						if (cbaPath.empty())
						{
							CryMessageBox(hwnd_nodes, "Unable to find root CBA file - was searching upwards for a directory containing a file Animations/Animations.cba\n", "Cannot find CBA", MB_ICONERROR | MB_OK);
						}
						else
						{
							sFile = cbaPath;
							sFile += " /file=";

							if (exportParams.sFilename.find(" ") != -1)
								sFile += "\"" + exportParams.sFilename + "\"";
							else
								sFile += exportParams.sFilename;

							sFile += " /skipdba";

							RunResourceCompiler(sFile.c_str(), false, CREATE_NO_WINDOW);
						}
					}
				}

				// If there were any errors, report them to the user. Report only the first 5.
				if (filesWithErrors.size() > 0)
				{
					if (filesWithErrors.size() > 5)
					{
						filesWithErrors.resize(5);
						filesWithErrors.push_back("...");
					}

					std::ostringstream message;
					message << "There were errors when exporting the following files:\n";
					for (int i = 0; i < filesWithErrors.size(); ++i)
						message << filesWithErrors[i] << '\n';

					CryMessageBox(hwnd_nodes, message.str().c_str(), "Export Error", MB_ICONEXCLAMATION | MB_OK);
				}
			}
		}
		else
		{
			// Check whether we should ask the user for a filename.
			if (bDontShowMessageBoxes)
			{
				if (!sFileName.empty())
					exportParams.sFilename = sFileName;
				else
					exportParams.sFilename = PathUtil::Make(PathUtil::GetPath(GetCOREInterface()->GetCurFilePath().data()), FindDefaultBoneAnimExportFilename());
			}
			else
			{
				// Ask the user for a filename.
				char str[1024];
				char filename[1024];
				sprintf(str,"Save skeleton starting at bone \"%s\" As...",exportParams.bones[0]->GetName());
				int bSave = SaveFileDlg(FileType_Anim,filename, str, const_cast<char*>(FindDefaultBoneAnimExportFilename().c_str()));
				if(!bSave) return IDCANCEL;
				exportParams.sFilename = filename;
			}

			char szBuffer[1024];
			GetFullPathName(exportParams.sFilename.c_str(), sizeof(szBuffer), szBuffer, 0);

			// Select the animation range.
			int manualrange = pb_timing->GetInt(PB_MAN_RANGE);
			if(manualrange)
			{
				exportParams.range.SetStart(pb_timing->GetInt(PB_RANGE_START) * GetTicksPerFrame());
				exportParams.range.SetEnd(pb_timing->GetInt(PB_RANGE_END) * GetTicksPerFrame());
			}
			else
			{
				exportParams.range = ip->GetAnimRange();
			}

			// Export the file.
			int nRes = ExportBoneAnimToFile(exportParams);

			if (nRes == IDYES)
			{
				PakSystem pakSystem;
				std::string cbaPath = CBAHelpers::FindCBAFileForFile(exportParams.sFilename, &pakSystem);
				if (cbaPath.empty())
				{
					CryMessageBox(hwnd_nodes, "Unable to find root CBA file - was searching upwards for a directory containing a file Animations/Animations.cba\n", "Cannot find CBA", MB_ICONERROR | MB_OK);
				}
				else
				{
					// Run RC here!!!
					std::string sFile;

					sFile = cbaPath;
					//sFile += "\" /file= \"" + exportParams.sFilename;// + " /wait";
					sFile += " /file=";

					if (exportParams.sFilename.find(" ") != -1)
						sFile += "\"" + exportParams.sFilename + "\"";
					else
						sFile += exportParams.sFilename;

					sFile += " /skipdba";

					RunResourceCompiler(sFile.c_str(), false, CREATE_NO_WINDOW);
				}
			}

			return nRes;
		}
	}

	return IDYES;
}

// TODO: Get rid of this
void CSExportUtility::VariantToString(const PROPVARIANT* pProp, TCHAR* szString, int bufSize)
{
	switch (pProp->vt) 
	{
		case VT_LPWSTR:
			//_tcscpy(szString, TSTR(pProp->pwszVal));
			std::copy(pProp->pwszVal, pProp->pwszVal + wcslen(pProp->pwszVal), szString);
			break;
		case VT_LPSTR:
			_tcscpy(szString, TSTR(pProp->pszVal));
			break;
		case VT_I4:
			_stprintf(szString, "%ld", pProp->lVal);
			break;
		case VT_R4:
			_stprintf(szString, "%f", pProp->fltVal);
			break;
		case VT_R8:
			_stprintf(szString, "%lf", pProp->dblVal);
			break;
		case VT_BOOL:
			_stprintf(szString, "%d", pProp->boolVal ? 1  : 0);
			break;
		case VT_FILETIME:
			SYSTEMTIME sysTime;
			FileTimeToSystemTime(&pProp->filetime, &sysTime);
			GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &sysTime, NULL, szString, bufSize);
			break;
		default:
			_tcscpy(szString, "");	
			break;
	}
}

void CSExportUtility::RecursePrepareMtlChunks(ISourceMaterial* pMaterial)
{
	if(!pMaterial) return;

	CHUNK_HEADER ch_ent;
	ch_ent.FileOffset		= -1;
	ch_ent.ChunkType		= ChunkType_MtlName;
	//ch_ent.ChunkVersion		= MTL_CHUNK_DESC_EXPORTER::VERSION;
	ch_ent.ChunkVersion		= MTL_NAME_CHUNK_DESC_EXPORTER::VERSION;
	ChunkList.Append(pMaterial, &ch_ent,true);

	{
		bool bMultiMat = (pMaterial->GetSubMaterials() != 0);

		MTL_NAME_CHUNK_DESC_EXPORTER chunk;
		memset(&chunk,0,sizeof(chunk));
		chunk.chdr = ch_ent;
		strncpy(chunk.name, pMaterial->GetName().c_str(), sizeof(chunk.name));

		if (pMaterial->GetSubMaterials() != 0)
		{
			int nSubMaterials = min(pMaterial->GetSubMaterials()->Count(), MAX_SUB_MATERIALS);
			for (int i = 0; i < nSubMaterials; i++)
			{
				ISourceMaterial *submtl = pMaterial->GetSubMaterials()->Get(i);
				if (!submtl)
					continue;

				CHUNK_HEADER ch_ent;
				ch_ent.FileOffset		= -1;
				ch_ent.ChunkType		= ChunkType_MtlName;
				ch_ent.ChunkVersion		= MTL_NAME_CHUNK_DESC_EXPORTER::VERSION;
				ChunkList.Append(submtl, &ch_ent,true,CHUNK_FLAG_SUB_MATERIAL);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSExportUtility::PrepareAllBones( INodeTab &allBones )
{
	int i;
	for (i = 0; i < pb_bones->Count(PB_BONES); i++)
	{
		INode *rootbone = pb_bones->GetINode(PB_BONES,0,i);
		if(!rootbone)
			continue;

		allBones.Append(1,&rootbone);
		GetValidChildrenList(rootbone, allBones, true);
	}
	// Fill the Bonelist
	for(i=0;i<allBones.Count();i++)
	{
		INode *node = allBones[i];
		BoneList.GetID( node );
	}
}

//////////////////////////////////////////////////////////////////////////
int CSExportUtility::SaveColladaGeomFile()
{
	SaveConfigToMaxFile();

	extern ExportStubGUP* g_exportStub;
	if (!g_exportStub)
	{
		CryMessageBox(NULL,"MaxCryExport*.dll is not loaded.", "Export Error", MB_OK | MB_ICONEXCLAMATION);
		return IDNO;
	}

	std::vector<INode*> nodes;
	for (int i = 0; i < pb_nodes->Count(PB_NODES); ++i)
	{
		INode* const node = pb_nodes->GetINode(PB_NODES, 0, i);
		if (node)
		{
			nodes.push_back(node);
		}
	}

	if (nodes.empty())
	{
		MessageBox(hwnd_nodes,
			"Node list is Empty\n"
			"Please add one or more nodes to the list which will be exported",
			"No nodes to Export",
			MB_OK | MB_ICONEXCLAMATION);
		return IDCANCEL;
	}

	int const nExportType = pb_nodes->GetInt(PB_EXPORT_TO);
	if (nExportType != EXPORT_TO_CGF)
	{
		CryMessageBox(NULL,
			"This button ('Export nodes (COLLADA)') currently only works for static geometry (*.cgf). The file will not be exported.",
			"Export Error", 
			MB_OK | MB_ICONEXCLAMATION);
		return IDNO;
	}

	if (pb_nodes->GetInt(PB_USE_CUSTOM_FILENAME))
	{
		CryMessageBox(NULL,
			"'Custom Filename' must be disabled before exporting COLLADA.\n",
			"Export Error", 
			MB_OK | MB_ICONEXCLAMATION);
		return IDNO;
	}

	string parameters;
	{
		bool const bIndividualFiles = (pb_nodes->GetInt(PB_INDIVIDUAL_FILES) != 0);
		bool const bMergeObjects = (pb_nodes->GetInt(PB_MERGE_OBJECTS) != 0);
		PropertyHelpers::SetPropertyValue(parameters, "ExportFilePerNode", (bIndividualFiles ? "1" : "0"));
		PropertyHelpers::SetPropertyValue(parameters, "MergeAllNodes", (bMergeObjects ? "1" : "0"));
	}

	g_exportStub->DoExport(parameters.c_str(), &nodes[0], nodes.size());
}
	
// SaveGeomFile ______________________________________________________________________________________
int CSExportUtility::SaveGeomFile(std::time_t exportTime, bool bRunResourceCompiler)
{
	SaveConfigToMaxFile();

	ErrorReporter errorReporter(this);
	ErrorReporter* pErrorReporter = &errorReporter;

	//===================================
	//check if there are nodes to export
	//===================================
	std::vector<INode*> nodes;
	for (int i = 0; i < pb_nodes->Count(PB_NODES); ++i)
	{
		INode* node = pb_nodes->GetINode(PB_NODES, 0, i);
		if (!node)
			continue;
		nodes.push_back(node);
	}

	if (nodes.empty())
	{
		MessageBox(hwnd_nodes,"Node list is Empty\nPlease Add one or more nodes to the list which will be exported","No nodes to Export",MB_OK | MB_ICONEXCLAMATION);
		return IDCANCEL;
	}

	Interval ivalid;
	TimeValue time=GetTime();

	MaxExportSource* pExportSource = this->CreateExportDataSource(pErrorReporter, nodes);

	bBuilding = pb_nodes->GetInt(PB_BUILDING) != 0;
	bool bExportGeom = pb_nodes->GetInt(PB_EXPORT_GEOM) != 0;

	bool bIndividualFiles = pb_nodes->GetInt (PB_INDIVIDUAL_FILES) != 0;

	//=================
	//create cgf file
	//=================
	FILE *f = 0;

	// Pick a file name to export to.
	std::string sFilename;
	if (pb_nodes->GetInt(PB_USE_CUSTOM_FILENAME))
		sFilename = FindCustomExportFilename();
	else
		sFilename = FindDefaultExportFilename();

	bCanMergeNodes = false;
	int nExportType = pb_nodes->GetInt(PB_EXPORT_TO);
	switch (nExportType)
	{
	case EXPORT_TO_CGF:
		bCanMergeNodes = true; // Only static CGF`s can merge node meshes.
		break;
	case EXPORT_TO_CHR:

		// [9/11/2005 MichaelS] Allow multiple characters to be exported from
		// the one scene, assuming that they share the same skeleton.
		//bIndividualFiles = false;

		break;
	case EXPORT_TO_CGA:
		// [13/4/2007 MichaelS] Vehicles can now be exported both as normal and destroyed by linking all sub-nodes to one of the two
		// root nodes and exporting using file-per-node - therefore it now makes sense to allow this for CGAs.
		//bIndividualFiles = false;
		break;
	case EXPORT_TO_ANM:
		bIndividualFiles = false;
		bExportGeom = false;
		break;
	default:
		CryMessageBox( NULL,"Unknown Export Format","Export Error",MB_OK );
		return IDNO;
	}

	lastSavedFile = "";

	if (sFilename.empty()) 
	{
		return IDNO;
	}

	if (!bIndividualFiles && GetFileAttributes(sFilename.c_str()) != INVALID_FILE_ATTRIBUTES && (GetFileAttributes(sFilename.c_str()) & FILE_ATTRIBUTE_READONLY))
	{
		char text[1024];
		sprintf( text,"Overwrite Read-Only file %s?\r\nFile can be under source control.",sFilename.c_str() );
		if (CryMessageBox( NULL,text,"Read Only File",MB_YESNO|MB_ICONWARNING ) != IDYES)
			return IDNO;
		ResetReadOnlyFlag(sFilename.c_str());
	}

	// [5/12/2005 - MichaelS] Resource compiler cannot handle situation where multiple nodes are
	// being exported to a single CGF, and where some nodes have vertex colours and others don't.
	// Therefore if we detect this situation we force all nodes to have vertex colours, even if
	// none are defined in the node. If necessary we write out white for each vertex. This is only
	// necessary if the nodes will be merged into a single mesh by the RC. (see bug FPP-3216)
	bool bForceVertexColours = false;
	if (!bIndividualFiles && pb_nodes->GetInt(PB_WRITE_VCOL) && bCanMergeNodes)
	{
		// Check whether any node has vertex colours.
		bool bAnyNodeHasVertexColoursOrAlpha = false;
		for (int nNode = 0; nNode < pExportSource->GetObjects()->Count(); ++nNode)
		{
			TimeValue	time = GetTime();
			ISourceObject* pSourceObject = pExportSource->GetObjects()->Get(nNode);
			Object* pReferencedObject = pSourceObject->GetMaxNode()->GetObjectRef();
			Object* pObject	= pReferencedObject->Eval(time).obj;
			if (pObject->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
			{
				TriObject* pTriangleObject = (TriObject*)pObject->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
				Mesh* pMesh	= &pTriangleObject->mesh;

				bool bNodeHasVertexColours = true;
				if (!(pMesh->mapSupport(0) || pMesh->mapSupport(VDATA_ALPHA)) || pMesh->getNumVertCol() == 0)
					bNodeHasVertexColours = false;

				if (bNodeHasVertexColours)
					bAnyNodeHasVertexColoursOrAlpha = true;

				if (pTriangleObject != pObject)
					pTriangleObject->DeleteMe();
			}
		}

		// If any file has vertex colours, make sure all of them do.
		if (bAnyNodeHasVertexColoursOrAlpha)
			bForceVertexColours = true;
	}

	bool bAllowBlending = pb_nodes->GetInt(PB_LINK_TYPE, time);

	std::string sExportFilename = sFilename;
	for (int nFile = 0; nFile < (bIndividualFiles ? pExportSource->GetObjects()->Count() : 1); ++nFile)
	{
		ISourceObject* pSourceObjectInFile = pExportSource->GetObjects()->Get(nFile);

		if (bIndividualFiles)
		{
			// Only parent nodes exported as individual files.
			if (pSourceObjectInFile->GetParent())
				continue;

			sExportFilename = PathUtil::AddBackslash( PathUtil::GetPath(sFilename) ) + pSourceObjectInFile->GetName();
			sExportFilename = PathUtil::ReplaceExtension( sExportFilename,FindExportExtension() );

			CreateDirectoryForFile(sExportFilename.c_str());
			if (GetFileAttributes(sExportFilename.c_str()) != INVALID_FILE_ATTRIBUTES && (GetFileAttributes(sExportFilename.c_str()) & FILE_ATTRIBUTE_READONLY))
			{
				char text[1024];
				sprintf( text,"Overwrite Read-Only file %s?\r\nFile can be under source control.",sExportFilename.c_str() );
				if (CryMessageBox( NULL,text,"Read Only File",MB_YESNO|MB_ICONWARNING ) != IDYES)
					return IDNO;
				ResetReadOnlyFlag(sExportFilename.c_str());
			}
		}

		// We will write temporary file to prevent external file readers from reading half-written file

		string sTmpFilename;
		{
			string dir;
			string name;
			PathUtil::Split(sExportFilename, dir, name);
			sTmpFilename = dir;
			sTmpFilename += "$tmp$_";
			sTmpFilename += name;
		}

#define HALT(saved) { if(f) fclose(f); DeleteFile(sTmpFilename.c_str()); delete pExportSource; return saved; }

		f = _fsopen(sTmpFilename.c_str(), "wb", _SH_DENYRW);
		if (!f)
		{
			if (bIndividualFiles)
			{
				string msg = "Can't Export this file ";
				msg += sExportFilename;
				msg += "\r\nbecause can't write file ";
				msg += sTmpFilename;
				msg += "\r\nContinue?";
				if (MessageBox(hwnd_nodes,msg.c_str(),"Warning", MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
				{
					HALT(IDCANCEL);
				}
			}
			HALT(IDNO);
		}

		//==================
		//Write File Header
		//==================
		FILE_HEADER header;
		memset(&header, 0, sizeof(header));
		header.FileType				= FileType_Geom;
		header.ChunkTableOffset		= -1;
		header.Version				= ChunkFileVersion;
		strcpy(header.Signature,FILE_SIGNATURE);
		int res=fwrite(&header,sizeof(header),1,f);
		res=res!=1;
		if (res)
			HALT(IDNO);

		//=======================
		//pre-prepare all chunks
		//=======================
		BoneList.Reset();
		ChunkList.Reset();
		CHUNK_HEADER ch_ent;
		ch_ent.FileOffset		= -1;
		//=======================
		//Prepare the info chunk
		//=======================
		ch_ent.ChunkType = ChunkType_SourceInfo;
		ch_ent.ChunkID = 0;
		ch_ent.ChunkVersion = 0;
		ChunkList.Append(NULL, &ch_ent, true);

		//=======================
		//Prepare Export Flags.
		//=======================
		ch_ent.ChunkType		= ChunkType_ExportFlags;
		ch_ent.ChunkVersion		= EXPORT_FLAGS_CHUNK_DESC::VERSION;
		ChunkList.Append(NULL,&ch_ent,true);

		//=======================
		//Prepare SceneProperties
		//=======================
		int numProps = ip->GetNumProperties(PROPSET_USERDEFINED);
		if(numProps)
		{
			ch_ent.ChunkType		= ChunkType_SceneProps;
			ch_ent.ChunkVersion		= SCENEPROPS_CHUNK_DESC_VERSION;
			ChunkList.Append(NULL,&ch_ent,true);
		}
		//=======================
		//Prepare Timing
		//=======================
		ch_ent.ChunkType		= ChunkType_Timing;
		ch_ent.ChunkVersion		= TIMING_CHUNK_DESC_VERSION;
		ChunkList.Append(NULL,&ch_ent,true);

		for (i = 0; i<pExportSource->GetObjects()->Count(); i++)
		{
			ISourceObject* pSourceObject = pExportSource->GetObjects()->Get(i);
			if (bIndividualFiles)
			{
				if (pSourceObjectInFile != pSourceObject && !IsChildOf( pSourceObject,pSourceObjectInFile ))
					continue;
			}

			//Check for the possible problems that this node may have
			int res=CheckNodeProblems(pSourceObject);
			if (res==IDCANCEL)
				HALT(IDCANCEL);
			if (res==IDNO)
				HALT(IDNO);
			if (res==ID_SKIP_NODE)
				continue;

			//=======================
			//Prepare nodes
			//=======================
			ch_ent.ChunkType		= ChunkType_Node;
			ch_ent.ChunkVersion		= NODE_CHUNK_DESC_VERSION;
			ChunkList.Append(pSourceObject,&ch_ent,true);

			//=======================
			//Prepare Controllers
			//=======================
			ch_ent.ChunkType = ChunkType_Controller;
			ch_ent.ChunkVersion = CONTROLLER_CHUNK_DESC_0826::VERSION;
			if (pSourceObject->GetPositionController() && pSourceObject->GetPositionController()->KeyCount() > 0)
				ChunkList.Append(pSourceObject->GetPositionController(), &ch_ent, true);
			if (pSourceObject->GetRotationController() && pSourceObject->GetRotationController()->KeyCount() > 0)
				ChunkList.Append(pSourceObject->GetRotationController(), &ch_ent, true);
			if (pSourceObject->GetScaleController() && pSourceObject->GetScaleController()->KeyCount() > 0)
				ChunkList.Append(pSourceObject->GetScaleController(), &ch_ent, true);

			if (!bExportGeom)
			{
				continue;
			}

			//=======================
			//Prepare Materials
			//=======================
			if (pSourceObject->GetMaterial() != 0)
			{
				RecursePrepareMtlChunks(pSourceObject->GetMaterial());
			};

			//=======================
			//Prepare objects
			//=======================
			Object *der_obj	= pSourceObject->GetMaxNode()->GetObjectRef();

			if (!der_obj)
				continue;

			// Check for XRef.
			if (der_obj->SuperClassID()==SYSTEM_CLASS_ID && der_obj->ClassID()==Class_ID(XREFOBJ_CLASS_ID))
			{
				// This node is XRef Object.
				ch_ent.ChunkType	= ChunkType_Helper;
				ch_ent.ChunkVersion	= HELPER_CHUNK_DESC_VERSION;
			}
			else
			{
				// Not XRef object.
				Object *obj		= der_obj->Eval(time).obj;
				SClass_ID sid	= obj->SuperClassID();
				Class_ID  cid	= obj->ClassID();
				switch(sid)
				{
				case GEOMOBJECT_CLASS_ID:
					{
						ch_ent.ChunkType		= ChunkType_Mesh;
						ch_ent.ChunkVersion		= MESH_CHUNK_DESC_VERSION;
					}
					break;

				case CAMERA_CLASS_ID:
				case HELPER_CLASS_ID:
					ch_ent.ChunkType		= ChunkType_Helper;
					ch_ent.ChunkVersion		= HELPER_CHUNK_DESC_VERSION;
					break;

				default:
					sprintf(s, "Object \"%s\" cannot be exported since it is not of the supported types", pSourceObject->GetName().c_str());
					CryMessageBox(hwnd_nodes, s, "Unsupported object type", MB_OK | MB_ICONINFORMATION);
					continue;
				}
			}


			ch_ent.FileOffset = -1;
			res = ChunkList.Append(pSourceObject->GetMaxNode(), &ch_ent, true);

			if (res!=-1)
			{
				//=======================
				//Prepare Vertex Anim
				//=======================
				if (ch_ent.ChunkType == ChunkType_Mesh && pb_nodes->GetInt(PB_WRITE_VA))
				{
					ch_ent.ChunkType = ChunkType_VertAnim;
					ch_ent.ChunkVersion = VERTANIM_CHUNK_DESC_VERSION;
					ChunkList.Append(pSourceObject->GetMaxNode(), &ch_ent, true);
				}
			}
			else	//this object has already been saved in another chunk
			{
				//check if multiple nodes use the instanced-physique-assigned objects
				if (pb_nodes->GetInt(PB_WRITE_WEIGHTS) && FindPhysiqueModifier(der_obj))
				{
					sprintf(s,"Node \"%s\" is an instance of another node.\nInstanced nodes with Physique modifiers on them are not allowed therefore this node will NOT be saved."
						"\n\nDo you want skip this node and continue exporting?",pSourceObject->GetName().c_str());
					if (bPreviewMode)
						return IDNO;
					res = CryMessageBox(hwnd_nodes, s, "Instanced bone-animated nodes found!", MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);
					if (res == IDNO)
						return IDCANCEL;
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		// Prepare Bones hierarchy.
		//////////////////////////////////////////////////////////////////////////
		INodeTab allBones;
		PrepareAllBones( allBones );

		if (pExportSource->GetSkeleton() != 0)
		{
			//============================
			//Prepare bone hierarchy chunk
			//============================
			ch_ent.FileOffset		= -1;
			ch_ent.ChunkType		= ChunkType_BoneAnim;
			ch_ent.ChunkVersion		= BONEANIM_CHUNK_DESC_VERSION;
			ChunkList.Append(allBones[0],&ch_ent,true);

			for (int i = 0; i < pExportSource->GetSkeleton()->GetBones()->Count(); ++i)
			{
				IBone* pBone = pExportSource->GetSkeleton()->GetBones()->Get(i);
				if (pBone->GetMaterial() != 0)
				{
					RecursePrepareMtlChunks(pBone->GetMaterial());
				};
			}
		}

		//=======================
		//Prepare Bone Names
		//=======================
		if (BoneList.nodes.Count() > 0 /*&& pb_nodes->GetInt(PB_WRITE_WEIGHTS)*/)
		{
			ch_ent.ChunkType	= ChunkType_BoneNameList;
			ch_ent.ChunkVersion	= BONENAMELIST_CHUNK_DESC_0745::VERSION;
			ChunkList.Append(NULL,&ch_ent,true);
		}
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


		//=======================
		//Prepare Breakable Physics chunk
		//=======================
		int isBP = pb_physics->GetInt(PB_BREAKABLE_PHYSICS);
		if (isBP)
		{
			ch_ent.ChunkType	= ChunkType_BreakablePhysics;
			ch_ent.ChunkVersion	= BREAKABLE_PHYSICS_CHUNK_DESC::VERSION;
			ChunkList.Append(NULL,&ch_ent,true);
		}
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////



		//=======================
		//WRITE THE CHUNKS
		//=======================
		//Every chunk is prepared.. Now write them
		m_skippedMorphTargetNames.clear();

		AssetWriter writer(f, pErrorReporter, this);

		int lajno=0;
		for(i=0;i < ChunkList.ch_list.Count();i++)
		{
			int					ChunkType = ChunkList.ch_list[i].ChunkType;
			void*	ChunkPtr  = ChunkList.ptr_list[i].pRefTarget;
			int res=0;

			unsigned int uFlags = 0;
			uFlags |= bForceVertexColours ? nSMF_ForceSaveVertexColours : 0;

			switch(ChunkType)
			{
			case ChunkType_SourceInfo:
				SaveSourceInfo(pExportSource->GetInfo(), f, ChunkType, exportTime);
				break;

			case ChunkType_Mesh:
				{
					std::map<Mtl*, MaxMaterial*> dummyMap;
					MaxObject object(pErrorReporter, (INode*)ChunkPtr, pb_nodes->GetInt(PB_GENERATE_DEFAULT_UVS), bAllowBlending, this->BoneList, dummyMap, pb_nodes->GetFloat(PB_MORPH_MIN_OFFSET));
					Matrix34 transform;
					object.GetTransform(transform);
					Matrix34 objectOffset;
					object.GetObjectOffsetTransform(objectOffset);
					int nVertexAnimID = ChunkList.GetID((INode*)ChunkPtr, ChunkType_VertAnim);
					res=SaveMeshObject(f, writer, pExportSource->GetExportFlags(), pExportSource->GetSkeleton(), object.GetMesh(), object.GetMaterial(), transform, objectOffset, object.GetName(), i, nVertexAnimID, uFlags|nSMF_SaveMorphTargets|nSMF_SaveInitBonePos);
				}
				break;

			case ChunkType_BoneMesh:
				{
					std::map<Mtl*, MaxMaterial*> dummyMap;
					MaxSkeleton::Bone bone(pErrorReporter, "", (INode*)ChunkPtr, false, bAllowBlending, this->BoneList, dummyMap, 0, pb_nodes->GetFloat(PB_MORPH_MIN_OFFSET));
					Matrix34 transform;
					bone.GetTransform(transform);
					Matrix34 objectOffset;
					objectOffset.SetIdentity();
					int nVertexAnimID = ChunkList.GetID((INode*)ChunkPtr, ChunkType_VertAnim);
					res=SaveMeshObject(f, writer, pExportSource->GetExportFlags(), pExportSource->GetSkeleton(), bone.GetMesh(), bone.GetMaterial(), transform, objectOffset, bone.GetBoneName(), i, nVertexAnimID, uFlags|nSMF_IsBoneMeshChunk|nSMF_DontSaveBoneInfo);
				}
				break;

			case ChunkType_Helper:
				{
					MaxHelperObject helperObject((INode *)ChunkPtr);
					res=SaveHelperObject(f, &helperObject, (INode*)ChunkPtr);
				}
				break;

			case ChunkType_VertAnim:
				{
					MaxVertexAnimation vertexAnimation((INode*)ChunkPtr);
					res = SaveVertAnim(f, (INode*)ChunkPtr, &vertexAnimation);
				}
				break;

			case ChunkType_Node:
				{
					ISourceObject* pSourceObject = (ISourceObject*)ChunkPtr;
					res=SaveNode(f, pSourceObject);
				}
				break;

			case ChunkType_Controller:
				{
					res = SaveController(f, (IController*) ChunkPtr);
				}
				break;

			case ChunkType_MtlName:
				{
					lajno++;
					res=SaveMaterialName(f, ((MaxMaterial*) ChunkPtr), &ChunkList.ptr_list[i]);
				}
				break;


			case ChunkType_BoneAnim:
				res=SaveBoneHierarchy(f, pExportSource->GetSkeleton(), allBones);
				break;

			case ChunkType_BoneNameList:
				res = SaveBoneNameList(f, pExportSource->GetSkeleton());
				break;

			case ChunkType_SceneProps:
				res=SaveSceneProps(f, pExportSource->GetSceneProperties());
				break;

			case ChunkType_Timing:
				res=SaveTiming(f);
				break;

			case ChunkType_ExportFlags:
				res=SaveExportFlagsChunk(f, pExportSource->GetExportFlags());
				break;

			case ChunkType_MeshMorphTarget:
				break;

			case ChunkType_BoneInitialPos:
				break;

			case ChunkType_BreakablePhysics:
				res=SaveBreakablePhysics(f, pExportSource->GetBreakablePhysicsInfo());
				break;

			default:
				CryMessageBox(hwnd_nodes, "unsuported chunk type encountered", "Internal Error", MB_OK | MB_ICONINFORMATION);
				break;
			}

			if(res) 
				HALT(IDNO);
		}

		//=======================
		//Write Chunk List
		//=======================
		res=WriteChunkList(f, header);
		if (res)
			HALT(IDNO);

		//==========================================
		//CLOSE File and Call DLL Callback function
		//==========================================
		fclose(f);
		f = 0;

		if (!m_skippedMorphTargetNames.empty())
		{
			std::ostringstream message;
			message << m_skippedMorphTargetNames.size();
			message << " morph targets were not saved, since they don't have any vertices with the specified (";
			message << pb_nodes->GetFloat(PB_MORPH_MIN_OFFSET);
			message << ") minimal offset:\n";

			for (std::vector<std::string>::iterator itString = m_skippedMorphTargetNames.begin(); itString != m_skippedMorphTargetNames.end(); ++itString)
				message << "   \"" << *itString << "\"\n";

			MessageBox(ip->GetMAXHWnd(), message.str().c_str(), "Warning", MB_OK);
		}

		if (bRunResourceCompiler)
		{
			if (!RunResourceCompiler( sTmpFilename.c_str() ))
			{
				if (bIndividualFiles)
				{
					string msg = "Can't export this file ";
					msg += sExportFilename;
					msg += "\r\nbecause RC was unable to compile it";
					msg += "\r\n(by using temporary file ";
					msg += sTmpFilename;
					msg += ").";
					if (MessageBox(hwnd_nodes,msg.c_str(),"Warning", MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
					{
						HALT(IDCANCEL);
					}
				}
				HALT(IDNO);
			}
		}

		remove(sExportFilename.c_str());

		if (rename(sTmpFilename.c_str(), sExportFilename.c_str()) != 0)
		{
			if (bIndividualFiles)
			{
				string msg = "Can't export this file ";
				msg += sExportFilename;
				msg += "\r\nbecause can't rename file ";
				msg += sTmpFilename;
				msg += "\r\nto ";
				msg += sExportFilename;
				msg += "\r\nContinue?";
				if (MessageBox(hwnd_nodes,msg.c_str(),"Warning", MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
				{
					HALT(IDCANCEL);
				}
			}
			HALT(IDNO);
		}

#undef HALT

		//CallExtensionDLLs(filename,DLL_FLAG_GEOM);
		lastSavedFile = sExportFilename.c_str();
		m_bLastSavedIndividualFiles = bIndividualFiles;

	}

	delete pExportSource;

	return IDYES;
}

// WriteChunkList ______________________________________________________________________________________
bool CSExportUtility::WriteChunkList(FILE *f, FILE_HEADER &header)
{
	//get chunk list pos
	fpos_t chunklistpos;
	fgetpos(f,&chunklistpos);

	//remove the chunks that are not saved
	for(int i=ChunkList.ch_list.Count()-1;i>=0;i--) 
	{
		if(ChunkList.ch_list[i].FileOffset == -1)
		{
			ChunkList.ch_list.Delete(i,1);
			ChunkList.ptr_list.Delete(i,1);
		}
	}

	//=======================
	//Write # of Chunks
	//=======================
	int nch=ChunkList.ch_list.Count();
	int res=fwrite(&nch,sizeof(nch),1,f);
	if(res!=1) return true;

	//=======================
	//Write Chunk List
	//=======================
	for(i=0;i<nch;i++)
	{
		CHUNK_HEADER ch_ent=ChunkList.ch_list[i];
		int res=fwrite(&ch_ent,sizeof(ch_ent),1,f);
		if(res!=1) return true;
	}

	//update Header for chunk list offset
	header.ChunkTableOffset		= int(chunklistpos);
	res=fseek(f,0,SEEK_SET);
	if(res) return true;
	res=fwrite(&header,sizeof(header),1,f);
	if(res!=1) return true;

	return false;
}


int CSExportUtility::SaveDirDlgCallback (HWND hwnd, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		if (lastSavedFile.Length() && m_bLastSavedIndividualFiles)
		{
			const char* szLastFile = lastSavedFile.data();
			SendMessage (hwnd, BFFM_SETSELECTION, (WPARAM)szLastFile, (LPARAM)szLastFile);
		}/*
		else
		{
			TSTR strMaxFile = GetCOREInterface()->GetCurFileName();
			const char* szMaxFile = strMaxFile.data();
			for (int i = strMaxFile.Length()-1; i > 0; --i)
				if(strMaxFile[i] == '\\' || strMaxFile[i] == '/')
				{
					strMaxFile.Resize (i);
					break;
				}
			if (strMaxFile.Length())
				SendMessage (hwnd, BFFM_SETSELECTION, (WPARAM)strMaxFile.data(), TRUE);
		}*/
		break;
	}
	return 0;
}



int CSExportUtility::SaveDirDlg(int FileType,char *filename, char *title, char *def_file_name)
{
	BROWSEINFO bi;
	memset (&bi, 0, sizeof(bi));
	bi.hwndOwner      = hwnd_nodes;
	bi.pszDisplayName = filename;
	bi.lpszTitle      = title;
	bi.ulFlags        = BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;

	bi.lpfn   = SaveDirDlg_BrowseCallbackProc;
	bi.lParam = (LPARAM)this;

	LPITEMIDLIST pIIL = SHBrowseForFolder(&bi);
	if (!pIIL)
		return 0;

	if (!SHGetPathFromIDList (pIIL, filename))
		filename[0] = '\0';

	LPMALLOC pMalloc;
	if (SUCCEEDED(SHGetMalloc (&pMalloc)))
	{
		pMalloc->Free(m_pidlLast);
		pMalloc->Release();
	}

	m_pidlLast = pIIL;

	return filename[0];
}


// SaveFileDlg ______________________________________________________________________________________
// bDirectory  - if it's true, then the directory choose dialog is displayed
int CSExportUtility::SaveFileDlg(int FileType,char *filename, const char *title, const char *def_file_name)
{
	static char		FileName[256];

	OPENFILENAME	ofn;	
	FilterList		filter;

	char def_ext[16];

	switch(FileType)
	{
	case FileType_Geom:
		if (bBuilding)
		{
			// Save as a building.
			filter.Append(_T("Crytek Building File (*.bld)"));
			filter.Append(_T("*.bld"));
			strcpy(def_ext,"bld");
		}
		else
		{
			// Save as normal.
			filter.Append(_T("Crytek Geometry File (*.cgf)"));
			filter.Append(_T("*.cgf"));
			filter.Append(_T("Crytek Animated Geometry File (*.cga)"));
			filter.Append(_T("*.cga"));
			filter.Append(_T("Animation File (*.anm)"));
			filter.Append(_T("*.anm"));
			strcpy(def_ext,"cgf");
		}
		break;

	case FileType_Anim:
		filter.Append(_T("Crytek Bone Animation File (*.caf)"));
		filter.Append(_T("*.caf"));
		strcpy(def_ext,"caf");
		break;
	
	default: 
		assert(false);
	}

	FileName[0]=0;
	if(def_file_name) strcpy(FileName,def_file_name);

	//Set dialog box options
	ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner         = hwnd_nodes;
	ofn.hInstance         = NULL;	
	ofn.lpstrTitle        = title;
	ofn.lpstrFilter       = filter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 1;
	ofn.lpstrFile         = FileName;
	ofn.nMaxFile          = sizeof(FileName)-1;
	ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFileTitle     = 0;
	ofn.lpstrInitialDir   = m_strLastChosenSaveFile.Length() ? (const char*)m_strLastChosenSaveFile : (const char*)NULL;
	ofn.Flags             = OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lpstrDefExt       = def_ext;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL;

	if(!GetSaveFileName(&ofn))
	{
			return NULL; //cancelled (no error though, it is ok)
	}

	if(filename) strcpy(filename,FileName);

	// find the directory separator and remember the directory for subsequent references
	{
		for (int i = strlen(FileName)-1; i >= 0; --i)
		{
			if (FileName[i] == '\\' || FileName[i] == '/')
			{
				FileName[i] = '\0';
				break;
			}
		}
		m_strLastChosenSaveFile = FileName;
	}

	ResetReadOnlyFlag (filename);

	return TRUE;
}



INode *CSExportUtility::FindRootBone(INode *node)
{
	INode *n=node;
	if(!n) return NULL;

	while(n->GetParentNode())
	{
		Control *pc=n->GetTMController();
		if(pc && (pc->ClassID() == BIPBODY_CONTROL_CLASS_ID)) 
		{
			while (n->GetParentNode()
				&& n->EvalWorldState(0).obj->ClassID() == Class_ID(DUMMY_CLASS_ID,0))
			return n;
		}

		if(n->GetParentNode()->IsRootNode())
			return n;
		n=n->GetParentNode();
	}
	
	return NULL;
}

void CSExportUtility::UpdateProgress(int percent, char *format,  ...)
{
	static int  old_percent=-1234;
	static char old_format[64];

	va_list list;
	va_start(list, format);

	if(!hProgressBar) return;
	if((percent==old_percent) && (!strcmp(format,old_format))) return;

	old_percent=percent;
	strcpy(old_format,format);

	vsprintf(s, format, list);
	SetWindowText(hProgressWindow, s);
	SendMessage(hProgressBar, PBM_SETPOS, percent, 0);
}

void CSExportUtility::EnableDisableControls()
{
	if(!pmap_bones) return;
	if(!pmap_nodes) return;

	Interval ivalid;
	//check for dublicate entry
	bool RootFound=false;
}

void CSExportUtility::RemoveChildBones(INode *parent)
{
	if(!pb_nodes)	return;
	if(!pb_bones)	return;
	if(!pmap_bones) return;
	if(!pmap_nodes) return;

	for(int i=pb_bones->Count(PB_BONES)-1;i>=0;i--)
	{
		INode *bone=pb_bones->GetINode(PB_BONES,0,i);
		if(!bone) continue;

		bone=bone->GetParentNode();
		while(bone)
		{
			if(bone==parent)
			{
				DebugPrint("%s deletes the child bone %s\n",parent->GetName(),pb_bones->GetINode(PB_BONES,0,i)->GetName());
				DebugPrint("before: %d",pb_bones->Count(PB_BONES));
				pb_bones->Delete(PB_BONES,i,1);
				DebugPrint("after: %d\n",pb_bones->Count(PB_BONES));
				break;
			}

			bone=bone->GetParentNode();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSExportUtility::AddBoneToList( INode *bone )
{
	if (!bone)
		return;
	for(int i=0; i < pb_bones->Count(PB_BONES); i++)
	{
		INode *node = pb_bones->GetINode(PB_BONES,0,i);
		if (bone == node)
		{
			return;
		}
	}

	PB2Value v;
	v.r=bone;
	if (bone_validator.Validate(v))
	{
		pb_bones->Append(PB_BONES,1,&bone);
	}
}

void CSExportUtility::AddRelatedBones()
{
	if(!pb_nodes)	return;
	if(!pb_bones)	return;
	if(!pmap_bones) return;
	if(!pmap_nodes) return;

	
	pb_bones->SetCount(PB_BONES,0);

	int nobj=pb_nodes->Count(PB_NODES);
	for(int k=0;k<nobj;k++)
	{
		INode *nodek=pb_nodes->GetINode(PB_NODES,0,k);
		if(!nodek) continue;

		
		

		Modifier *phy_mod=FindPhysiqueModifier(nodek->GetObjectRef());
		if(!phy_mod) 
		{
			// Something in the skin modifier seems to crash if you call it while the file is being
			// closed, so let's not call it in that case, since it doesn't seem necessary!
			if (this->bClosingFile)
				continue;
			Modifier *skin_mod=FindSkinModifier(nodek->GetObjectRef());
			if (!skin_mod)  continue;	
			AddSkinRelatedBones(skin_mod,nodek);
			continue;
		}
		
	

		IPhysiqueExport *phyInterface = (IPhysiqueExport *)( phy_mod->GetInterface(I_PHYINTERFACE));
		if(!phyInterface) continue;

		IPhyContextExport *phyContex = phyInterface->GetContextInterface(nodek);
		if(!phyContex) continue;
		
		phyContex->ConvertToRigid(true);
		phyContex->AllowBlending(true);

		int n_vert=phyContex->GetNumberVertices();
		for(int i=0;i<n_vert;i++)
		{
			IPhyVertexExport *vert=phyContex->GetVertexInterface(i);
			
			int vert_type=vert->GetVertexType();
			switch(vert_type)
			{
			case RIGID_NON_BLENDED_TYPE:
				{
					IPhyRigidVertex *RigVert=(IPhyRigidVertex *)vert;
					INode *bone=RigVert->GetNode();

					PB2Value v;
					v.r=bone;

					if(pb_nodes->GetInt(PB_ADD_WHOLE_SKEL))
					{
						INode *root_bone=FindRootBone(bone);

						v.r = root_bone;
						if(bone_validator.Validate(v))
						{
							AddBoneToList(root_bone);
							DebugPrint("\"%s\" added....\n",bone->GetName());
							//phyContex->ReleaseVertexInterface(vert);
							break;
						}
					}

					if(bone_validator.Validate(v))
					{
						AddBoneToList(bone);
						DebugPrint("\"%s\" added....\n",bone->GetName());
						//RemoveChildBones(bone);
					}
				}
				break;

			case RIGID_BLENDED_TYPE:
				{
					IPhyBlendedRigidVertex *BlndVert=(IPhyBlendedRigidVertex *)vert;
					int nLinks=BlndVert->GetNumberNodes();
					for(int j=0;j<nLinks;j++)
					{
						INode *bone=BlndVert->GetNode(j);
						
						PB2Value v; v.r=bone;

						if(pb_nodes->GetInt(PB_ADD_WHOLE_SKEL))
						{
							INode *root_bone=FindRootBone(bone);
							//DebugPrint("Root of \"%s\" is \"%s\"\n",bone->GetName(), root_bone->GetName());

							v.r=root_bone;
							if(bone_validator.Validate(v))
							{
								AddBoneToList(root_bone);
								DebugPrint("\"%s\" added....\n",root_bone->GetName());
								i=n_vert+1; //break the outer loop (i) also
								break;
							}
						}

						if(bone_validator.Validate(v))
						{
							AddBoneToList(bone);
							DebugPrint("\"%s\" added....\n",bone->GetName());
							//RemoveChildBones(bone);
						}
					}
				}
				break;
			}
		
			phyContex->ReleaseVertexInterface(vert);
		} //for vertex
		
		phyInterface->ReleaseContextInterface(phyContex);
		phy_mod->ReleaseInterface(I_PHYINTERFACE, phyInterface);
	} //for obj
}

INode *CSExportUtility::DerObj2Node(Object *obj)
{
	int n=pb_nodes->Count(PB_NODES);
	for(int i=0;i<n;i++)
	{
		INode *node=pb_nodes->GetINode(PB_NODES,0,i);
		if(node && node->GetObjectRef()==obj ) return node;
	}

	return NULL;
}

//sorts the tab and removes duplicate elements
static int CompTable( const void *elem1, const void *elem2 ) 
{
	int a = *((int *)elem1);
	int b = *((int *)elem2);
	
	return a-b;
}

void CSExportUtility::CleanUpIntTab(IntTab &tab)
{
	tab.Sort(CompTable);
	for(int i=1;i<tab.Count();i++)
	{
		if(tab[i] == tab[i-1])
		{	
			tab.Delete(i,1);
			i--;
		}
	}
}

bool CSExportUtility::IsBipedBone(INode *node)
{
	if(!node) return false;
	Control *pc=node->GetTMController();
	return (pc && (
		(pc->ClassID() == BIPBODY_CONTROL_CLASS_ID) || 
		(pc->ClassID() == FOOTPRINT_CLASS_ID) || 
		(pc->ClassID() == BIPSLAVE_CONTROL_CLASS_ID)));
}

bool CSExportUtility::IsBipedRoot(INode *node)
{
	if(!node) return false;
	Control *pc=node->GetTMController();
	return (pc && (pc->ClassID() == BIPBODY_CONTROL_CLASS_ID));
}

bool CSExportUtility::IsFootPrint(INode *node)
{
	if(!node) return false;
	Control *pc=node->GetTMController();
	return (pc && (pc->ClassID() == FOOTPRINT_CLASS_ID));
}

bool CSExportUtility::IsLegBone(INode *node)
{
	if(!node) return false;
	if(FindRThigh(node,false))	return true;
	if(FindLThigh(node,false))	return true;
	if(FindRThigh(node,true))	return true;
	if(FindLThigh(node,true))	return true;
	return false;
}

INode * CSExportUtility::FindLThigh(INode *root, bool forward)
{
	char *name=root->GetName();
	char *pos=strstr(name," L Thigh");
	if(pos)
	{
		if( (pos-name+strlen(pos)) == strlen(name))
		{
			return root;
		}
	}

	if(forward)
	{
		for(int i=0;i<root->NumberOfChildren();i++)
		{
			if(INode *found=FindLThigh(root->GetChildNode(i),forward)) return found;
		}
	}
	else if(root->GetParentNode())
	{
		if(INode *found=FindLThigh(root->GetParentNode(),forward)) return found;
	}


	return NULL;
}

INode * CSExportUtility::FindRThigh(INode *root, bool forward)
{
	char *name=root->GetName();
	char *pos=strstr(name," R Thigh");
	if(pos)
	{
		if( (pos-name+strlen(pos)) == strlen(name))
		{
			return root;
		}
	}

	if(forward)
	{
		for(int i=0;i<root->NumberOfChildren();i++)
		{
			if(INode *found=FindRThigh(root->GetChildNode(i),forward)) return found;
		}
	}
	else if(root->GetParentNode())
	{
		if(INode *found=FindRThigh(root->GetParentNode(),forward)) return found;
	}
	return NULL;
}

void CSExportUtility::CallExtensionDLLs(const char *fname, int Flags)
{
	//Try to load CryPlug.dll
	char extension_name[256];
	sprintf(extension_name,"%s\\%s",ip->GetDir(APP_PLUGCFG_DIR),"Crytek*.dll");

	WIN32_FIND_DATA fd;
	HANDLE hSearch=FindFirstFile(extension_name, &fd);
	if(hSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			char dllname[256];
			sprintf(dllname,"%s\\%s",ip->GetDir(APP_PLUGCFG_DIR), fd.cFileName);
			UpdateProgress(25,"Loading Extension DLL: \"%s\"",fd.cFileName);
			HINSTANCE hDLL=LoadLibrary(dllname);
			CryCallbackFunc geomCallBack=NULL;
			CryCallbackFunc animCallBack=NULL;
			if(hDLL)
			{
				geomCallBack=(CryCallbackFunc)GetProcAddress(hDLL,_T("GeomCallback"));
				animCallBack=(CryCallbackFunc)GetProcAddress(hDLL,_T("AnimCallback"));
				
				if((Flags & DLL_FLAG_GEOM) && geomCallBack)
				{
					UpdateProgress(50,"Executing Extension DLL: \"%s\"",fd.cFileName);
					int res=geomCallBack(hwnd_nodes, fname);
					if(res && pb_options->GetInt(PB_WARN_DLL))
					{
						sprintf(s,"Extension DLL file \"%s\" returned an error value of %d",dllname,res);
						MessageBox(hwnd_nodes,s,"Extension DLL Error",MB_OK);
					}
				}

				if((Flags & DLL_FLAG_ANIM) && animCallBack)
				{
					UpdateProgress(75,"Executing Extension DLL: \"%s\"",fd.cFileName);
					int res=animCallBack(hwnd_nodes,fname);
					if(res && pb_options->GetInt(PB_WARN_DLL))
					{
						sprintf(s,"Extension DLL file \"%s\" returned an error value of %d",dllname,res);
						MessageBox(hwnd_nodes,s,"Extension DLL Error",MB_OK);
					}
				}

				FreeLibrary(hDLL);      
				hDLL=NULL;

				UpdateProgress(100,"Executing Extension DLL: \"%s\"",fd.cFileName);
			}
			else if(pb_options->GetInt(PB_WARN_DLL))
			{
				sprintf(s,"Can not load the Extension DLL file \"%s\"\nDo you want to continue executing other Extension DLLs?",dllname);
				int res=MessageBox(hwnd_nodes,s,"Extension DLL Error",MB_YESNO);
				if(res==IDNO) break;
			}

		} while (FindNextFile(hSearch, &fd));
	}
}

bool CSExportUtility::IsHaveBoneInfo( INode *node )
{
	// try to find physique or skin
	Modifier *mod=FindPhysiqueModifier(node->GetObjectRef());
	if(mod) 
		return true;
	else
	{
		mod = FindSkinModifier(node->GetObjectRef());
		if (mod) 
			return true;
	}
	// Doesnt have bone info.
	return false;		
}

bool CSExportUtility::AddSkinRelatedBones(Modifier *skin, INode *node)
{
	ISkin *skinInterface = (ISkin *) skin->GetInterface(I_SKIN);
	if (!skinInterface) return true;

	ISkinContextData *skinContext = skinInterface->GetContextInterface(node);

	int nVerts = skinContext->GetNumPoints();

	for (int i=0;i<nVerts;i++)
	{
		 int nLinks = skinContext->GetNumAssignedBones(i);
		 for (int j=0;j<nLinks;j++)
		 {
			 INode *bone = skinInterface->GetBone(skinContext->GetAssignedBone(i,j));
			 PB2Value v; 
			 v.r = bone;

			 if (pb_nodes->GetInt(PB_ADD_WHOLE_SKEL))
			 {
				 INode *root_bone = FindRootBone(bone);
				 v.r = root_bone;

				 if (bone_validator.Validate(v))
				 {
					 AddBoneToList(root_bone);
					 i=nVerts; // break outer loop
					 break;
				 }
			 } 

			 if (bone_validator.Validate(v))
			 {
				 AddBoneToList(bone);
			 }
		 }
	}

	skin->ReleaseInterface(I_SKIN, skinInterface);
	return false;
}


bool CSExportUtility::PreviewGeomFile()
{
	bool bIndividualFiles = pb_nodes->GetInt (PB_INDIVIDUAL_FILES) != 0;

	std::string sFilename = GetCOREInterface()->GetCurFilePath();

	int nExportType = pb_nodes->GetInt(PB_EXPORT_TO);
	switch (nExportType)
	{
	case EXPORT_TO_CGF:
		sFilename = PathUtil::ReplaceExtension( sFilename,CGF_EXTENSION );
		break;
	case EXPORT_TO_CHR:

		// [9/11/2005 MichaelS] Allow multiple characters to be exported from
		// the one scene, assuming that they share the same skeleton.
		//bIndividualFiles = false;

		sFilename = PathUtil::ReplaceExtension( sFilename,CHR_EXTENSION );
		break;
	case EXPORT_TO_CGA:
		bIndividualFiles = false;
		sFilename = PathUtil::ReplaceExtension( sFilename,CGA_EXTENSION );
		break;
	case EXPORT_TO_ANM:
		bIndividualFiles = false;
		sFilename = PathUtil::ReplaceExtension( sFilename,ANM_EXTENSION );
		break;
	default:
		CryMessageBox( NULL,"Unknown Export Format","Export Error",MB_OK );
		return false;
	}

	if (sFilename.empty())
		return false;

	if (bIndividualFiles)
	{
		// Get name of the selected node.
		for (int i = 0; i < pb_nodes->Count(PB_NODES); i++)
		{
			INode* pNode = pb_nodes->GetINode(PB_NODES,0,i);
			if (!pNode)
				continue;

			if (pNode->Selected())
			{
				sFilename = PathUtil::AddBackslash( PathUtil::GetPath(sFilename) ) + pNode->GetName();
				sFilename = PathUtil::ReplaceExtension(sFilename, FindExportExtension());
				break;
			}
		}
	}

	HWND h = FindWindow( NULL,"CryEngine Sandbox" );		//changed by [MG]
	if (!h)
	{
		// Open new instance of CryEdit.
		char cmd[1024];
		
		sprintf( cmd,"%s %s",m_sEditorExe.c_str(),sFilename.c_str() );

		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		memset( &si,0,sizeof(si) );
		si.cb = sizeof(si);
		if (!CreateProcess( NULL,cmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi ))
		{
			//ms->NonRigid_Links=true;
			sprintf(s,"Error executing Model Preview,  filename: %s",cmd );
			CryMessageBox(hwnd_nodes, s, "Warning", MB_OK|MB_ICONERROR);
		}
		else
			Sleep(100);
	}
	else
	{
		char szData[1024];
		strcpy( szData,sFilename.c_str() );
		COPYDATASTRUCT data;
		data.dwData = 1024;
		data.cbData = strlen(szData);
		data.lpData = szData;
		SendMessage( h,WM_COPYDATA,(WPARAM)NULL,(LPARAM)&data );
		SetActiveWindow( h );
		SetForegroundWindow( h );
	}
	return false;
}

bool CSExportUtility::PreviewGame( TSTR fileName )
{
	if (fileName.length() == 0)
		return true;

	// Open new instance of Game.
	char cmd[1024];
		
	HKEY hKey;
	char szMasterCD[_MAX_PATH];
	DWORD dwBufLen;
		
	RegOpenKeyEx( HKEY_CURRENT_USER,"Software\\Crytek",0,KEY_QUERY_VALUE,&hKey );
	if (RegQueryValueEx( hKey,"MasterCD", NULL, NULL,(LPBYTE)szMasterCD,&dwBufLen) != ERROR_SUCCESS)
	{
		strcpy( szMasterCD,"C:\\MasterCD" );
	}
	RegCloseKey( hKey );
	
	// Remove backslash if present.
	if (szMasterCD[strlen(szMasterCD)-1] == '\\')
		szMasterCD[strlen(szMasterCD)-1] = 0;
		
	sprintf( cmd,"%s\\XIsle.exe /preview %s",szMasterCD,(char*)fileName.data() );

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset( &si,0,sizeof(si) );
	si.cb = sizeof(si);
	CreateProcess( NULL,cmd,NULL,NULL,FALSE,0,NULL,szMasterCD,&si,&pi );
	Sleep(100);

	return false;
}

CStr CSExportUtility::GetRelativePath( const TSTR &fullPath )
{
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char fext[_MAX_EXT];

	strcpy( path_buffer,fullPath.data() );
	_splitpath( path_buffer,drive,dir,fname,fext );
	char *pos = dir;
	if (pos[0] == '\\')
		pos++;
	pos = strchr(pos,'\\');
	if (pos)
	{
		char temp[_MAX_DIR];
		strcpy( temp,pos+1 );
		strcpy( dir,temp );
	}
	else
	{
		strcpy( dir,"" );
	}
	_makepath( path_buffer,NULL,dir,fname,fext );
	return path_buffer;
}

//////////////////////////////////////////////////////////////////////////
bool CSExportUtility::RunResourceCompiler( const char *sExportFilename, bool bUseQuota, DWORD dwWindow )
{
	char cmd[4096];
	strcpy( cmd,"/threads=1 /refresh /WX");
	CResourceCompilerHelper* rch = GetResourceCompilerHelper();

	CResourceCompilerHelper::ERcCallResult result = rch->CallResourceCompiler(sExportFilename, cmd, 0, true, bUseQuota);
	if (result == CResourceCompilerHelper::eRcCallResult_notFound)
	{
		rch->ResourceCompilerUI(ip->GetMAXHWnd());
		result = rch->CallResourceCompiler(sExportFilename, cmd, 0, true, bUseQuota);
	}

	return result == CResourceCompilerHelper::eRcCallResult_success;

	/*
	std::string RCPath = (const char*)m_EditorPath;
	if (RCPath.empty())
	{
		MessageBox(NULL, "Cannot find Resource Compiler rc.exe executable. Please, setup correct path to Sandbox in CryExport.ini", "", MB_OK);
		return true;
	}
	RCPath = PathUtil::AddBackslash(PathUtil::GetPath(RCPath)) + "RC";
	std::string RCExe = PathUtil::AddBackslash(RCPath) + "rc.exe";

	// Refresh cgf file, treat warning as errors.
	if (bUseQuota)
		sprintf( cmd,"%s \"%s\" /singlethreaded /refresh /WX",RCExe.c_str(),sExportFilename ); ///wait
	else
		sprintf( cmd,"%s %s /singlethreaded /refresh /WX /nowindow",RCExe.c_str(),sExportFilename ); ///wait

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset( &si,0,sizeof(si) );
	si.cb = sizeof(si);
	//if (!CreateProcess( NULL,cmd,NULL,NULL,FALSE,0,NULL,szMasterCD,&si,&pi ))
	
	if (!CreateProcess( NULL,cmd,NULL,NULL,FALSE,dwWindow,NULL,RCPath.c_str(),&si,&pi ))
	{
		//ms->NonRigid_Links=true;
		sprintf(s,"Error executing Resource Compiler\r\n%s",cmd );
		CryMessageBox(hwnd_nodes, s, "Warning", MB_OK|MB_ICONERROR);
		return false;
	}
	// Wait until child process exits.
	WaitForSingleObject( pi.hProcess, INFINITE );

	DWORD lpExitCode = 0;
	GetExitCodeProcess( pi.hProcess,&lpExitCode );
	// Close process and thread handles.
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	if (lpExitCode != EXIT_SUCCESS)
		return false;
	/**/
	return true;
}

void CSExportUtility::SelectCustomExportFilename()
{
	// Get the file type filter.
	char szFilter[1024];
	LoadString(hInstance, FindExportFilenameTypesString(), (LPSTR)szFilter, sizeof(szFilter)); 
	std::replace(szFilter, szFilter + strlen(szFilter), '|', '\0');

	// Get the default filename.
	std::string sDefault = FindCustomExportFilename();
	char szFile[1024];
	strncpy(szFile, sDefault.c_str(), sizeof(szFile));
	szFile[sizeof(szFile) - 1] = 0;
	char szInitialDir[1024];
	strncpy(szInitialDir, PathUtil::GetPath(FindDefaultExportFilename()).c_str(), sizeof(szInitialDir));
	szInitialDir[sizeof(szInitialDir) - 1] = 0;

	// Ask the user for the new filename.
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd_nodes;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szFile;
	ofn.lpstrInitialDir = szInitialDir;
	ofn.nMaxFile = sizeof(szFile);
	ofn.Flags = OFN_SHOWHELP;
	ofn.lpstrTitle = "Choose export destination";

	int nDialogResult = GetSaveFileName(&ofn);
	if (nDialogResult == 0)
	{
		// Find out what really went wrong.
		const char* szDescription = 0;
		switch (CommDlgExtendedError())
		{
		case 0:
			// The user cancelled the operation.
			break;

#define COMDLGERR(E) case E: szDescription = #E; break
			COMDLGERR(CDERR_DIALOGFAILURE);
			COMDLGERR(CDERR_FINDRESFAILURE);
			COMDLGERR(CDERR_INITIALIZATION);
			COMDLGERR(CDERR_LOADRESFAILURE);
			COMDLGERR(CDERR_LOADSTRFAILURE);
			COMDLGERR(CDERR_LOCKRESFAILURE);
			COMDLGERR(CDERR_MEMALLOCFAILURE);
			COMDLGERR(CDERR_MEMLOCKFAILURE);
			COMDLGERR(CDERR_NOHINSTANCE);
			COMDLGERR(CDERR_NOHOOK);
			COMDLGERR(CDERR_NOTEMPLATE);
			COMDLGERR(CDERR_STRUCTSIZE);
			COMDLGERR(FNERR_BUFFERTOOSMALL);
			COMDLGERR(FNERR_INVALIDFILENAME);
			COMDLGERR(FNERR_SUBCLASSFAILURE);
#undef COMDLGERR
			default:
				szDescription = "UNKNOWN";
				break;
		}
		if (szDescription != 0)
		{
			if (IDYES ==
				CryMessageBox(hwnd_nodes, (std::string("Save dialog failed: ") + szDescription + " - Reset to default export filename?").c_str(), "Dialog Failure", MB_YESNO | MB_ICONERROR))
			{
				pb_nodes->SetValue(PB_CUSTOM_FILENAME, 0, const_cast<TCHAR*>(FindDefaultCustomExportParameter().c_str()));
			}
		}
	}
	else
	{
		// Get the directory that the current file is in.
		std::string sDefaultFilename = GetCOREInterface()->GetCurFilePath();
		std::string sDirectory = PathUtil::GetPath(sDefaultFilename);
		std::string sExtension = FindExportExtension();

		// Check whether this is an absolute path.
		std::string sPath = szFile;
		if (!PathUtil::IsAbsolutePath(sPath))
			sPath = PathUtil::Make(sDirectory, sPath, sExtension);

		// The export file must be in the same directory as the current file or a subdirectory.
		std::string sFilename = sPath;
		if (sDefaultFilename == "" || !PathUtil::IsInDirectory(sDirectory, sFilename))
		{
			if (sDefaultFilename != "")
				CryMessageBox(hwnd_nodes, "Files must be exported to the same directory as max file or in a subdirectory.", "Invalid Export Filename", MB_OKCANCEL);
			sFilename = FindDefaultExportFilename();
		}

		// Make the relative path.
		std::string sRelativePath = PathUtil::GetRelativePath(sDirectory, sFilename);

		// Remove the extension.
		PathUtil::RemoveExtension(sRelativePath);

		// Alter the parameter.
		pb_nodes->SetValue(PB_CUSTOM_FILENAME, 0, const_cast<TCHAR*>(sRelativePath.c_str()));
	}
}

std::string CSExportUtility::FindDefaultExportFilename()
{
	std::string sFilename = GetCOREInterface()->GetCurFilePath();

	int nExportType = pb_nodes->GetInt(PB_EXPORT_TO);
	switch (nExportType)
	{
	case EXPORT_TO_CGF:
		return PathUtil::ReplaceExtension(sFilename, CGF_EXTENSION);
	case EXPORT_TO_CHR:
		return PathUtil::ReplaceExtension(sFilename, CHR_EXTENSION);
	case EXPORT_TO_CGA:
		return PathUtil::ReplaceExtension(sFilename, CGA_EXTENSION);
	case EXPORT_TO_ANM:
		return PathUtil::ReplaceExtension(sFilename, ANM_EXTENSION);
	default:
		return "";
	}
}

std::string CSExportUtility::FindExportExtension()
{
	int nExportType = pb_nodes->GetInt(PB_EXPORT_TO);
	switch (nExportType)
	{
	case EXPORT_TO_CGF:
		return CGF_EXTENSION;
	case EXPORT_TO_CHR:
		return CHR_EXTENSION;
	case EXPORT_TO_CGA:
		return CGA_EXTENSION;
	case EXPORT_TO_ANM:
		return ANM_EXTENSION;
	default:
		return "";
	}
}

std::string CSExportUtility::FindCustomExportFilename()
{
	std::string sDefaultFilename = GetCOREInterface()->GetCurFilePath();
	std::string sDirectory = PathUtil::GetPath(sDefaultFilename);

	std::string sExtension = FindExportExtension();

	const TCHAR* szFilename = pb_nodes->GetStr(PB_CUSTOM_FILENAME);
	std::string sPath;
	if (szFilename != 0)
		sPath = PathUtil::Make(sDirectory, szFilename, sExtension);
	else
		sPath = FindDefaultExportFilename();
	return sPath;
}

std::string CSExportUtility::FindDefaultCustomExportParameter()
{
	std::string sFileName = GetCOREInterface()->GetCurFilePath();
	return PathUtil::GetFileName(sFileName);
}

int CSExportUtility::FindExportFilenameTypesString()
{
	int nExportType = pb_nodes->GetInt(PB_EXPORT_TO);
	switch (nExportType)
	{
	case EXPORT_TO_CGF:
		return IDS_CGF_FILETYPE_STRING;
	case EXPORT_TO_CHR:
		return IDS_CHR_FILETYPE_STRING;
	case EXPORT_TO_CGA:
		return IDS_CGA_FILETYPE_STRING;
	case EXPORT_TO_ANM:
		return IDS_ANM_FILETYPE_STRING;
	default:
		return -1;
	}
}

void CSExportUtility::RunTestSuite()
{
	// Check whether the configuration file specified a test suite directory.
	if (this->m_sTestSuitePath.empty())
	{
		CryMessageBox(hwnd_nodes, "Cannot run automated test - please specify a test directory in Crytek.ini.", "Test Suite error", MB_OK | MB_ICONERROR);
		return;
	}

	// Run the test suite.
	TestSuite(this, HandleTestSuiteError, this->m_sTestSuitePath).Run();
}

void CSExportUtility::HandleTestSuiteError(const std::string& sMessage, CSExportUtility* pUtility)
{
	CryMessageBox(pUtility->hwnd_nodes, sMessage.c_str(), "Test Suite Error", MB_OK | MB_ICONERROR);
}

void ExportColladaButton(CSExportUtility *tsu)
{
	if (!tsu->RunPreExportCallback())
		return;

	int const nSaved = tsu->SaveColladaGeomFile();

	if (nSaved == IDNO)
	{
		WarningFileNotSaved(tsu->ip->GetMAXHWnd(), "");
	}
}

void ExportButton(CSExportUtility *tsu, std::time_t exportTime, bool bRunResourceCompiler)
{
	if (!tsu->RunPreExportCallback())
		return;

	tsu->hProgressWindow= CreateDialog(hInstance, MAKEINTRESOURCE(IDD_PROGRESS), tsu->hwnd_nodes, ProgressDlgProc);
	tsu->hProgressBar	= GetDlgItem(tsu->hProgressWindow, IDC_PROGRESS);
	RECT maxrect, myrect;
	GetWindowRect(tsu->ip->GetMAXHWnd(), &maxrect);
	GetWindowRect(tsu->hProgressWindow, &myrect);
	SetWindowPos(tsu->hProgressWindow, HWND_TOP, (maxrect.right+maxrect.left-myrect.right+myrect.left)>>1, (maxrect.bottom+maxrect.top-myrect.bottom+myrect.top)>>1,0,0,SWP_NOSIZE);
	ShowWindow(tsu->hProgressWindow, SW_SHOW);

	int nSaved = tsu->SaveGeomFile(exportTime, bRunResourceCompiler);

	DestroyWindow(tsu->hProgressWindow);

	tsu->hProgressBar = tsu->hProgressWindow = NULL;

	if (nSaved == IDNO)
		WarningFileNotSaved (tsu->ip->GetMAXHWnd(), "");
}

Value* CryExp_cf( Value** arg_list, int count )
{
	check_arg_count( CryExp, 0, count );

	//Point3  p1 = arg_list[0]->to_point3();
	//Point3  p2 = arg_list[1]->to_point3();

	//float dSq = (p2.x-p1.x)*(p2.x-p1.x) - (p2.y-p1.y)*(p2.y-p1.y) - (p2.z-p1.z)*(p2.z-p1.z);


	if (once)
		CSExportUtilCD.MakeAutoParamBlocks(&theCSExportUtility);
	once = 0;

	theCSExportUtility.InitConfig();

	bool bPrev = bDontShowMessageBoxes;
	bDontShowMessageBoxes = true;
	ExportButton(&theCSExportUtility);
	bDontShowMessageBoxes = bPrev;

	return ( Integer::intern(0) );
}

void NodesExportToAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
	//// Update the custom export filename parameter to have the correct filename (since the extension has now changed).
	//std::string sFilename = m_pUtility->FindDefaultExportFilename();
	//m_pUtility->pb_nodes->SetValue(PB_CUSTOM_FILENAME, 0, const_cast<TCHAR*>(sFilename.c_str()));

	// Update the file types in the custom file save dialog.
	int nFiletypeDescriptionStringID = m_pUtility->FindExportFilenameTypesString();
	nodes_param_blk.GetParamDef(PB_CUSTOM_FILENAME).file_types = nFiletypeDescriptionStringID;
	//m_pUtility->pb_nodes->CallSet(PB_CUSTOM_FILENAME);
	static std::string sInitialFilePickerPath;
	sInitialFilePickerPath = m_pUtility->FindCustomExportFilename();
	nodes_param_blk.GetParamDef(PB_CUSTOM_FILENAME).init_file = const_cast<TCHAR*>(sInitialFilePickerPath.c_str());
}

void NodesCustomFilenameAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
	// Update the edit box that shows the custom filename.
	if (m_pUtility->hwnd_nodes != NULL)
	{
		HWND hCustomFilenameEdit = GetDlgItem(m_pUtility->hwnd_nodes, IDC_CUSTOMFILENAMEEDIT);
		if (hCustomFilenameEdit != NULL)
			SendMessage(hCustomFilenameEdit, WM_SETTEXT, 0, (LPARAM)v.s);
	}
}

BOOL NodesCustomFilenameValidator::Validate(PB2Value& v)
{
	// Get the directory that the current file is in.
	std::string sFilename = GetCOREInterface()->GetCurFilePath();
	std::string sDirectory = PathUtil::GetPath(sFilename);

	// The export file must be in the same directory as the current file or a subdirectory.
	return PathUtil::IsInDirectory(sDirectory, v.s) ? TRUE : FALSE;
}

std::string CSExportUtility::FindDefaultBoneAnimExportFilename()
{
	INode *rootbone=pb_bones->GetINode(PB_BONES,0,0);

	char def_fname[1024];
	char *max_name=GetCOREInterface()->GetCurFileName();
	if(strlen(max_name)>=4)
	{
		strcpy(def_fname,max_name);
		def_fname[strlen(def_fname)-4]='_';
		def_fname[strlen(def_fname)-3]=0;
		if (rootbone) 
			strcat(def_fname,rootbone->GetName());
	}
	else strcpy(def_fname,rootbone->GetName());
	strcat(def_fname, ".caf");
	return def_fname;
}

#define HALT { fclose(f); DeleteFile(exportParams.sFilename.c_str()); return IDNO; }
int CSExportUtility::ExportBoneAnimToFile(const BoneAnimExportParameters& exportParams)
{
	ErrorReporter errorReporter(this);
	ErrorReporter* pErrorReporter = &errorReporter;

	//===================================
	//check if there are nodes to export
	//===================================
	std::vector<INode*> nodes;
	for (int i = 0; i < pb_nodes->Count(PB_NODES); ++i)
	{
		INode* node = pb_nodes->GetINode(PB_NODES, 0, i);
		if (!node)
			continue;
		nodes.push_back(node);
	}

	if (nodes.empty())
	{
		MessageBox(hwnd_nodes,"Node list is Empty\nPlease Add one or more nodes to the list which will be exported","No nodes to Export",MB_OK | MB_ICONEXCLAMATION);
		return IDCANCEL;
	}

	Interval ivalid;
	TimeValue time=GetTime();

	MaxExportSource* pExportSource = this->CreateExportDataSource(pErrorReporter, nodes);

	int	 res=0;
	FILE *f = fopen(exportParams.sFilename.c_str(),"wb");
	int nobjects=pb_bones->Count(PB_BONES);
	for(int i=nobjects-1;i>=0;i--)	if(!pb_bones->GetINode(PB_BONES,time,i)) nobjects--;
	if (!f) return IDNO; // error: cannot open the file

	ChunkList.Reset();

	//==================
	//Write File Header
	//==================
	FILE_HEADER header;
	std::memset(&header, 0, sizeof(header));
	header.FileType				= FileType_Anim;
	header.ChunkTableOffset		= -1;
	header.Version				= ChunkFileVersion;

	strcpy(header.Signature,FILE_SIGNATURE);
	res=fwrite(&header,sizeof(header),1,f);
	res=res!=1;
	if (res)
		HALT;

	CHUNK_HEADER ch_ent;
	ChunkList.Reset();
	BoneList.Reset();
	ch_ent.FileOffset		= -1;

	//=======================
	//Prepare Export Flags.
	//=======================
	ch_ent.ChunkType		= ChunkType_ExportFlags;
	ch_ent.ChunkVersion		= EXPORT_FLAGS_CHUNK_DESC::VERSION;
	ChunkList.Append(NULL,&ch_ent,true);

	//=======================
	//Prepare SceneProperties
	//=======================
	int numProps = ip->GetNumProperties(PROPSET_USERDEFINED);
	if(numProps)
	{
		ch_ent.ChunkType		= ChunkType_SceneProps;
		ch_ent.ChunkVersion		= SCENEPROPS_CHUNK_DESC_VERSION;
		ChunkList.Append(NULL,&ch_ent,true);
	}
	//=======================
	//Prepare Timing
	//=======================
	ch_ent.ChunkType		= ChunkType_Timing;
	ch_ent.ChunkVersion		= TIMING_CHUNK_DESC_VERSION;
	ChunkList.Append(NULL,&ch_ent,true);

	//---------------------------------------------------
	//Get all the bones and prepare the controller chunks
	//---------------------------------------------------
	INodeTab allnodes;
	PrepareAllBones(allnodes);

	//add controller chunks
	for(int i=0;i<allnodes.Count();i++)
	{
		INode *node = allnodes[i];

		//============================
		//Prepare Controller chunk(s)
		//============================
		ch_ent.ChunkType		= ChunkType_Controller;
		ch_ent.ChunkVersion		= CONTROLLER_CHUNK_DESC_0827::VERSION;
		ChunkList.Append(node,&ch_ent,true);
	}

	//===========================
	//prepare bone namelist chunk
	//===========================
	ch_ent.ChunkType		= ChunkType_BoneNameList;
	ch_ent.ChunkVersion		= BONENAMELIST_CHUNK_DESC_0745::VERSION;
	ChunkList.Append(NULL,&ch_ent,true);

	//---------------------------------------------------
	//Every chunk is prepared.. Now write them
	//---------------------------------------------------
	//==================================================
	//Treverse the Bone Tree and save Controller Chunks
	//==================================================

	for (int nboneroot = 0; nboneroot < exportParams.bones.size(); nboneroot++)
	{
		INode* rootbone = exportParams.bones[nboneroot];
		res = SaveBoneController(exportParams.range, f, rootbone);
		if (res)
			HALT;
	}

	//==================
	//Save other chunks
	//==================
	for(i=0;i<ChunkList.ch_list.Count();i++)
	{
		int	ChunkType = ChunkList.ch_list[i].ChunkType;
		void* ChunkPtr = ChunkList.ptr_list[i].pRefTarget;
		int res=0;

		switch(ChunkType)
		{
		case ChunkType_Controller:
			//skip: we expoerted hierarchically with SaveBoneController
			break;

		case ChunkType_BoneAnim:
			//res=SaveBoneHierarchy(f, allnodes);
			break;

		case ChunkType_BoneNameList:
			res=SaveBoneNameList(f, pExportSource->GetSkeleton());
			break;

		case ChunkType_Timing:
			res=SaveTiming(f, &exportParams.range);
			break;

		case ChunkType_SceneProps:
			res=SaveSceneProps(f);
			break;

		case ChunkType_ExportFlags:
			res=SaveExportFlagsChunk(f, pExportSource->GetExportFlags());
			break;

		case ChunkType_BoneMesh:
			break;

		default:
			CryMessageBox(hwnd_nodes, "unsuported chunk type encountered", "Internal Error", MB_OK | MB_ICONINFORMATION);
		}

		if (res)
			HALT;
	}

	//=======================
	//Write Chunk List
	//=======================
	res=WriteChunkList(f, header);
	if (res)
		HALT;

	//==========================================
	//CLOSE File and Call DLL Callback function
	//==========================================
	fclose(f);
	CallExtensionDLLs(exportParams.sFilename.c_str(), DLL_FLAG_ANIM);

	return IDYES;
}
#undef HALT	

void CSExportUtility::SetPreExportCallback(Value* callback)
{
	preExportCallback = callback;
}

bool CSExportUtility::RunPreExportCallback()
{
	bool allowExport = true;

	if (preExportCallback)
	{
		try
		{
			Value* result = preExportCallback->apply(0, 0);
			allowExport = result->to_bool();
		}
		catch (MAXScriptException e)
		{
			StringStream error;
			e.sprin1(&error);
			std::string message = std::string("Error while calling export callback: ") + error.content_string + "\nContinuing with export. See maintainer of CryTools for more information.";
			MessageBox(hwnd_nodes, message.c_str(), "Callback Error", MB_ICONWARNING | MB_OK);
		}
	}

	if (!allowExport)
	{
		int result = MessageBox(hwnd_nodes, "The pre-export validation script has reported a problem with the scene.\nFor further information contact maintainer of CryTools.\nDo you wish to force the export to continue?", "Scene Check Error", MB_ICONERROR | MB_YESNO);
		if (result == IDYES)
			allowExport = true;
	}

	return allowExport;
}

void CSExportUtility::CopyRangesToTimeTags()
{
	if (once)
	{
		CSExportUtilCD.MakeAutoParamBlocks(this);
		once = 0;
		InitConfig();
	}
	IFrameTagManager* frameTagMgr = (IFrameTagManager*)GetCOREInterface( FRAMETAGMANAGER_INTERFACE );

	for(int i=0;i<Ranges.size();i++)
	{
		NamedRange ent=Ranges[i];

		CStr startFrame = ent.sName.c_str();
		startFrame.Append("_start");
		CStr endFrame		= ent.sName.c_str();
		endFrame.Append("_end");

		DWORD startID = frameTagMgr->CreateNewTag(startFrame, ent.nStart * GetTicksPerFrame(), 0, false);
		frameTagMgr->CreateNewTag(endFrame, ent.nEnd * GetTicksPerFrame(), startID, false);
	}
}

const char* CSExportUtility::GetCustomFilename()
{
	if (once)
	{
		CSExportUtilCD.MakeAutoParamBlocks(this);
		once = 0;
		InitConfig();
	}
	const TCHAR* szFilename = pb_nodes->GetStr(PB_CUSTOM_FILENAME);
	return szFilename ? szFilename : "";
}

void CSExportUtility::SetCustomFilename(const char *filename)
{
	if (once)
	{
		CSExportUtilCD.MakeAutoParamBlocks(this);
		once = 0;
		InitConfig();
	}
	pb_nodes->SetValue(PB_CUSTOM_FILENAME, 0, const_cast<TCHAR*>(filename));
	SaveConfigToMaxFile();
}

bool CSExportUtility::GetUseCustomFilename()
{
	if (once)
	{
		CSExportUtilCD.MakeAutoParamBlocks(this);
		once = 0;
		InitConfig();
	}

	return pb_nodes->GetInt(PB_USE_CUSTOM_FILENAME) != 0;
}

void CSExportUtility::SetUseCustomFilename(bool useCustom)
{
	if (once)
	{
		CSExportUtilCD.MakeAutoParamBlocks(this);
		once = 0;
		InitConfig();
	}

	pb_nodes->SetValue(PB_USE_CUSTOM_FILENAME, 0, useCustom);
	SaveConfigToMaxFile();
}

ErrorReporter::ErrorReporter(CSExportUtility* pUtility)
:	pUtility(pUtility)
{
}

void ErrorReporter::Report(ErrorLevel level, const std::string& sMessage)
{
	std::string sCaption = "No Caption";
	unsigned int uFlags = MB_OK;
	switch (level)
	{
	case Info:
		sCaption = "Information";
		uFlags = MB_OK | MB_ICONINFORMATION;
		break;
	case Warning:
		sCaption = "Warning";
		uFlags = MB_OK | MB_ICONINFORMATION;
		break;
	case Error:
		sCaption = "Error";
		uFlags = MB_OK | MB_ICONINFORMATION;
		break;
	}
	CryMessageBox(this->pUtility->hwnd_nodes, sMessage.c_str(), sCaption.c_str(), uFlags);
}

MaxExportSource* CSExportUtility::CreateExportDataSource(ErrorReporter* pErrorReporter, std::vector<INode*>& nodes)
{
	// Create an export source object to read the data from the max scene.
	std::vector<INode*> rootBones;
	for (int i = 0; i < pb_bones->Count(PB_BONES); i++)
	{
		INode *rootbone = pb_bones->GetINode(PB_BONES,0,i);
		if(!rootbone)
			continue;

		rootBones.push_back(rootbone);
	}
	NodeUtils::ValidChildrenListKeepDummySetting eKeepDummy = (pb_bones->GetInt(PB_IGNORE_DUMMY) ? NodeUtils::ValidChildrenListIgnoreDummy : NodeUtils::ValidChildrenListKeepDummy);
	NodeUtils::ValidChildrenListSortSetting eSort = (pb_bones->GetInt(PB_SORT_BONES) ? NodeUtils::ValidChildrenListSort : NodeUtils::ValidChildrenListNoSort);

	bool bGenDefUVs = pb_nodes->GetInt(PB_GENERATE_DEFAULT_UVS) != 0;
	bool bAllowBlending = pb_nodes->GetInt(PB_LINK_TYPE, 0);
	MaxExportSource* pExportSource = new MaxExportSource(
		pErrorReporter,
		this->ip,
		nodes,
		rootBones,
		eKeepDummy,
		eSort,
		bGenDefUVs,
		bAllowBlending,
		this->BoneList,
		pb_physics->GetInt(PB_GRANULARITY),
		pb_nodes->GetInt(PB_MERGE_OBJECTS) != 0,
		pb_nodes->GetInt(PB_WRITE_WEIGHTS) != 0,
		pb_nodes->GetInt(PB_WRITE_VCOL) != 0,
		pb_nodes->GetInt(PB_ALLOW_MULTIUV) != 0,
		pb_nodes->GetFloat(PB_MORPH_MIN_OFFSET));
	return pExportSource;
}

// MAXScript interface func.
void CryExporterOpsImpl::ExportNodes()
{
	extern bool bDontShowMessageBoxes;

	bool bPrev = bDontShowMessageBoxes;
	bDontShowMessageBoxes = true;
	ExportButton(&theCSExportUtility, 0, false);
	bDontShowMessageBoxes = bPrev;
}

bool CryExporterOpsImpl::CanExportAnims()
{
	return theCSExportUtility.CanExportBones();
}

void CryExporterOpsImpl::ExportAnims()
{
	extern bool bDontShowMessageBoxes;

	bool bPrev = bDontShowMessageBoxes;
	bDontShowMessageBoxes = true;
	if (theCSExportUtility.CanExportBones())
		theCSExportUtility.SaveBoneAnimFile("");
	bDontShowMessageBoxes = bPrev;
}

void CryExporterOpsImpl::CopyRangesToTimeTags()
{
	theCSExportUtility.CopyRangesToTimeTags();
}

const char* CryExporterOpsImpl::GetCustomFilename()
{
	return theCSExportUtility.GetCustomFilename();
}

void CryExporterOpsImpl::SetCustomFilename(const char *filename)
{
	theCSExportUtility.SetCustomFilename(filename);
}

bool CryExporterOpsImpl::GetUseCustomFilename()
{
	return theCSExportUtility.GetUseCustomFilename();
}

void CryExporterOpsImpl::SetUseCustomFilename(bool useCustom)
{
	theCSExportUtility.SetUseCustomFilename(useCustom);
}

void CryExporterOpsImpl::ExportAnim(const char* szString)
{
	extern bool bDontShowMessageBoxes;

	bool bPrev = bDontShowMessageBoxes;
	bDontShowMessageBoxes = true;
	if (theCSExportUtility.CanExportBones())
		theCSExportUtility.SaveBoneAnimFile(szString);
	bDontShowMessageBoxes = bPrev;
}

void CryExporterOpsImpl::RegisterExportCallback(Value* value)
{
	theCSExportUtility.SetPreExportCallback(value);
}

void CryExporterOpsImpl::SetNodeList(Tab<INode*> nodeList)
{
	theCSExportUtility.pb_nodes->ZeroCount(PB_NODES);
	for (int nodeIndex = 0; nodeIndex < nodeList.Count(); ++nodeIndex)
	{
		INode* pNode = nodeList[nodeIndex];
		theCSExportUtility.pb_nodes->Append(PB_NODES, 1, &pNode);
	}
}

Tab<INode*> CryExporterOpsImpl::GetNodeList()
{
	Tab<INode*> nodes;
	for (int i = 0; i < theCSExportUtility.pb_nodes->Count(PB_NODES); ++i)
	{
		INode* node = theCSExportUtility.pb_nodes->GetINode(PB_NODES, 0, i);
		nodes.Append(1, &node);
	}
	return nodes;
}

void CryExporterOpsImpl::SetBoneList(Tab<INode*> nodeList)
{
	theCSExportUtility.pb_bones->ZeroCount(PB_BONES);
	for (int nodeIndex = 0; nodeIndex < nodeList.Count(); ++nodeIndex)
	{
		INode* pNode = nodeList[nodeIndex];
		theCSExportUtility.pb_bones->Append(PB_BONES, 1, &pNode);
	}
}

Tab<INode*> CryExporterOpsImpl::GetBoneList()
{
	Tab<INode*> bones;
	for (int i = 0; i < theCSExportUtility.pb_bones->Count(PB_BONES); ++i)
	{
		INode* bone = theCSExportUtility.pb_bones->GetINode(PB_BONES, 0, i);
		bones.Append(1, &bone);
	}
	return bones;
}

const char* CryExporterOpsImpl::GetRootPath()
{
	string sPath = GetResourceCompilerHelper()->GetSettingsManager()->GetRootPath();
	char* lpPath = new char[sPath.length()+1];
	memcpy(lpPath, sPath.c_str(), sPath.length()+1);
	return lpPath;
}

//////////////////////////////////////////////////////////////////////////
const char* CryExporterOpsImpl::GetValue(const char* szKey)
{
	CEngineSettingsManager* sm = GetResourceCompilerHelper()->GetSettingsManager();
	string* val = new string();
	if (sm->GetModuleSpecificEntry(szKey, *val))
		return val->c_str();
	return "";
}

//////////////////////////////////////////////////////////////////////////
void CryExporterOpsImpl::SetValue(const char* szKey, const char* szValue)
{
	CEngineSettingsManager* sm = GetResourceCompilerHelper()->GetSettingsManager();
	sm->SetModuleSpecificEntry(szKey,szValue);
}


void CryExporterOpsImpl::ExecuteCommandLine(const char* szCommandLine, bool wait)
{
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};

	si.cb = sizeof( si );
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;

	std::string cmdLine(szCommandLine);

	if (!cmdLine.empty())
	{
		if (CreateProcess( NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi ))
		{
			if (wait)
				WaitForSingleObject( pi.hProcess, INFINITE );
		}

		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
}
