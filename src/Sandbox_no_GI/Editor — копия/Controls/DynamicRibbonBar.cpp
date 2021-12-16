#include "StdAfx.h"
#include "DynamicRibbonBar.h"
#include "ObjectCreateTool.h"
#include "UndoDropDown.h"

void CDynamicRibbonBar::Init( CXTPRibbonBar *pRibbonBar )
{
	m_pRibbonBar = pRibbonBar;

	// Parse Resource.h file.
	ParseResourceFile( "Editor/UI/Resource.h" );
}

//////////////////////////////////////////////////////////////////////////
void CDynamicRibbonBar::MakeCreateTab()
{
	CXTPRibbonTab* pTab = m_pRibbonBar->AddTab("Create");

	std::vector<IClassDesc*> classes;
	CClassFactory::Instance()->GetClassesBySystemID( ESYSTEM_CLASS_OBJECT,classes );
	std::map<CString,int> categoryMap;
	for (int i = 0; i < classes.size(); i++)
	{
		const char *category = classes[i]->Category();
		if (strlen(category) > 0)
			categoryMap[category]++;
	}

	{
		CXTPRibbonGroup* pGroup = pTab->AddGroup( "Default" );
		for (int i = 0; i < classes.size(); i++)
		{
			int numObjectsInCategory = stl::find_in_map(categoryMap,classes[i]->Category(),0);
			if (numObjectsInCategory == 1)
			{
				int nCmdId = MakeCreateCommand( classes[i]->ClassName() );
				CXTPControl *pCtrl = pGroup->Add(xtpControlButton,nCmdId,NULL,-1,TRUE);
				pCtrl->SetStyle(xtpButtonCaption);
				pCtrl->SetCaption( classes[i]->ClassName() );
			}
		}
	}

	for (std::map<CString,int>::iterator cit = categoryMap.begin(); cit != categoryMap.end(); ++cit)
	{
		CString category = cit->first;
		int numObjectsInCategory = cit->second;
		if (numObjectsInCategory < 2)
			continue;

		CXTPRibbonGroup* pGroup = pTab->AddGroup( category );
		for (int i = 0; i < classes.size(); i++)
		{
			if (strcmp(category,classes[i]->Category()) == 0)
			{
				int nCmdId = MakeCreateCommand( classes[i]->ClassName() );
				CXTPControl *pCtrl = pGroup->Add(xtpControlButton,nCmdId,NULL,-1,TRUE);
				pCtrl->SetStyle(xtpButtonCaption);
				pCtrl->SetCaption( classes[i]->ClassName() );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDynamicRibbonBar::ParseResourceFile( const char *filename )
{
	CCryFile file;
	if (!file.Open(filename,"rb"))
		return;


	int nSize = file.GetLength();
	char *pBuffer = new char[nSize+16];
	memset( pBuffer,0,nSize );
	file.ReadRaw( pBuffer,nSize );

	CString str = pBuffer;

	int curLinePos = 0;
	CString line = str.Tokenize("\n",curLinePos);
	while (line != "")
	{
		if (strncmp(line,"#define",7) == 0)
		{
			CString resourceName;
			int resourceId = 0;
			int nIndex = 0;
			int curPos = 0;
			CString token = line.Tokenize(" ",curPos);
			while (token != "")
			{
				if (nIndex == 1)
				{
					resourceName = token;
				}
				else if (nIndex == 2)
				{
					resourceId = atoi(token);
				}
				nIndex++;
				token = line.Tokenize(" ",curPos);
			}
			if (resourceId != 0)
			{
				m_resourceNameToIdMap[resourceName] = resourceId;
			}
		}
		line = str.Tokenize("\n",curLinePos);
	};

	delete []pBuffer;
}

//////////////////////////////////////////////////////////////////////////
void CDynamicRibbonBar::LoadFromFile( const CString &filename )
{
	XmlNodeRef root = GetISystem()->LoadXmlFile( filename );
	if (root)
		LoadFromXML( root );
}

//////////////////////////////////////////////////////////////////////////
void CDynamicRibbonBar::LoadFromXML( XmlNodeRef &root )
{
	XmlNodeRef tabsNode = root->findChild( "Tabs" );
	
	// Load tabs.
	{
		for (int nTab = 0; nTab < tabsNode->getChildCount(); nTab++)
		{
			XmlNodeRef tabNode = tabsNode->getChild(nTab);
			if (!tabNode->isTag("Tab"))
				continue;

			LoadTab( tabNode );
		}
	}
}

void CDynamicRibbonBar::LoadTab( XmlNodeRef &tabNode )
{
	  //  <Tab Caption="&amp;Home" Id="130" Selected="1">

	CString caption;
	tabNode->getAttr( "Caption",caption );

	CXTPRibbonTab* pTab = m_pRibbonBar->AddTab(caption);
	
	// Load Groups.
	XmlNodeRef groupNode = tabNode->findChild( "Groups" );
	if (!groupNode)
		return;
	for (int i = 0; i < groupNode->getChildCount(); i++)
	{
		XmlNodeRef node = groupNode->getChild(i);
		if (!node->isTag("Group"))
			continue;
		LoadGroup( pTab,node );
	}
}

void CDynamicRibbonBar::LoadGroup( CXTPRibbonTab* pTab,XmlNodeRef &groupNode )
{
	CString caption;
	int nControlGrouping = 0;
	int nControlCentering = 0;
	groupNode->getAttr( "Caption",caption );
	groupNode->getAttr( "ControlGrouping",nControlGrouping );
	groupNode->getAttr( "ControlCenetring",nControlCentering );
	
	CXTPRibbonGroup* pGroup = pTab->AddGroup(caption);

	pGroup->SetControlsGrouping( (nControlGrouping!=0)?TRUE:FALSE );
	pGroup->SetControlsCentering( (nControlCentering!=0)?TRUE:FALSE );

	//pGroup->SetID( GetIEditor()->GetCommandManager()->GetNewCommandId() );

	// Load Controls.
	XmlNodeRef controlsNode = groupNode->findChild( "Controls" );
	if (!controlsNode)
		return;
	for (int i = 0; i < controlsNode->getChildCount(); i++)
	{
		XmlNodeRef node = controlsNode->getChild(i);
		if (!node->isTag("Control"))
			continue;
		LoadControl( pGroup,node );
	}
}

//////////////////////////////////////////////////////////////////////////
void CDynamicRibbonBar::LoadControl( CXTPRibbonGroup *pGroup,XmlNodeRef &controlNode )
{
	// ex: <Control Class="CXTPControlButton" Id="57600" Caption="New" TooltipText="New" DescriptionText="Create a new document"/>

	int nCtrlId = 0;
	int nIconIndex = 0;
	int nStyle = xtpButtonAutomatic;
	int nSeprator = 0;
	int nIconOnly = 0;
	int nChecked = 0;
	CString resourceId,caption,tooltip,description,command,icon;
	CString typeName("Button");
	CString styleName("");

	controlNode->getAttr( "Id",resourceId );
	controlNode->getAttr( "Command",command );
	controlNode->getAttr( "Caption",caption );
	controlNode->getAttr( "Type",typeName );
	controlNode->getAttr( "TooltipText",tooltip );
	controlNode->getAttr( "DescriptionText",description );
	controlNode->getAttr( "Icon",icon );
	controlNode->getAttr( "IconIndex",nIconIndex );
	controlNode->getAttr( "Style",styleName );
	controlNode->getAttr( "Separator",nSeprator );
	controlNode->getAttr( "Checked",nChecked );

	//////////////////////////////////////////////////////////////////////////
	if (0 == strcmp(styleName,"Caption"))
	{
		nStyle = xtpButtonCaption;
	}
	else if (0 == strcmp(styleName,"Icon"))
	{
		nStyle = xtpButtonIcon;
	}
	else if (0 == strcmp(styleName,"IconAndCaption"))
	{
		nStyle = xtpButtonIconAndCaption;
	}
	else if (0 == strcmp(styleName,"IconAndCaptionBelow"))
	{
		nStyle = xtpButtonIconAndCaptionBelow;
	}
	else if (0 == strcmp(styleName,"CaptionAndDescription"))
	{
		nStyle = xtpButtonCaptionAndDescription;
	}
	//////////////////////////////////////////////////////////////////////////


	if (resourceId.IsEmpty())
	{
		CCommand::UIInfo cmdUI;
		cmdUI.caption = caption;
		cmdUI.tooltip = tooltip;
		cmdUI.description = description;
		cmdUI.iconFilename = icon;
		cmdUI.nIconIndex = nIconIndex;

		GetIEditor()->GetCommandManager()->AttachUI( command,cmdUI );
		nCtrlId = cmdUI.nCommandId;
	}
	else
	{
		nCtrlId = stl::find_in_map(m_resourceNameToIdMap,resourceId,0);
		if (0 == nCtrlId)
		{
			Warning( "Resource Control ID=%s not found in resource map",(const char*)resourceId );
		}
		if (!icon.IsEmpty())
		{
			// If Icon filename specified force Register Icon.
			GetIEditor()->GetIconManager()->RegisterCommandIcon( icon,nCtrlId );
		}
	}

	CXTPControl *pCtrl = 0;

//	RUNTIME_CLASS(
	if (0 == strcmp(typeName,"Button"))
	{
		pCtrl = pGroup->Add(xtpControlButton,nCtrlId,NULL,-1,TRUE);
	}
	else if (0 == strcmp(typeName,"ButtonPopup"))
	{
		pCtrl = pGroup->Add(xtpControlButtonPopup,nCtrlId,NULL,-1,TRUE);
		//pCtrl = pGroup->Add(new CCustomControlButtonPopup,nCtrlId);
	}
	else if (0 == strcmp(typeName,"SplitButtonPopup"))
	{
		pCtrl = pGroup->Add(xtpControlSplitButtonPopup,nCtrlId,NULL,-1,TRUE);
		//pCtrl = pGroup->Add(new CCustomControlSplitButtonPopup,nCtrlId);
	}
	else if (0 == strcmp(typeName,"ComboBox"))
	{
		pCtrl = pGroup->Add(xtpControlComboBox,nCtrlId,NULL,-1,TRUE);

		//CXTPControlComboBox* pControlCombo = (CXTPControlComboBox*)pCtrl;
		//pControlCombo->GetCommandBar()->SetCommandBars( m_pRibbonBar->GetCommandBars() );
		//pControlCombo->SetStyle(xtpButtonIcon);

		CXTPControlComboBox* pControlCombo = (CXTPControlComboBox*)pCtrl;
		pControlCombo->ModifyListBoxStyle(0, LBS_HASSTRINGS);
		//pControlCombo->SetWidth(100);
		pControlCombo->SetStyle(xtpButtonUndefined);
		pControlCombo->SetDropDownListStyle();
	}
	else if (0 == strcmp(typeName,"Edit"))
	{
		pCtrl = pGroup->Add(xtpControlEdit,nCtrlId,NULL,-1,TRUE);
	}
	else if (0 == strcmp(typeName,"CheckBox"))
	{
		pCtrl = pGroup->Add(xtpControlCheckBox,nCtrlId,NULL,-1,TRUE);
	}
	else if (0 == strcmp(typeName,"RadioButton"))
	{
		pCtrl = pGroup->Add(xtpControlRadioButton,nCtrlId,NULL,-1,TRUE);
	}
	else if (0 == strcmp(typeName,"Gallery"))
	{
		pCtrl = pGroup->Add(xtpControlGallery,nCtrlId,NULL,-1,TRUE);
	}
#ifdef _XTP_INCLUDE_MARKUP
	else if (0 == strcmp(typeName,"MarkupLabel"))
	{
		pCtrl = pGroup->Add(xtpControlGallery,nCtrlId,NULL,-1,TRUE);
		CXTPControlMarkupLabel* pLabel = new CXTPControlMarkupLabel();
		pGroup->Add(pLabel, nCtrlId);
		pLabel->SetBeginGroup(TRUE);
	}
#endif
	
	if (pCtrl)
	{
		if (pCtrl->GetCommandBar())
			pCtrl->GetCommandBar()->SetCommandBars( m_pRibbonBar->GetCommandBars() );

		if (nSeprator)
			pCtrl->SetBeginGroup(TRUE);

		if (nStyle != 0)
			pCtrl->SetStyle( (XTPButtonStyle)nStyle );

		if (!caption.IsEmpty())
			pCtrl->SetCaption( caption );
		if (!description.IsEmpty())
			pCtrl->SetDescription( description );
		if (!tooltip.IsEmpty())
			pCtrl->SetTooltip( tooltip );

		if (nChecked)
			pCtrl->SetChecked(nChecked);
	}

	if (caption == "Layer")
	{
		//CXTPRibbonTab* pTabEditors = pRibbonBar->AddTab("MyNewtab");
		//CXTPRibbonGroup *pGroup1 = pGroup->GetParentTab()->AddGroup("Test");
		/*
		CXTPControlComboBox* pControlCombo = (CXTPControlComboBox*)pGroup1->Add(xtpControlComboBox, IDC_SELECTION_MASK+1);

		pControlCombo->GetCommandBar()->SetCommandBars(m_pRibbonBar->GetCommandBars());
		pControlCombo->ModifyListBoxStyle(0, LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);
		pControlCombo->SetWidth(150);
		pControlCombo->SetStyle(xtpButtonIcon);
		pControlCombo->SetDropDownListStyle();
		pControlCombo->SetEditText(_T("ComboBoxLals test"));
		pControlCombo->AddString(_T("Lala 1"));
		pControlCombo->AddString(_T("Lala 2"));
		*/
	}
}

//////////////////////////////////////////////////////////////////////////
class CEditorObjectCreateCommand : public CCommand
{
	CString m_className;
public:
	CEditorObjectCreateCommand( const CString &cmdName,const CString &className )
		: CCommand(cmdName,0)
	{
		m_className = className;
	}
	virtual void Execute()
	{
		// Create browse mode for this category.
		CObjectCreateTool *tool = new CObjectCreateTool;
		GetIEditor()->SetEditTool( tool );
		tool->StartCreation( m_className );
	}
};

//////////////////////////////////////////////////////////////////////////
int CDynamicRibbonBar::MakeCreateCommand( const CString &className )
{
	// Make a create a command for that object class.
	CString createCommand = CString("Editor.CreateObject.") + className;
	CCommand *pCmd = new CEditorObjectCreateCommand( createCommand,className );

	CCommand::UIInfo cmdUI;
	cmdUI.caption = className;
	cmdUI.tooltip = CString("Create") + className;
	//cmdUI.description;
	cmdUI.iconFilename = className;
	//cmdUI.nIconIndex = nIconIndex;

	GetIEditor()->GetCommandManager()->RegisterCommand( pCmd );
	GetIEditor()->GetCommandManager()->AttachUI( createCommand,cmdUI );

	return cmdUI.nCommandId;
}

//////////////////////////////////////////////////////////////////////////
class CEditorOpenViewCommand : public CCommand
{
	CString m_className;
public:
	CEditorOpenViewCommand( const CString &cmdName,const CString &className )
		: CCommand(cmdName,0)
	{
		m_className = className;
	}
	virtual void Execute()
	{
		// Create browse mode for this category.
		static_cast<CMainFrame*>(AfxGetMainWnd())->OpenPage( m_className );
	}
};

//////////////////////////////////////////////////////////////////////////
void CDynamicRibbonBar::MakeViewTab()
{
	// Find view tab
	CXTPRibbonTab* pTab = 0;
	for (int i = 0; i < m_pRibbonBar->GetTabCount(); i++)
	{
		CString cap = m_pRibbonBar->GetTab(i)->GetCaption();
		if (strstr(m_pRibbonBar->GetTab(i)->GetCaption(),"View") != 0)
		{
			pTab = m_pRibbonBar->GetTab(i);
			break;
		}
	}
	if (!pTab)
	{
		pTab = m_pRibbonBar->AddTab("View");
	}

	//CXTPRibbonGroup* pGroupDefault = pTab->AddGroup( "Default" );


	//------------------------------------------------------------------------------
	// Use a map to sort menu items alphabetically
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID( ESYSTEM_CLASS_VIEWPANE,classes );

	std::map<CString, IViewPaneClass*> menuMap;
	std::map<CString, CXTPRibbonGroup*> groupMap;

	size_t classSize = classes.size();
	for (size_t i = 0; i < classSize; ++i)
	{
		IClassDesc *pClass = classes[i];
		IViewPaneClass *pViewClass = NULL;
		if (!FAILED(pClass->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewClass )))
		{
			menuMap[pViewClass->GetPaneTitle()] = pViewClass;
		}
	}

	

	//------------------------------------------------------------------------------
	uint32 ind =0;
	for(std::map<CString, IViewPaneClass*>::iterator it = menuMap.begin(); it != menuMap.end(); ++it)
	{
		CString menuTitle = it->second->GetPaneTitle();
		CString className = it->second->ClassName();
		CString category = it->second->Category();

		CXTPRibbonGroup *pGroup = stl::find_in_map( groupMap,category,0 );
		if (!pGroup)
		{
			pGroup = pTab->AddGroup( category );
			groupMap[category] = pGroup;
		}

		int nCmdId = 0;
		{
			// Make a create a command for that object class.
			CString createCommand = CString("Editor.OpenView.") + className;
			CCommand *pCmd = new CEditorOpenViewCommand( createCommand,className );

			CCommand::UIInfo cmdUI;
			cmdUI.caption = className;
			cmdUI.tooltip = CString("Create") + className;
			//cmdUI.description _T("");
			cmdUI.iconFilename = "mine";
			//cmdUI.nIconIndex = nIconIndex;

			GetIEditor()->GetCommandManager()->RegisterCommand( pCmd );
			GetIEditor()->GetCommandManager()->AttachUI( createCommand,cmdUI );
			nCmdId = cmdUI.nCommandId;
		}

		CXTPControl *pCtrl = pGroup->Add(xtpControlButton,nCmdId,NULL,-1,TRUE);
		pCtrl->SetStyle(xtpButtonAutomatic);
		pCtrl->SetCaption( menuTitle );

		if (strcmp(category,"Viewport") == 0)
		{
			pCtrl->SetStyle(xtpButtonIcon);

		}

		++ind;
	}
}
