#ifndef LiveCreateDialog_h
#define LiveCreateDialog_h

#include "LiveCreate.h"

#include "../Dialogs/BaseFrameWnd.h"
#include "../Controls/SplitterWndEx.h"

#include "../PropertiesPanel.h"

#include "../Animation/SkeletonMapperGraph.h"

class CLiveCreateTreeConnection :
	public CTreeCtrl,
	public ILiveCreateSceneListener
{
public:
	enum EID
	{
		eID = 2000,

		eID_Connection_Connect,
		eID_Connection_Disconnect,
		eID_Connection_SetEntity,

		eID_Actor_SetEntity,

		eID_Actor_MappingGraph_New,
		eID_Actor_MappingGraph_Open,
	};

public:
	CLiveCreateTreeConnection();
	~CLiveCreateTreeConnection();

public:
	void Create(CWnd* pParent, UINT nID);

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnConnection_Connect();
	afx_msg void OnConnection_Disconnect();
	afx_msg void OnConnection_SetEntity();

	afx_msg void OnActor_SetEntity();
	afx_msg void OnActor_MappingGraph_New();
	afx_msg void OnActor_MappingGraph_Open();

private:
	void CreateMenuConnection(CLiveCreateConnection& connection, CMenu& menu);

	CLiveCreateConnection* IsConnection(HTREEITEM hItem);
	CLiveCreateActor* IsActor(HTREEITEM hItem);

	HTREEITEM FindConnectionItem(const CLiveCreateConnection* pConnection);
	HTREEITEM FindConnectionItem(const CLiveCreateScene* pScene);

	IEntity* GetEditorSelectedEntity();

public:
	CLiveCreateConnection* GetSelectedConnection();
	CLiveCreateActor* GetSelectedActor();

	void RefreshConnection(HTREEITEM hConnection, bool bForceExpand = false);
	void RefreshConnections();

	// ILiveCreateSceneListener
public:
	void OnCreateActor(const CLiveCreateScene& scene, const CLiveCreateActor& actor);

private:
	CMenu m_menuConnection;
	CMenu m_menuActor;
	CMenu m_menuMappingGraph;

	// TEMP
public:
	Skeleton::CMapperGraphView* m_pGraphView;
};

class CLiveCreateDialog :
	public CBaseFrameWnd,
	public Skeleton::IMapperGraphViewListener
{
	DECLARE_DYNCREATE(CLiveCreateDialog)

private:
	enum EID
	{
		eID = 2000,

		eID_Tree,

		eID_Connect,
		eID_SetEntity,
	};

public:
	static void RegisterViewClass();

public:
	CLiveCreateDialog();
	virtual ~CLiveCreateDialog();

private:
	void DisplayConnectioProperties(CLiveCreateConnection* pConnection);
	void DisplayActorProperties(CLiveCreateActor* pActor);

protected:
	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnTreeSelection(NMHDR*, LRESULT*);

	// Skeleton::IMapperGraphViewListener
public:
	virtual void OnSelection(const std::vector<CHyperNode*>& previous, const std::vector<CHyperNode*>& current);

private:
	CXTSplitterWnd m_splitters[2];
	CLiveCreateTreeConnection m_tree;
	CPropertiesPanel m_properties;

	CPropertiesPanel m_graphProperties;
	Skeleton::CMapperGraphView m_graphView;
};

#endif // LiveCreateDialog_h
