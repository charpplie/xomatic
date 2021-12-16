#ifndef __DescEditor_h__
#define __DescEditor_h__

#if _MSC_VER > 1000
# pragma once
#endif

#include "..\..\Game_Hunt\GameDll\Game_P1\Core\ObjectDesc.h"     // GameDll
#include "Properties\PropertyGrid.h"														 // PropertyGrid
#include "DescEditorTree.h"                                 // Custom CXTPReportControl
#include "DescSourceControl.h"

namespace CryGame
{
#define DESC_EDITOR_TOOL_NAME           "Desc Editor"

	class CDescEditor;
	class CDescEditorTree;

	//////////////////////////////////////////////////////////////////////////
	// These are directly related to the object desc .xml files.  One exists per .xml file.
	class CDescEdObjectDesc
	{
	public:
		CDescEdObjectDesc() : m_pObjDesc(NULL), m_pRootObjectTreeRecord(NULL), m_pSelectedObjectTreeRecord(NULL) {}
		~CDescEdObjectDesc() {}

		_smart_ptr<CObjectDesc> GetObjectDesc() { return m_pObjDesc; }
		void SetObjectDesc(_smart_ptr<CObjectDesc> pValue) { m_pObjDesc = pValue; }

		CXTPReportRecord* GetObjectTreeRootRecord() const { return m_pRootObjectTreeRecord; }
		void SetObjectTreeRootRecord(CXTPReportRecord* pValue) { m_pRootObjectTreeRecord = pValue; }

		CXTPReportRecord* GetObjectTreeSelectedRecord() const { return m_pSelectedObjectTreeRecord; }
		void SetObjectTreeSelectedRecord(CXTPReportRecord* pValue) { m_pSelectedObjectTreeRecord = pValue; }

	private:
		_smart_ptr<CObjectDesc> m_pObjDesc;
		CXTPReportRecord* m_pRootObjectTreeRecord;
		CXTPReportRecord* m_pSelectedObjectTreeRecord;
	};

	//////////////////////////////////////////////////////////////////////////
	// This is the DescObject class and contains a list of all of the related object desc .xml files
	class CDescEdObjectDescMgr
	{
	public:
		CDescEdObjectDescMgr() : m_pDescClass(NULL) {}
		~CDescEdObjectDescMgr()
		{
			while (!m_pObjDescs.empty())
			{
				delete m_pObjDescs.back();
				m_pObjDescs.pop_back();
			}
		}

		IClass* GetDescClass() { return m_pDescClass; }
		void SetDescClass(IClass* pValue) { m_pDescClass = pValue; }

		std::vector<CDescEdObjectDesc*>& GetObjDescs() { return m_pObjDescs; }
		void RemoveObjDesc(_smart_ptr<CObjectDesc> pDesc)
		{
			std::vector<CDescEdObjectDesc*>::iterator it = m_pObjDescs.begin();
			std::vector<CDescEdObjectDesc*>::iterator end = m_pObjDescs.end();
			CDescEdObjectDesc* pDescMgr;

			while (it != end)
			{
				pDescMgr = *it;
				if (pDescMgr->GetObjectDesc() == pDesc)
				{
					delete pDescMgr;
					m_pObjDescs.erase(it);
					break;
				}
				++it;
			}
		}

	private:
		IClass* m_pDescClass;
		std::vector<CDescEdObjectDesc*> m_pObjDescs;
	};

	//////////////////////////////////////////////////////////////////////////
	// Support class for handling CReflectedObjects displayed on the properties panel and tree controls
	// Every item listed on the property panel ( or desc tree ) has one of these retrieved from GetUserData or GetItemData
	class CDescPropertyInfo
	{
	public:
		CDescPropertyInfo()
			: m_pObject(NULL)
			, m_pProperty(NULL)
			, m_pClass(NULL)
			, m_pParentInfo(NULL)
			, m_pParentDesc(NULL)
			, m_dirty(0)
			, m_pParentRecord(NULL)
			, m_pRecord(NULL)
			, m_pElement(NULL)
			, m_deleted(false)
		{}

		CDescPropertyInfo(_smart_ptr<CReflectedObject> pObject, IClass* pClass = NULL, IProperty* pProperty = NULL, _smart_ptr<CReflectedObject> pElement = NULL, CDescEdObjectDesc* pParentDesc = NULL)
			: m_pObject(pObject)
			, m_pClass(pClass)
			, m_pProperty(pProperty)
			, m_pParentInfo(NULL)
			, m_pParentDesc(pParentDesc)
			, m_dirty(0)
			, m_pParentRecord(NULL)
			, m_pRecord(NULL)
			, m_pElement(pElement)
			, m_deleted(false)
		{
		}

		_smart_ptr<CReflectedObject> GetObject() const { return m_pObject; }
		IProperty* GetProperty() const { return m_pProperty; }
		IClass* GetClass() const { return m_pClass; }
		CDescPropertyInfo* GetParentInfo() const { return m_pParentInfo; }
		CDescEdObjectDesc* GetParentDesc() const { return m_pParentDesc; }
		CXTPReportRecord* GetParentRecord() const { return m_pParentRecord; }
		CXTPReportRecord* GetRecord() const { return m_pRecord; }
		_smart_ptr<CReflectedObject> GetElement() const { return m_pElement; }
		int GetDirtyCount() const { return m_dirty; }
		SPropertyGridState& GetPropertyGridState() { return m_propertyGridState; }
		bool IsDeleted() const { return m_deleted; }

		void SetObject(_smart_ptr<CReflectedObject> pValue) { m_pObject = pValue; }
		void SetProperty(IProperty* pValue) { m_pProperty = pValue; };
		void SetClass(IClass* pValue) { m_pClass = pValue; };
		void SetParentInfo(CDescPropertyInfo* pValue) { m_pParentInfo = pValue; }
		void SetParentDesc(CDescEdObjectDesc* pValue) { m_pParentDesc = pValue; }
		void SetParentRecord(CXTPReportRecord* pValue) { m_pParentRecord = pValue; }
		void SetRecord(CXTPReportRecord* pValue) { m_pRecord = pValue; }
		void SetElement(_smart_ptr<CReflectedObject> pValue) { m_pElement = pValue; }
		void SetDirtyCount(int value) { m_dirty = value; }
		void SetIsDeleted(bool value) { m_deleted = value; }

		int GetElementIndex()
		{
			if (m_pProperty && m_pObject && m_pElement)
			{
				for (int i = 0; i < m_pProperty->GetElementCount(m_pObject); ++i)
				{
					Value element = m_pProperty->GetElementAt(m_pObject, i);
					_smart_ptr<CReflectedObject> object = _smart_ptr<CReflectedObject>(element);
					if (object == m_pElement)
					{
						return i;
					}
				}
			}

			return -1;
		}

		void ResetDirtyCount(bool resetOriginalValue = false)
		{
			m_dirty = 0;

			if (m_pParentInfo)
			{
				m_pParentInfo->ResetDirtyCount();
			}

			m_propertyGridState.ResetDirtyStatus(resetOriginalValue);
		}

		void IncrementDirty(int count = 1)
		{
			m_dirty += count;

			if (m_pParentInfo)
			{
				m_pParentInfo->IncrementDirty(count);
			}
		}

		void DecrementDirty(int count = 1)
		{
			m_dirty -= count;

			if (m_pParentInfo)
			{
				m_pParentInfo->DecrementDirty(count);
			}
		}

	private:
		// Desc related variables
		_smart_ptr<CReflectedObject> m_pObject;          // The owning object of m_pProperty
		IProperty* m_pProperty;                          // Used when m_pProperty is type array
		_smart_ptr<CReflectedObject> m_pElement;         // Used if this is an element in an array
		IClass* m_pClass;                                // Used to create a new object and add it to the owning object/property ( m_pObject/m_pProperty )
		CDescEdObjectDesc* m_pParentDesc;                // Used to track whether we need to save or not.  Set m_pParentDesc->m_dirty to true to indicate save needed

		// Editor related variables
		CXTPReportRecord* m_pParentRecord;               // The selected record in the Desc Tree when the property panel is built
		CXTPReportRecord* m_pRecord;                     // If this is a record itself that contains desc object elements, store it here
		CDescPropertyInfo* m_pParentInfo;                // The parent info of this element.  Used to track m_dirty status

		// PropertyGrid related variables
		SPropertyGridState m_propertyGridState;         // State of the property grid for this item

		int m_dirty;
		bool m_deleted;
	};

	//////////////////////////////////////////////////////////////////////////
	// Wrapper class to allow handling OnSelect from ComboBoxes displayed inside a CXTPDockingPane.  CXTPDockingPane
	// doesn't notify parents when the message lands
	class CDescFilter : public CDialog
	{
	public:
		CDescFilter(CDescEditor* pDescEditor)
			: m_pDescEditor(pDescEditor)
			, m_filterDesc(true)
			, m_filterProperties(false)
			, m_pEditFilter(NULL)
			, m_pCheckBoxDescs(NULL)
			, m_pCheckBoxProperties(NULL)
		{};

		enum
		{
			IDD = IDD_DESC_EDITOR_FILTER
		};

		void DoFilter();

	private:
		void LayoutControls();
		void FilterDescs();
		void FilterProperties();

		CXTPToolBar m_wndToolBar;
		CString m_filterText;
		bool m_filterDesc;
		bool m_filterProperties;
		CDescEditor* m_pDescEditor;
		CXTPControlEdit* m_pEditFilter;
		CXTPControlCheckBox* m_pCheckBoxDescs;
		CXTPControlCheckBox* m_pCheckBoxProperties;

	protected:
		DECLARE_MESSAGE_MAP()
		afx_msg void OnComboBoxSelectDesc();
		afx_msg void OnResetFilter();
		afx_msg void OnFilterText(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnCheckboxDesc();
		afx_msg void OnCheckboxProperties();

	public:
		virtual BOOL OnInitDialog();
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnClose();
	};

	//////////////////////////////////////////////////////////////////////////
	// This contains all of the editor logic to edit properties, save, add, delete, etc

	class CDescEditor : public CBaseFrameWnd, public IEditorNotifyListener, public IPropertyListener
	{
		DECLARE_DYNCREATE(CDescEditor)

	public:
		CDescEditor();   // standard constructor
		virtual ~CDescEditor();

		enum { IDD = IDD_DESCEDITORDIALOG };

		static void RegisterViewClass();

		void OnDescElementSelectionChanged(CDescEditorTree* pTree);
		void OnRButtonUpDescElementPanel(CDescEditorTree* pTree, void* pData, CPoint& rPoint);
		void OnRButtonUpDescTree(CDescEditorTree* pTree, void* pData, CPoint& rPoint);
		void OnRButtonUpObjectTree(CDescEditorTree* pTree, void* pData, CPoint& rPoint);
		void OnRButtonUpClipboardTree(CDescEditorTree* pTree, void* pData, CPoint& rPoint);
		void OnComboBoxSelectDesc();
		void OnPropertyEvent(uint32 event, CPropertyGrid* pGrid, const CPropertyGridData* pData);

		// Updates dirty status of all .XML files under current selected desc
		void UpdateDirtyStatus(CDescEditorTree* pTree);

		// Update the active and desc trees dirty flags on records / items
		void UpdateDirtyStatus(CXTPReportRecord* pRecord = NULL, bool clean = false);

		// Pass in a record to check dirty status on it and all children
		void UpdateDirtyStatusItem(CXTPReportRecord* pRecord, bool clean, bool isDescTreeItem = false);
		
		// Sets item text color based on dirty status
		void UpdateItemDirtyTextColor(CXTPReportRecordItem* pItem, CDescPropertyInfo* pInfo = NULL);

		void CloneDescElement(CDescPropertyInfo* pSourceObject, CDescPropertyInfo* pTargetObject, CDescEditorTree* pTree);

		// Gather all the .XML files for a specific m_objectDescManager desc index then populate the desc tree.
		void GatherDescs(int index, bool useCategories);
		void GatherDirtyDescs(int index);

		// Tree accessors
		void SetActiveTree(CDescEditorTree* pTree);
		CDescEditorTree* GetActiveTree() const { return m_pActiveTree; }
		CDescEditorTree* GetDescTree() const { return m_pTreeDescSelection; }
		CDescEditorTree* GetObjectTree() const { return m_pTreePropertyObjects; }
		CDescEditorTree* GetClipboardTree() const { return m_pTreeClipboard; }

		// Notification from DescEditorTree that the item has changed
		void OnBeforeDescElementSelectionChanged();

		void GetPropertyAncestors(std::map<string, IClass*>& list);
		void GetPropertyElementList(IClass* pFindClass, std::vector<string>& list);
		_smart_ptr<CObjectDesc> GetSelectedObjectDesc();

	protected:
		DECLARE_MESSAGE_MAP()
		virtual BOOL OnInitDialog();
		void OnReset();
		void OnSave();

		//////////////////////////////////////////////////////////////////////////
		// IEditorNotifyListener
		//////////////////////////////////////////////////////////////////////////
		virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
		//////////////////////////////////////////////////////////////////////////

	private:
		CDescSourceControl m_sourceControl;
		std::vector<CDescEdObjectDescMgr*> m_objectDescManager;
		std::vector<CDescPropertyInfo*> m_deletedProperties;

		CXTPDockingPane* m_pDockPaneFilter;
		CXTPDockingPane* m_pDockPaneDescs;
		CXTPDockingPane* m_pDockPanePropertyObjects;
		CXTPDockingPane* m_pDockPanePropertyGrid;
		CXTPDockingPane* m_pDockPaneClipboard;

		CDescFilter* m_pFilterDescTreeBar;						// Desc List filter
		CDescEditorTree* m_pTreeDescSelection;					// ObjectDesc tree that lists the xml files sorted by class
		CDescEditorTree* m_pTreePropertyObjects;				// Object tree that lists the properties that are objects
		CDescEditorTree* m_pTreeClipboard;						// Clipboard tree
		CDescEditorTree* m_pActiveTree;							// Currently active tree
		CPropertyGrid* m_pPropertyGrid;							// Property grid

		_smart_ptr<CReflectedObject> m_pSelectedObject;
		CDescEdObjectDesc* m_pSelectedDesc;
		string m_rootPath;
		CImageList m_icons;

		// Builds the list of valid descs from .XML files and adds them to the selection combo box and m_objectDescManager.
		void BuildDescList();

		// Loads desc from disc then it adds to the vector inside CDescEdObjectDescMgr& rDescMgr
		bool AddDescFile(IClass* pClass, const char* pFilename, CDescEdObjectDescMgr& rDescMgr);

		// Removes specified desc from m_objectDescManager
		bool RemoveDescFile(IClass* pClass, CObjectDesc* pDesc);

		// Creates a new desc .XML and saves it as the specified filename
		bool CreateDescFile(IClass* pClass);

		// Adds valid descs to the desc tree
		// Called when the user selects a desc class from the combo box.  The index is the combo box selection index.
		void BuildTreeDescSelection(int index = -1);

		// Populates the object tree with the properties from the specified desc
		void BuildTreePropertyObjects(CDescEdObjectDesc* pObjectDesc);

		// Pass in a record and info.  All properties will be found and inserted into the record
		void PopulateObjectProperties(CXTPReportRecord* pRecord, CDescPropertyInfo* pInfo);

		// Pass in a record and info.  All sub elements will be found and inserted into the record
		void PopulateObjectPropArrayElements(CXTPReportRecord* pRecord, CDescPropertyInfo* pInfo);

		// Add the child record to the parent record and update the caption on the child.
		CXTPReportRecordItem* AddObjectPropArrayElementChild(CXTPReportRecord* pParentRecord, CDescPropertyInfo* pDescPropertyInfo);

		// Create a new property element then add the element to the tree.
		CDescPropertyInfo* AddObjectPropArrayElement(CDescPropertyInfo* pInfo, IClass* pClass, CDescEditorTree* pTree, int index = -1);

		// Delete a property element
		void DeleteObjectPropArrayElement(CDescPropertyInfo* pInfo);

		// Populate the property grid with the specified property
		void BuildPropertyGrid(CDescPropertyInfo* pInfo);

		// Set the caption of an array property that indicates the number of elements contained
		void SetArrayCaption(CXTPReportRecordItem* pItem, IProperty* pProperty, CReflectedObject* pObject);

		// Return true is the dirty count for the supplied desc != 0
		bool CheckDirtyDesc(CDescEdObjectDesc* pDesc);

		// Calls ResetObjectPropertyTree() then removes all records and cleans up user data for desc tree.
		void ResetDescTree();

		// Calls ResetPropertyGrid() then removes all records and cleans up user data for object tree.
		void ResetObjectPropertyTree();

		// Resets the property grid
		void ResetPropertyGrid();

		// Finds the CDescEdObjectDescMgr for the specified class
		CDescEdObjectDescMgr* FindDescEdObjectDescMgr(IClass* pClass);

		// Removes records for the specified desc from the tree then reloads the desc from disc.
		void ResetDesc(CDescPropertyInfo* pInfo, bool forceReset = false);

		// Find a property array / record to host the source object when dropped on a tree
		CDescPropertyInfo* GetValidCloneTarget(CDescPropertyInfo* pSourceObject, CDescPropertyInfo* pTargetObject);

		// Delete all elements tagged with IsDeleted flag.  Used prior to saving or resetting.
		void ResolveDeletedElements(CObjectDesc* pDesc);

		// Serialize the desc after resolving deleted elements, etc.
		bool SerializeDesc(CObjectDesc* pDesc);

		void RenameDesc(CObjectDesc* pDesc, CDescPropertyInfo* pInfo);
		bool DeleteDesc(CObjectDesc* pDesc);
		void DuplicateDesc(CObjectDesc* pDesc);

		void LayOutControls();
		void CreateDescPopupMenu(CMenu* pMenu, CObjectDesc* pDesc);
		void HandleDescPopupMenu(CMenu* pMenu, CPoint& rPoint, CWnd* pParent);
		int HandlePopupMenu(CMenu* pMenu, CPoint& rPoint, CWnd* pParent);
		bool RequestSave(CObjectDesc* pDesc);
		bool SaveDesc(CObjectDesc* pDesc);
		void SaveAll();
		void ResetAll();
		void ExpandAll(CXTPReportRecord* pRecord);
		void CollapseAll(CXTPReportRecord* pRecord);
		CDescPropertyInfo* GetItemData(CXTPReportRow* pRow);
		CDescPropertyInfo* GetItemData(CXTPReportRecord* pRecord);
		bool BuildClassMenu(CMenu& menu, IClass* pClass, const char* title, bool IsCategories = false);
		void GetObjectCategory(CReflectedObject* pObject, string& category);
		_smart_ptr<CObjectDesc> GetObjectDesc(CXTPReportRecord* pRecord);
		_smart_ptr<CObjectDesc> GetObjectDesc(CXTPReportRecordItem* pItem);

	public:
		afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnDestroy();
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	class CDescEditorViewClass : public TRefCountBase<IViewPaneClass>
	{
		//////////////////////////////////////////////////////////////////////////
		// IClassDesc
		//////////////////////////////////////////////////////////////////////////
	public:
		CDescEditorViewClass() : m_pDescEditor(NULL) {};

		CDescEditor* GetDescEditor() const {return m_pDescEditor;}
		void SetDescEditor(CDescEditor* pDescEditor) {m_pDescEditor = pDescEditor;}

	private:
		virtual ESystemClassID SystemClassID() {return ESYSTEM_CLASS_VIEWPANE;};
		virtual REFGUID ClassID()
		{
			// {737b04ee-c2fa-4986-80bd-5fcadefe7f98} // Can find this in Filters after new dialog is created
			static const GUID guid = {0x737b04ee, 0xc2fa, 0x4986, { 0x80, 0xbd, 0x5f, 0xca, 0xde, 0xfe, 0x7f, 0x98 }};
			return guid;
		}
		virtual const char* ClassName() {return DESC_EDITOR_TOOL_NAME;};
		virtual const char* Category() {return "Game";};
		//////////////////////////////////////////////////////////////////////////
		virtual CRuntimeClass* GetRuntimeClass() {return RUNTIME_CLASS(CDescEditor);};
		virtual const char* GetPaneTitle() {return _T(DESC_EDITOR_TOOL_NAME);};
		virtual EDockingDirection GetDockingDirection() {return DOCK_FLOAT;};
		virtual CRect GetPaneRect() {return CRect(200, 200, 800, 800);};
		virtual bool SinglePane() {return false;};
		virtual bool WantIdleUpdate() {return true;};

		CDescEditor* m_pDescEditor;
	};

}  // namespace CryGame

#endif
