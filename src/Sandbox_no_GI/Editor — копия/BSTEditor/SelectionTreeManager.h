////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeManager.h
//  Version:     v1.00
//  Created:     24/03/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __SelectionTreeManager_H__
#define __SelectionTreeManager_H__

#include "Util/IXmlHistoryManager.h"
#include "LevelIndependentFileMan.h"
#include <ISelectionTreeManager.h>
#include "SelectionTreeErrorReport.h"

class CSelectionTreeModifier;
class CSelectionTreeVariablesView;
class CSelectionTreeSignalsView;
class CSelectionTreeGraph;
class CSelectionTreeValidator;

// Tree data
enum ESelectionTreeGroupTypeId
{
	eSTGTI_Tree = 0,
	eSTGTI_Block,
};

enum ESelectionTreeTypeId
{
	eSTTI_Undefined = -1,
	eSTTI_All = eSTTI_Undefined,
	eSTTI_NameInfo = 0,
	eSTTI_Variables,
	eSTTI_Signals,
	eSTTI_LeafTranslations,
	eSTTI_Tree,
};

struct SSelectionTreeBlockInfo
{
	SSelectionTreeBlockInfo();

	const char* Name;
	const char* FileName;
	bool IsModified;
	ESelectionTreeTypeId Type;

	XmlNodeRef XmlData;
};

typedef std::vector<SSelectionTreeBlockInfo> TBlockInfoList;

struct SSelectionTreeInfo
{
	SSelectionTreeInfo();

	const char* Name;
	const char* FileName;
	const char* TreeType;
	bool IsModified;
	bool IsLoaded;
	bool IsTree;
	int CurrTreeIndex;
	int GetBlockCount() const {return Blocks.size();}
	bool GetBlockById( SSelectionTreeBlockInfo& out, ESelectionTreeTypeId typeId, int index = 0 ) const;
	int GetBlockCountById( ESelectionTreeTypeId typeId ) const;

	TBlockInfoList Blocks;
};
typedef std::list< SSelectionTreeInfo > TSelectionTreeInfoList;

class CXmlHistoryManager;
struct SXmlHistoryGroup;
struct SXmlHistory;

class CSelectionTreeManager
	: public IXmlHistoryEventListener
	, public ILevelIndependentFileModule
	, public ISelectionTreeObserver
{
public:
	CSelectionTreeManager();
	~CSelectionTreeManager();

	// Init
	void Init();

	// History
	IXmlHistoryManager* GetHistory();

	// load / save
	void LoadFromFolder( const char* folder );
	bool SaveAll();

	// info
	void GetInfoList( TSelectionTreeInfoList& infoList ) const;
	bool GetRefInfoByName( ESelectionTreeTypeId type, const char* name, SSelectionTreeBlockInfo& out, SSelectionTreeInfo* pGroup = NULL ) const;
	bool GetTreeInfoByName(SSelectionTreeInfo& out, const char* name) const;
	bool GetRefGroupInfoByName(SSelectionTreeInfo& out, const char* name) const;
	bool GetCurrentTreeInfo(SSelectionTreeInfo& out, int* pIndex = NULL) const;

	// change views
	void DisplayTree( const char* name );
	void DisplayRefGroup( const char* name, int blockTreeIndex = 0 );
	void UnloadViews();

	// add/delete/rename tree
	struct STreeDefinition
	{
		const char* name;
		const char* filename;
		const char* type;
	};

	bool AddNewTree( const STreeDefinition& definition );
	bool DeleteTree( const char* name );
	bool EditTree( const char* name, const STreeDefinition& definition );

	// add/delete/rename refs
	struct SRefGroupDefinition
	{
		const char* name;
		const char* filename;
		const char* varsname;
		const char* signalsname;
		const char* treename;
		bool vars;
		bool signals;
		bool tree;
	};
	bool AddNewRef( const SRefGroupDefinition& definition );
	bool DeleteRef( const char* name );
	bool EditRef( const char* name, const SRefGroupDefinition& definition );

	// subtrees
	bool DeleteTreeFromBlock( const char* name, const char* group );
	bool CreateTreeForBlock( const char* name, const char* group );
	bool RenameTreeForBlock( const char* name, const char* newname, const char* group );

	// Reset all data
	void Reset();

	CSelectionTreeModifier* GetModifier() const { return m_pModifier; }
	// IXmlHistoryEventListener
	virtual void OnEvent( EHistoryEventType event, void* pData = NULL );
	// ~IXmlHistoryEventListener

	// ILevelIndependentFileModule
	virtual bool PromptChanges();
	// ~ILevelIndependentFileModule

	void SetIngame( bool inGame );
	// ISelectionTreeObserver
	virtual void SetSelectionTree(const char* name, const STreeNodeInfo& rootNode);
	virtual void DumpVars(const TVariableStateMap& vars);
	virtual void StartEval();
	virtual void EvalNode(uint16 nodeId);
	virtual void EvalNodeCondition(uint16 nodeId, bool condition);
	virtual void EvalStateCondition(uint16 nodeId, bool condition);
	virtual void StopEval(uint16 nodeId);
	// ~ISelectionTreeObserver

	// CVars
	static ICVar* CV_bst_debug_ai;
	static int CV_bst_debug_centernode;

	enum EWarnLevel
	{
		eWL_All = 0,
		eWL_NoWarningsInRef,
		eWL_NoErrorsInRef,
		eWL_OnlyErrors,
		eWL_OnlyErrorsInTree,
	};
	static int CV_bst_validator_warnlevel;

	static void	OnBSTDebugAIVariableChanged(ICVar*pCVar);
	static void	OnBSTValidatorWarnVariableChanged(ICVar*pCVar);

	// Views
	void SetVarView(CSelectionTreeVariablesView* pVarView) { m_pVarView = pVarView; }
	void SetSigView(CSelectionTreeSignalsView* pSigView) { m_pSigView = pSigView; }
	void SetTreeGraph(CSelectionTreeGraph* pGraph) { m_pGraph = pGraph; }

	CSelectionTreeVariablesView* GetVarView() const { return m_pVarView; };
	CSelectionTreeSignalsView* GetSigView() const { return m_pSigView; };
	CSelectionTreeGraph* GetTreeGraph() const { return m_pGraph; };

	// Error Report
	CSelectionTreeErrorReport::TSelectionTreeErrorId ReportError(const CSelectionTreeErrorRecord& error, bool displayErrorDialog = false);
	void RemoveError(CSelectionTreeErrorReport::TSelectionTreeErrorId id);
	void ReloadErrors();
	void ClearErrors();
	void DisplayErrors();

private:
	SXmlHistoryGroup* GetSelectionTree( const char* name, string* displayName = NULL ) const;
	SXmlHistoryGroup* GetBlockGroup( const char* name, string* displayName = NULL ) const;
	SXmlHistory* GetBlock( const char* name, string* displayName = NULL ) const;

	void LoadFromFolderInt( const char* folder );
	void LoadFromFileNode( const XmlNodeRef& node, const char* filename );
	void LoadTreeXml( const XmlNodeRef& tree, const char* filename );
	void LoadBlockXml( const XmlNodeRef& block, const char* filename, ESelectionTreeTypeId typeId );
	void ResolveBlocks();
	void UpdateFileList(bool reset = false);

	bool ReadGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const;
	bool ReadTreeGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const;
	bool ReadBlockGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const;
	bool ReadBlockInfo( SSelectionTreeInfo& info, ESelectionTreeTypeId type, const SXmlHistory* pData, const XmlNodeRef& nameInfoNode ) const;
	bool LoadTreeInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode, const char* filename );
	bool LoadBlockInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode, int blockindex, const char* filename );
	bool LoadBlockGroupInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode );

public:
	ESelectionTreeTypeId GetBlockType( const XmlNodeRef& xmlNode ) const;
	ESelectionTreeTypeId GetBlockTypeByStr( const char* name ) const;
	const char* GetBlockStrByType(ESelectionTreeTypeId type) const;

private:
	CXmlHistoryManager* m_pXmlHistoryMan;
	CSelectionTreeModifier* m_pModifier;
	CSelectionTreeVariablesView* m_pVarView;
	CSelectionTreeSignalsView* m_pSigView;
	CSelectionTreeGraph* m_pGraph;
	CSelectionTreeErrorReport* m_pErrorReport;
	CSelectionTreeValidator* m_pValiator;
	bool m_bIsLoading;

	typedef std::vector< SXmlHistoryGroup* > TXmlGroupList;
	TXmlGroupList m_XmlGroupList;

	typedef std::pair< string, XmlNodeRef > TXmlNodeSet;
	typedef std::list< TXmlNodeSet >TXmlNodeList;
	TXmlNodeList m_BlocksToResolve;

	typedef std::map< string, int > TFileCountList;
	TFileCountList m_FileList;
};

template<class T>
class CMakeNameUnique
{
public:
	typedef bool (T::*TValidateNameFct) ( const string& name );

	CMakeNameUnique( T* pThis, TValidateNameFct fct ) : m_pThis( pThis), m_Fct(fct) {}

	void MakeNameUnique( string& name )
	{
		if ( m_pThis && m_Fct )
		{
			MakeNameUniqueInt( name, 0 );
		}
	}

private:
	T* m_pThis;
	TValidateNameFct m_Fct;

	void MakeNameUniqueInt( string& name, int appendix )
	{
		string uniqueName = name;
		if ( appendix > 0 )
		{
			uniqueName.Format( "%s_%i", name, appendix );
		}

		if ( ( m_pThis->*m_Fct )( uniqueName ) )
		{
			name = uniqueName;
		}
		else
		{
			MakeNameUniqueInt( name, appendix + 1 );
		}
	}
};

#endif
