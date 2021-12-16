////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MannequinDialog.h"

#include "SequencerNode.h"

#include "ViewPane.h"
#include "StringDlg.h"
#include "NumberDlg.h"

#include "MannequinModelViewport.h"

#include <IGameFramework.h>

#include <ICryMannequinEditor.h>

#include "FragmentEditor.h"
#include "FragmentTrack.h"

#include "MannErrorReportDialog.h"
#include "Helper/MannequinFileChangeWriter.h"
#include "SequencerSequence.h"
#include "SequenceAnalyzerNodes.h"
#include "MannTagDefEditorDialog.h"
#include "MannContextEditorDialog.h"
#include "MannAnimDBEditorDialog.h"

#include "Controls/FragmentBrowser.h"
#include "Controls/SequenceBrowser.h"
#include "Controls/FragmentEditorPage.h"
#include "Controls/TransitionEditorPage.h"
#include "Controls/PreviewerPage.h"
#include "Objects/ObjectLayerManager.h"
#include "../objects/EntityObject.h"


//////////////////////////////////////////////////////////////////////////

#define MANNEQUIN_EDITOR_VERSION "1.00"
#define MANNEQUIN_EDITOR_TOOL_NAME "Mannequin Editor"
#define MANNEQUIN_LAYOUT_SECTION _T("MannequinLayout")

const int CMannequinDialog::s_minPanelSize = 200;

//////////////////////////////////////////////////////////////////////////
void SetMannequinDialogKeyPropertyCmd( IConsoleCmdArgs* pArgs )
{
	if ( pArgs->GetArgCount() < 3 )
	{
		return;
	}

	const char* propertyName = pArgs->GetArg( 1 );
	const char* propertyValue = pArgs->GetArg( 2 );

	CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();

	assert( pMannequinDialog != NULL );
	pMannequinDialog->SetKeyProperty( propertyName, propertyValue );
}

//////////////////////////////////////////////////////////////////////////
class CMannequinPaneClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {D7777B3D-B21E-4D8A-9AEE-929317B4B16D}
		static const GUID guid = { 0xd7777b3d, 0xb21e, 0x4d8a, { 0x9a, 0xee, 0x92, 0x93, 0x17, 0xb4, 0xb1, 0x6d } };
		return guid;
	}
	virtual const char* ClassName() { return MANNEQUIN_EDITOR_TOOL_NAME; };
	virtual const char* Category() { return "Animation"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CMannequinDialog); };
	virtual const char* GetPaneTitle() { return _T(MANNEQUIN_EDITOR_TOOL_NAME); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,500,300); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};


//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CMannequinPaneClass );
	GetIEditor()->GetSettingsManager()->AddToolName(MANNEQUIN_LAYOUT_SECTION, MANNEQUIN_EDITOR_TOOL_NAME);
	GetIEditor()->GetSettingsManager()->AddToolVersion(MANNEQUIN_EDITOR_TOOL_NAME, MANNEQUIN_EDITOR_VERSION);
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMannequinDialog, CBaseFrameWnd)

CMannequinDialog *CMannequinDialog::s_pMannequinDialog = NULL;

//////////////////////////////////////////////////////////////////////////
CMannequinDialog::CMannequinDialog(CWnd* pParent /*=NULL*/)
	: CBaseFrameWnd()
	, m_pFileChangeWriter(new CMannequinFileChangeWriter())
{
	s_pMannequinDialog = this;

	GetIEditor()->RegisterNotifyListener(this);

	CRect rc(0,0,1024,768);
	Create( WS_CHILD|WS_VISIBLE,rc,AfxGetMainWnd() );

	SEventLog toolEvent(MANNEQUIN_EDITOR_TOOL_NAME,"",MANNEQUIN_EDITOR_VERSION);
	GetIEditor()->GetSettingsManager()->RegisterEvent(toolEvent);

	GetIEditor()->GetObjectManager()->GetLayersManager()->AddUpdateListener( functor(*this,&CMannequinDialog::OnLayerUpdate) );

	REGISTER_COMMAND("e_mannequin_setkeyproperty", (ConsoleCommandFunc)SetMannequinDialogKeyPropertyCmd, VF_CHEAT, "");
}

CMannequinDialog::~CMannequinDialog()
{
	s_pMannequinDialog = 0;
	GetIEditor()->UnregisterNotifyListener(this);
	GetIEditor()->GetObjectManager()->GetLayersManager()->RemoveUpdateListener( functor(*this,&CMannequinDialog::OnLayerUpdate) );

	for (uint32 i=0; i<eMEM_Max; i++)
	{
		SMannequinContextViewData &viewData = m_contexts.viewData[i];

		if (viewData.m_pEntity)
		{
			gEnv->pEntitySystem->RemoveEntity(viewData.m_pEntity->GetId());
			viewData.m_pEntity = NULL;
		}

		uint32 numContexts = m_contexts.m_contextData.size();
		for (uint32 j=0; j<numContexts; j++)
		{
			SScopeContextData &contextData = m_contexts.m_contextData[j];
			SScopeContextViewData &contextViewData = contextData.viewData[i];

			if (contextViewData.entity)
			{
				gEnv->pEntitySystem->RemoveEntity(contextViewData.entity->GetId());
				contextViewData.entity = NULL;
			}
		}
	}

	ClearContextEntities();
	ClearContextViewData();
}


//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnLayerUpdate( int event,CObjectLayer *pLayer )
{
	switch (event)
	{
	case CObjectLayerManager::ON_LAYER_MODIFY:
		{
			//UpdateSequenceLockStatus();
		}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CMannequinDialog, CBaseFrameWnd)
	ON_COMMAND( ID_FILE_LOADPREVIEWSETUP, OnMenuLoadPreviewFile )
	ON_COMMAND( ID_FILE_CONTEXTEDITOR, OnContexteditor)
	ON_COMMAND( ID_FILE_ANIMDBEDITOR, OnAnimDBEditor)
	ON_COMMAND( ID_FILE_TAGDEFINITIONEDITOR, OnTagDefinitionEditor )
	ON_COMMAND( ID_FILE_SAVEALLCHANGES, OnSave )
	ON_COMMAND( ID_FILE_REEXPORTALLFILES, OnReexportAll )

	ON_COMMAND( ID_PREVIEWER_NEWSEQUENCE, OnNewSequence )
	ON_COMMAND( ID_PREVIEWER_LOADSEQUENCE, OnLoadSequence )
	ON_COMMAND( ID_PREVIEWER_SAVESEQUENCE, OnSaveSequence )

	ON_COMMAND( ID_VIEW_FRAGMENTEDITOR, OnViewFragmentEditor )
	ON_COMMAND( ID_VIEW_PREVIEWER, OnViewPreviewer )
	ON_COMMAND( ID_VIEW_TRANSITIONEDITOR, OnViewTransitionEditor )
	ON_COMMAND( ID_VIEW_ERRORREPORT, OnViewErrorReport )

	ON_UPDATE_COMMAND_UI( ID_PREVIEWER_NEWSEQUENCE, OnUpdateNewSequence )
	ON_UPDATE_COMMAND_UI( ID_PREVIEWER_LOADSEQUENCE, OnUpdateLoadSequence )
	ON_UPDATE_COMMAND_UI( ID_PREVIEWER_SAVESEQUENCE, OnUpdateSaveSequence )
	ON_UPDATE_COMMAND_UI( ID_VIEW_FRAGMENTEDITOR, OnUpdateViewFragmentEditor )
	ON_UPDATE_COMMAND_UI( ID_VIEW_PREVIEWER, OnUpdateViewPreviewer )
	ON_UPDATE_COMMAND_UI( ID_VIEW_TRANSITIONEDITOR, OnUpdateViewTransitionEditor )
	ON_UPDATE_COMMAND_UI( ID_VIEW_ERRORREPORT, OnUpdateViewErrorReport )

	ON_COMMAND( ID_MANNEQUIN_TOOLS_LISTUSEDANIMATIONSCURRENTPREVIEW, OnToolsListUsedAnimationsForCurrentPreview )
	ON_COMMAND( ID_MANNEQUIN_TOOLS_LISTUSEDANIMATIONS, OnToolsListUsedAnimations )

	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
bool CMannequinDialog::LoadPreviewFile(const char *filename, XmlNodeRef &xmlSequenceNode)
{
	XmlNodeRef xmlData = GetISystem()->LoadXmlFromFile( filename );
	IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

	m_wndErrorReport.Clear();

	if (xmlData && (strcmp(xmlData->getTag(), "MannequinPreview") == 0))
	{
		XmlNodeRef xmlControllerDef	 = xmlData->findChild("controllerDef");
		if (!xmlControllerDef)
		{
			CryLog("[CMannequinDialog::LoadPreviewFile] <ControllerDef> missing in %s", filename);
			return false;
		}
		XmlNodeRef xmlContexts			 = xmlData->findChild("contexts");

		xmlSequenceNode = xmlData->findChild("History");
		if (!xmlSequenceNode)
		{
			xmlSequenceNode = XmlHelpers::CreateXmlNode("History");
			xmlSequenceNode->setAttr("StartTime", 0);
			xmlSequenceNode->setAttr("EndTime", 0);
		}
		m_historyBackup = xmlSequenceNode->clone();

		const SControllerDef *controllerDef = mannequinSys.GetAnimationDatabaseManager().LoadControllerDef(xmlControllerDef->getAttr("filename"));
		if (!controllerDef)
		{
			CryLog("[CMannequinDialog::LoadPreviewFile] Invalid controllerDef file %s", xmlControllerDef->getAttr("filename"));
			return false;
		}

		const uint32 numContexts	 = xmlContexts ? xmlContexts->getChildCount() : 0;
		const uint32 numScopes		 = controllerDef->m_scopeIDs.GetNum();

		std::vector<SScopeContextData> newContextData;
		newContextData.reserve(numContexts);
		
		for (uint32 i=0; i<numContexts; i++)
		{
			SScopeContextData contextDef;
			XmlNodeRef xmlContext = xmlContexts->getChild(i);
			contextDef.name = xmlContext->getAttr("name");
			contextDef.startActive = false;
			xmlContext->getAttr("enabled", contextDef.startActive);
			for (uint32 j=0; j<eMEM_Max; j++)
			{
				contextDef.viewData[j].enabled = contextDef.startActive;
			}
			const char *databaseFilename = xmlContext->getAttr("database");
			const bool hasDatabase = (databaseFilename && (*databaseFilename != '\0'));
			if (hasDatabase)
			{
				contextDef.database = const_cast<IAnimationDatabase *>(mannequinSys.GetAnimationDatabaseManager().Load(databaseFilename));
			}
			else
			{
				contextDef.database = NULL;
			}
			contextDef.dataID = i;
			const char *contextData = xmlContext->getAttr("context");
			if (!contextData || !contextData[0])
			{
				contextDef.contextID = CONTEXT_DATA_NONE;
			}
			else
			{
				contextDef.contextID = controllerDef->m_scopeContexts.Find(contextData);
				if (contextDef.contextID == -1)
				{
					CryLog("[CMannequinDialog::LoadPreviewFile] Invalid preview file %s, couldn't find context scope %s in controllerDef %s", filename, xmlContext->getAttr("context"), xmlControllerDef->getAttr("filename"));
					return false;
				}
			}
			contextDef.changeCount = 0;
			const char *tagList = xmlContext->getAttr("tags");
			controllerDef->m_tags.TagListToFlags(tagList, contextDef.tags);
			contextDef.fragTags = xmlContext->getAttr("fragtags");
			const char *modelFName = xmlContext->getAttr("model");

			const char *controllerDefFilename = xmlContext->getAttr("controllerDef");
			if (controllerDefFilename && controllerDefFilename[0])
			{
				contextDef.pControllerDef = mannequinSys.GetAnimationDatabaseManager().LoadControllerDef(controllerDefFilename);
				if (!contextDef.pControllerDef)
				{
					CryLog("[CMannequinDialog::LoadPreviewFile] Invalid controllerDef file %s", controllerDefFilename);
					return false;
				}
			}
			const char *slaveDatabaseFilename = xmlContext->getAttr("slaveDatabase");
			if (slaveDatabaseFilename && slaveDatabaseFilename[0])
			{
				contextDef.pSlaveDatabase = mannequinSys.GetAnimationDatabaseManager().Load(slaveDatabaseFilename);
				if (!contextDef.pSlaveDatabase)
				{
					CryLog("[CMannequinDialog::LoadPreviewFile] Invalid slave database file %s", slaveDatabaseFilename);
					return false;
				}
			}

			if (hasDatabase && !contextDef.database)
			{
				CString strBuffer;
				strBuffer.Format("Missing database %s create?", databaseFilename);

				if (MessageBox(strBuffer, "Create new ADB?", MB_OKCANCEL) == IDOK)
				{
					contextDef.database = mannequinSys.GetAnimationDatabaseManager().Create(databaseFilename, controllerDef->m_filename.c_str());
				}
				else
				{
					return false;
				}
			}

			if (i==0)
			{
				m_wndFragmentEditorPage.ModelViewport()->LoadObject(modelFName);
				m_wndTransitionEditorPage.ModelViewport()->LoadObject(modelFName);
				m_wndPreviewerPage.ModelViewport()->LoadObject(modelFName);

				if (m_wndFragmentEditorPage.ModelViewport()->GetCharacterBase() == NULL)
				{
					CString strBuffer;
					strBuffer.Format("[CMannequinDialog::LoadPreviewFile] Invalid file name %s, couldn't find character instance", modelFName);

					MessageBox(strBuffer, "Missing file", MB_OK);
					return false;
				}

				contextDef.SetCharacter(eMEM_FragmentEditor, m_wndFragmentEditorPage.ModelViewport()->GetCharacterBase());
				contextDef.SetCharacter(eMEM_TransitionEditor, m_wndTransitionEditorPage.ModelViewport()->GetCharacterBase());
				contextDef.SetCharacter(eMEM_Previewer, m_wndPreviewerPage.ModelViewport()->GetCharacterBase());

				contextDef.startLocation.SetIdentity();
			}
			else if(modelFName && modelFName[0])
			{
				const char* ext = PathUtil::GetExt(modelFName);
				const bool isCharInst = ((stricmp(ext, "chr") == 0) || (stricmp(ext, "cdf") == 0) || (stricmp(ext, "cga") == 0) );

				if (isCharInst)
				{
					if (!contextDef.CreateCharInstance(modelFName))
					{
						CString strBuffer;
						strBuffer.Format("[CMannequinDialog::LoadPreviewFile] Invalid file name %s, couldn't find character instance", modelFName);

						MessageBox(strBuffer, "Missing file", MB_OK);

						continue;
					}
				}
				else
				{
					if (!contextDef.CreateStatObj(modelFName))
					{
						CString strBuffer;
						strBuffer.Format("[CMannequinDialog::LoadPreviewFile] Invalid file name %s, couldn't find stat obj", modelFName);

						MessageBox(strBuffer, "Missing file", MB_OK);

						continue;
					}
				}

				const char *attachment = xmlContext->getAttr("attachment");
				contextDef.attachment = attachment;

				const char *attachmentContext = xmlContext->getAttr("attachmentContext");
				contextDef.attachmentContext = attachmentContext;

				const char *attachmentHelper = xmlContext->getAttr("attachmentHelper");
				contextDef.attachmentHelper = attachmentHelper;

				contextDef.startLocation.SetIdentity();
				xmlContext->getAttr("startPosition", contextDef.startLocation.t);
				if (xmlContext->getAttr("startRotation", contextDef.startRotationEuler))
				{
					Ang3 radRotation(DEG2RAD(contextDef.startRotationEuler.x), DEG2RAD(contextDef.startRotationEuler.y), DEG2RAD(contextDef.startRotationEuler.z));
					contextDef.startLocation.q.SetRotationXYZ(radRotation);
				}
			}

			if (contextDef.database)
			{
				Validate(contextDef);
			}

			newContextData.push_back(contextDef);
		}

		m_contexts.backGroundObjects = NULL;
		m_contexts.backgroundProps.clear();
		XmlNodeRef xmlBackgroundNode = xmlData->findChild("Background");
		if (xmlBackgroundNode)
		{
			XmlNodeRef xmlBackgroundObjects = xmlBackgroundNode->findChild("Objects");
			if (xmlBackgroundObjects)
			{
				m_contexts.backGroundObjects = xmlBackgroundObjects->clone();
			}
			xmlBackgroundNode->getAttr("Pos", m_contexts.backgroundRootTran.t);
			xmlBackgroundNode->getAttr("Rotate", m_contexts.backgroundRootTran.q);

			XmlNodeRef xmlBackgroundProps = xmlBackgroundNode->findChild("Props");
			if (xmlBackgroundProps)
			{
				const uint32 numProps = xmlBackgroundProps->getChildCount();
				for (uint32 i=0; i<numProps; i++)
				{
					XmlNodeRef xmlProp = xmlBackgroundProps->getChild(i);

					SMannequinContexts::SProp prop;
					prop.name = xmlProp->getAttr("Name");
					prop.entity = xmlProp->getAttr("Entity");

					m_contexts.backgroundProps.push_back(prop);
				}
			}
		}

		ClearContextViewData();

		m_contexts.previewFilename = filename;
		m_contexts.m_controllerDef = controllerDef;
		m_contexts.m_contextData   = newContextData;
		m_contexts.m_scopeData.resize(numScopes);
		for (uint32 i=0; i<numScopes; i++)
		{
			SScopeData &scopeDef = m_contexts.m_scopeData[i];

			scopeDef.name					= controllerDef->m_scopeIDs.GetTagName(i);
			scopeDef.scopeID			= i;
			scopeDef.contextID		= controllerDef->m_scopeDef[i].context;
			scopeDef.layer				= controllerDef->m_scopeDef[i].layer;
			scopeDef.numLayers		= controllerDef->m_scopeDef[i].numLayers;
			scopeDef.mannContexts = &m_contexts;

			for (uint32 j=0; j<eMEM_Max; j++)
			{
				scopeDef.fragTrack[j] = NULL;
				scopeDef.animNode[j] = NULL;
				scopeDef.context[j]	= NULL;
			}
		}

		if (m_wndFragmentEditorPage.KeyProperties())
		{
			m_wndFragmentEditorPage.KeyProperties()->CreateAllVars();
		}

		if (m_wndTransitionEditorPage.KeyProperties())
		{
			m_wndTransitionEditorPage.KeyProperties()->CreateAllVars();
		}

		if (m_wndPreviewerPage.KeyProperties())
		{
			m_wndPreviewerPage.KeyProperties()->CreateAllVars();
		}

		return true;
	}

	return false;
}

void CMannequinDialog::ResavePreviewFile()
{
	SavePreviewFile(m_contexts.previewFilename);
}

//////////////////////////////////////////////////////////////////////////
bool CMannequinDialog::SavePreviewFile(const char *filename)
{
	IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
	const SControllerDef *pControllerDef = m_contexts.m_controllerDef;
	if (pControllerDef == NULL)
	{
		return false;
	}
	
	XmlNodeRef historyNode = m_historyBackup->clone();

	XmlNodeRef contextsNode = XmlHelpers::CreateXmlNode( "contexts" );
	const size_t numContexts = m_contexts.m_contextData.size();
	for (uint32 i=0; i<numContexts; i++)
	{
		XmlNodeRef contextDefNode = XmlHelpers::CreateXmlNode( "contextData" );
		SScopeContextData &contextDef = m_contexts.m_contextData[i];

		contextDefNode->setAttr("name",	contextDef.name);
		if (contextDef.startActive)
		{
			contextDefNode->setAttr("enabled", contextDef.startActive);
		}
		if (contextDef.database!=NULL)
		{
			contextDefNode->setAttr("database", contextDef.database->GetFilename());
		}
		if (contextDef.pControllerDef!=NULL)
		{
			contextDefNode->setAttr("controllerDef", contextDef.pControllerDef->m_filename.c_str());
		}
		if (contextDef.pSlaveDatabase!=NULL)
		{
			contextDefNode->setAttr("slaveDatabase", contextDef.pSlaveDatabase->GetFilename());
		}

		const char *contextDataName = (contextDef.contextID == CONTEXT_DATA_NONE) ? "" : pControllerDef->m_scopeContexts.GetTagName(contextDef.contextID);
		contextDefNode->setAttr("context", contextDataName);
		
		CryStackStringT<char,512> sTags;
		pControllerDef->m_tags.FlagsToTagList(contextDef.tags, sTags);
		if (!sTags.empty())
		{
			contextDefNode->setAttr("tags", sTags.c_str());
		}
		if (contextDef.fragTags.size()>0)
		{
			contextDefNode->setAttr("fragtags", contextDef.fragTags);
		}

		if (i==0)
		{
			assert(contextDef.viewData[0].charInst!=NULL);
			if(contextDef.viewData[0].charInst!=NULL)
				contextDefNode->setAttr("model", contextDef.viewData[0].charInst->GetFilePath());
		}
		else
		{
			if (contextDef.viewData[0].charInst)
			{
				contextDefNode->setAttr("model", contextDef.viewData[0].charInst->GetFilePath());
			}
			else if (contextDef.viewData[0].pStatObj)
			{
				contextDefNode->setAttr("model", contextDef.viewData[0].pStatObj->GetFilePath());
			}

			if (contextDef.attachment.size()>0)
			{
				contextDefNode->setAttr("attachment", contextDef.attachment);
			}
			if (contextDef.attachmentContext.size()>0)
			{
				contextDefNode->setAttr("attachmentContext", contextDef.attachmentContext);
			}
			if (contextDef.attachmentHelper.size()>0)
			{
				contextDefNode->setAttr("attachmentHelper", contextDef.attachmentHelper);
			}

			if (!contextDef.startLocation.t.IsZero())
			{
				contextDefNode->setAttr("startPosition", contextDef.startLocation.t);
			}
			if (!contextDef.startRotationEuler.IsEquivalent(Ang3(ZERO)))
			{
				contextDefNode->setAttr("startRotation", contextDef.startRotationEuler);
			}
		}

		contextsNode->addChild(contextDefNode);
	}

	XmlNodeRef xmlBackground = NULL;
	if (m_contexts.backGroundObjects)
	{
		xmlBackground = contextsNode->createNode("background");
		xmlBackground->setAttr("Pos", m_contexts.backgroundRootTran.t);
		xmlBackground->setAttr("Rotate", m_contexts.backgroundRootTran.q);
		XmlNodeRef bkObjects = m_contexts.backGroundObjects->clone();
		xmlBackground->addChild(bkObjects);

		uint32 numProps = m_contexts.backgroundProps.size();
		if (numProps > 0)
		{
			XmlNodeRef xmlProps = XmlHelpers::CreateXmlNode( "Props" );
			xmlBackground->addChild(xmlProps);
			for (uint32 i=0; i<numProps; i++)
			{
				SMannequinContexts::SProp &prop = m_contexts.backgroundProps[i];

				XmlNodeRef xmlProp = XmlHelpers::CreateXmlNode( "Prop" );
				xmlProp->setAttr("Name", prop.name);
				xmlProp->setAttr("Entity", prop.entity);
				xmlProps->addChild(xmlProp);
			}
		}
	}

	XmlNodeRef controllerDefNode = XmlHelpers::CreateXmlNode( "controllerDef" );
	controllerDefNode->setAttr("filename", pControllerDef->m_filename.c_str());

	// add all the children we've built above then save out the new/updated preview xml file
	XmlNodeRef previewNode = XmlHelpers::CreateXmlNode( "MannequinPreview" );
	previewNode->addChild(controllerDefNode);
	previewNode->addChild(contextsNode);
	previewNode->addChild(historyNode);
	if (xmlBackground)
	{
		previewNode->addChild(xmlBackground);
	}

	const bool saveSuccess = XmlHelpers::SaveXmlNode(previewNode, filename);
	return saveSuccess;
}

void CMannequinDialog::ClearContextData(const EMannequinEditorMode editorMode, const int scopeContextID)
{
	m_contexts.viewData[editorMode].m_pActionController->ClearScopeContext(scopeContextID);

	const uint32 numScopes = m_contexts.m_scopeData.size();
	SScopeContextData *pCurData = NULL;
	for (uint32 s=0; s<numScopes; s++)
	{
		SScopeData &scopeData = m_contexts.m_scopeData[s];
		if (scopeData.contextID == scopeContextID)
		{
			pCurData = scopeData.context[editorMode];
			scopeData.context[editorMode] = NULL;
		}
	}

	if (pCurData)
	{
		assert(pCurData->viewData[editorMode].enabled);

		pCurData->viewData[editorMode].enabled = false;

		if (pCurData->attachment.length() > 0)
		{
			uint32 parentContextDataID = 0;

			uint32 parentContextID = 0;
			if (pCurData->attachmentContext.length() > 0)
			{
				parentContextID = m_contexts.m_controllerDef->m_scopeContexts.Find(pCurData->attachmentContext.c_str());

				const uint32 numScopes = m_contexts.m_scopeData.size();
				for (uint32 s=0; s<numScopes; s++)
				{
					const SScopeData &scopeData = m_contexts.m_scopeData[s];
					if ((scopeData.contextID == parentContextID) && scopeData.context[editorMode])
					{
						parentContextDataID = scopeData.context[editorMode]->dataID;
						break;
					}
				}
			}

			ICharacterInstance *masterChar = m_contexts.m_contextData[parentContextDataID].viewData[editorMode].charInst;
			if (masterChar)
			{
				IAttachmentManager *attachmentManager = masterChar->GetIAttachmentManager();
				IAttachment *attachPt = attachmentManager->GetInterfaceByName(pCurData->attachment.c_str());

				if (attachPt)
				{
					attachPt->ClearBinding();

					attachPt->SetAttRelativeDefault(pCurData->viewData[editorMode].oldAttachmentLoc);
				}
			}
		}
		else
		{
			pCurData->viewData[editorMode].entity->Hide(true);
		}
	}
}

bool CMannequinDialog::EnableContextData(const EMannequinEditorMode editorMode, const int scopeContextDataID)
{
	const uint32 numContexts = m_contexts.m_contextData.size();
	
	for (uint32 i=0; i<numContexts; i++)
	{
		SScopeContextData &contextData = m_contexts.m_contextData[i];
		if (contextData.dataID == scopeContextDataID)
		{
			if (!contextData.viewData[editorMode].enabled)
			{
				if (contextData.contextID != CONTEXT_DATA_NONE)
				{
					ClearContextData(editorMode, contextData.contextID);
				}

				if (contextData.attachment.length() > 0)
				{
					uint32 parentContextDataID = 0;
					if (contextData.attachmentContext.length() > 0)
					{
						const uint32 parentContextID = m_contexts.m_controllerDef->m_scopeContexts.Find(contextData.attachmentContext.c_str());

						const uint32 numScopes = m_contexts.m_scopeData.size();
						for (uint32 s=0; s<numScopes; s++)
						{
							const SScopeData &scopeData = m_contexts.m_scopeData[s];
							if (scopeData.contextID == parentContextID)
							{
								if (scopeData.context[editorMode]!=NULL)
								{
									parentContextDataID = scopeData.context[editorMode]->dataID;
									break;
								}
								else
								{
									// Failed to find a parent context, commonly weapons scopes/attachments need a weapon selecting first.
									return false;
								}
							}
						}
					}

					ICharacterInstance *masterChar = m_contexts.m_contextData[parentContextDataID].viewData[editorMode].charInst;
					if (masterChar)
					{
						IAttachmentManager *attachmentManager = masterChar->GetIAttachmentManager();
						IAttachment *attachPt = attachmentManager->GetInterfaceByName(contextData.attachment.c_str());

						if (attachPt)
						{
							contextData.viewData[editorMode].oldAttachmentLoc = attachPt->GetAttRelativeDefault();

							if (contextData.viewData[editorMode].charInst)
							{
								CSKELAttachment *chrAttachment = new CSKELAttachment();
								chrAttachment->m_pCharInstance  = contextData.viewData[editorMode].charInst;

								attachPt->AddBinding(chrAttachment);
							}
							else
							{
								CCGFAttachment *cfgAttachment = new CCGFAttachment();
								cfgAttachment->pObj = contextData.viewData[editorMode].pStatObj;
								attachPt->AddBinding(cfgAttachment);

								if (contextData.attachmentHelper.length() > 0)
								{
									Matrix34 helperMat = contextData.viewData[editorMode].pStatObj->GetHelperTM(contextData.attachmentHelper.c_str());
									helperMat.InvertFast();

									QuatT requiredRelativeLocation = contextData.viewData[editorMode].oldAttachmentLoc * QuatT(helperMat.GetTranslation(), Quat(helperMat).GetNormalized());
									requiredRelativeLocation.q.Normalize();

									attachPt->SetAttRelativeDefault(requiredRelativeLocation);
								}
							}
						}
					}
				}
				else
				{
					contextData.viewData[editorMode].entity->Hide(false);
				}

				if (contextData.viewData[editorMode].m_pActionController)
				{
					m_contexts.viewData[editorMode].m_pActionController->SetSlaveController(*contextData.viewData[editorMode].m_pActionController, contextData.contextID, true, NULL);
				}
				else if (contextData.database)
				{
					m_contexts.viewData[editorMode].m_pActionController->SetScopeContext(contextData.contextID, *contextData.viewData[editorMode].entity, contextData.viewData[editorMode].charInst, contextData.database);
				}

				const uint32 numScopes = m_contexts.m_scopeData.size();
				for (uint32 s=0; s<numScopes; s++)
				{
					SScopeData &scopeData = m_contexts.m_scopeData[s];
					if (scopeData.contextID == contextData.contextID)
					{
						scopeData.context[editorMode] = &contextData;
					}
				}
				contextData.viewData[editorMode].enabled = true;
			}
		}
	}

	return true;
}

bool CMannequinDialog::InitialiseToPreviewFile(const char *previewFile)
{
	XmlNodeRef sequenceNode;
	const bool loadedSetup = LoadPreviewFile(previewFile, sequenceNode);

	EnableContextEditor(loadedSetup);

	if(loadedSetup)
	{
		IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

		const char *PAGE_POSTFIX[eMEM_Max] = {"", "_Prv", "_Trn"};

		for (uint32 em = 0; em < eMEM_Max; em++)
		{
			if (!m_contexts.m_contextData.empty())
			{
				ICharacterInstance *pIChar = m_contexts.m_contextData[0].viewData[em].charInst;
				if (pIChar)
				{
					pIChar->GetISkeletonAnim()->SetAnimationDrivenMotion(true);

					ISkeletonPose &skelPose = *pIChar->GetISkeletonPose();
					IAnimationPoseBlenderDir* pPoseBlenderAim = skelPose.GetIPoseBlenderAim();
					if ( pPoseBlenderAim )
					{
						pPoseBlenderAim->SetState(false);
						pPoseBlenderAim->SetLayer(3);
						pPoseBlenderAim->SetTarget( Vec3(0.0f, 0.0f, 0.0f) );
						pPoseBlenderAim->SetFadeoutAngle(DEG2RAD(180.0f));
					}

					IAnimationPoseBlenderDir* pPoseBlenderLook = skelPose.GetIPoseBlenderLook();
					if ( pPoseBlenderLook )
					{
						pPoseBlenderLook->SetState(false);
					}
				}
			}

			IEntity *pMannEntity = NULL;
			IEntityClass *pEntityClass =  gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
			assert(pEntityClass);
			for (uint32 i=0; i<m_contexts.m_contextData.size(); i++)
			{
				SScopeContextData &context = m_contexts.m_contextData[i];

				SEntitySpawnParams params;
				params.pClass = pEntityClass;
				CString name = context.name + PAGE_POSTFIX[em];
				params.sName = name;
				params.vPosition.Set(0.0f, 0.0f, 0.0f);
				params.qRotation.SetIdentity();
				params.nFlags |= (ENTITY_FLAG_NEVER_NETWORK_STATIC|ENTITY_FLAG_CLIENT_ONLY);

				context.viewData[em].entity = gEnv->pEntitySystem->SpawnEntity(params, true);
				assert(context.viewData[em].entity);
				if (context.viewData[em].charInst)
				{
					context.viewData[em].entity->SetCharacter(context.viewData[em].charInst, 0);
				}
				else
				{
					context.viewData[em].entity->SetStatObj(context.viewData[em].pStatObj, 0, false);
				}

				if (context.pControllerDef)
				{
					context.viewData[em].m_pAnimContext = new SEditorAnimationContext(this, *context.pControllerDef);
					context.viewData[em].m_pActionController = mannequinSys.CreateActionController(context.viewData[em].entity, *context.viewData[em].m_pAnimContext);
					context.viewData[em].m_pActionController->SetScopeContext(0, *context.viewData[em].entity, context.viewData[em].charInst, context.pSlaveDatabase ? context.pSlaveDatabase : context.database);
				}

				if (!pMannEntity)
				{
					pMannEntity = context.viewData[em].entity;
				}
				else
				{
					context.viewData[em].entity->Hide(true);
				}
			}
			m_contexts.viewData[em].m_pEntity = pMannEntity;

			m_contexts.viewData[em].m_pAnimContext = new SEditorAnimationContext(this, *m_contexts.m_controllerDef);
			m_contexts.viewData[em].m_pActionController = mannequinSys.CreateActionController(pMannEntity, *m_contexts.viewData[em].m_pAnimContext);

			m_contexts.potentiallyActiveScopes = 0;
			const uint32 numContextData = m_contexts.m_contextData.size();
			uint32 installedContexts = 0;
			for (uint32 i=0; i<numContextData; i++)
			{
				SScopeContextData &contextData = m_contexts.m_contextData[i];
				const uint32 contextFlag = (contextData.contextID == CONTEXT_DATA_NONE) ? 0 : (1<<contextData.contextID);
				const uint32 numScopes = m_contexts.m_scopeData.size();
				for (uint32 s=0; s<numScopes; s++)
				{
					if (m_contexts.m_scopeData[s].contextID == contextData.contextID)
					{
						m_contexts.potentiallyActiveScopes |= (1<<m_contexts.m_scopeData[s].scopeID);
					}
				}

				if (((installedContexts & contextFlag) == 0) && contextData.viewData[em].enabled)
				{
					contextData.viewData[em].enabled = false;
					EnableContextData((EMannequinEditorMode)em, i);
					installedContexts |= contextFlag;
				}
			}

			//--- Create background objects
			SMannequinContextViewData::TBackgroundObjects &backgroundObjects = m_contexts.viewData[em].backgroundObjects;
			backgroundObjects.clear();
			if (m_contexts.backGroundObjects)
			{
				QuatT inverseRoot = m_contexts.backgroundRootTran.GetInverted();
				GetIEditor()->SuspendUndo();
				CObjectArchive ar( GetIEditor()->GetObjectManager(),m_contexts.backGroundObjects,true );
				ar.EnableProgressBar(false); // No progress bar is shown when loading objects.
				ar.MakeNewIds( true );
				ar.LoadObjects( m_contexts.backGroundObjects );
				ar.ResolveObjects();
				const int numObjects = ar.GetLoadedObjectsCount();
				for (int i = 0; i < numObjects; i++)
				{
					CBaseObject *pObj = ar.GetLoadedObject(i);
					if(pObj)
					{
						// TODO Jean: review
						// pObj->SetFlags(OBJFLAG_NOTINWORLD);

						if (!pObj->GetParent())
						{
							QuatT childTran(pObj->GetPos(), pObj->GetRotation());
							QuatT adjustedTran = inverseRoot * childTran;
							pObj->SetPos(adjustedTran.t);
							pObj->SetRotation(adjustedTran.q);
							backgroundObjects.push_back(pObj);

							//--- Store off the IDs for each prop
							if (pObj->IsKindOf(RUNTIME_CLASS(CEntityObject)))
							{
								CEntityObject *pEntObject = (CEntityObject *)pObj;
								const uint32 numProps = m_contexts.backgroundProps.size();
								for (uint32 i=0; i<numProps; i++)
								{
									if (pObj->GetName() == m_contexts.backgroundProps[i].entity)
									{
										m_contexts.backgroundProps[i].entityID[em] = pEntObject->GetIEntity()->GetId();
									}
								}
							}
						}
					}
				}
				GetIEditor()->ResumeUndo();
			}
		}

		PopulateTagList();

		m_wndFragmentEditorPage.InitialiseToPreviewFile(sequenceNode);
		m_wndTransitionEditorPage.InitialiseToPreviewFile(sequenceNode);
		m_wndPreviewerPage.InitialiseToPreviewFile();

		m_pFileChangeWriter->SetControllerDef( m_contexts.m_controllerDef );

		EnableMenuCommand(ID_MANNEQUIN_TOOLS_LISTUSEDANIMATIONSCURRENTPREVIEW, true);
	}

	return loadedSetup;
}

void CMannequinDialog::LoadNewPreviewFile(const char *previewFile)
{
	m_bPreviewFileLoaded = false;

	if (previewFile == NULL)
		previewFile = m_contexts.previewFilename.c_str();

	if (InitialiseToPreviewFile(previewFile))
	{
		m_wndFragmentBrowser->SetScopeContext(-1);
		m_wndFragmentBrowser->SetContext(m_contexts);
		m_wndTransitionBrowser->SetContext(m_contexts);
		
		m_wndFragmentEditorPage.ModelViewport()->ClearCharacters();
		m_wndTransitionEditorPage.ModelViewport()->ClearCharacters();
		m_wndPreviewerPage.ModelViewport()->ClearCharacters();

		for (uint32 i=0; i<m_contexts.m_contextData.size(); i++)
		{
			const SScopeContextData &contextData = m_contexts.m_contextData[i];
			if (contextData.attachment.length() == 0)
			{
				m_wndFragmentEditorPage.ModelViewport()->AddCharacter(contextData.viewData[eMEM_FragmentEditor].entity, contextData.startLocation);
				m_wndTransitionEditorPage.ModelViewport()->AddCharacter(contextData.viewData[eMEM_TransitionEditor].entity, contextData.startLocation);
				m_wndPreviewerPage.ModelViewport()->AddCharacter(contextData.viewData[eMEM_Previewer].entity, contextData.startLocation);
			}
		}

		m_wndFragmentEditorPage.ModelViewport()->OnLoadPreviewFile();
		m_wndTransitionEditorPage.ModelViewport()->OnLoadPreviewFile();
		m_wndPreviewerPage.ModelViewport()->OnLoadPreviewFile();

		m_wndFragmentEditorPage.TrackPanel()->SetFragment(FRAGMENT_ID_INVALID);

		m_bPreviewFileLoaded = true;
	}
}


//////////////////////////////////////////////////////////////////////////
BOOL CMannequinDialog::OnInitDialog()
{
	__super::OnInitDialog();

	m_bPreviewFileLoaded = false;
	m_bRefreshingTagCtrl = false;

	m_tagVars.reset(new CVarBlock());

	// Error report
	m_wndErrorReport.Create(CMannErrorReportDialog::IDD, this);

	// Fragment editor page
	m_wndFragmentEditorPage.Create(CFragmentEditorPage::IDD, this);

	// Fragment panel splitter
	m_wndFragmentSplitter.CreateStatic(this, 2, 1, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, AFX_IDW_PANE_FIRST);
	m_wndFragmentSplitter.SetRowInfo(0, 400, s_minPanelSize);
	m_wndFragmentSplitter.SetRowInfo(1, 400, s_minPanelSize);

	// Fragment browser
	m_wndFragmentBrowser = new CFragmentBrowser(*m_wndFragmentEditorPage.TrackPanel(), &m_wndFragmentSplitter, m_wndFragmentSplitter.IdFromRowCol(0, 0));
	m_wndFragmentBrowser->SetOwner(this);

	// Transition editor page
	m_wndTransitionEditorPage.Create(CTransitionEditorPage::IDD, this);

	// Previewer page
	m_wndPreviewerPage.Create(CPreviewerPage::IDD, this);

	// Create the sequence browser
	m_wndSequenceBrowser = new CSequenceBrowser( this, m_wndPreviewerPage );
	m_wndSequenceBrowser->SetOwner(this);

	// Transition browser
	m_wndTransitionBrowser = new CTransitionBrowser( this, m_wndTransitionEditorPage );
	m_wndTransitionBrowser->SetOwner(this);

	// Tags panel
	m_wndTagsPanel = new CPropertiesPanel(this);
	m_wndTagsPanel->EnableWindow(FALSE);
	
	// Fragment rollup panel
	m_wndFragmentsRollup.Create(WS_VISIBLE | WS_CHILD, CRect(8, 8, 100, 100), &m_wndFragmentSplitter, m_wndFragmentSplitter.IdFromRowCol(1, 0));
	m_wndFragmentsRollup.SetOwner(this);
	int tagsPanelIndex = m_wndFragmentsRollup.InsertPage("No Fragment Selected", m_wndTagsPanel);
	RC_PAGEINFO* pTagsPanelInfo = m_wndFragmentsRollup.GetPageInfo(tagsPanelIndex);
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));
	logFont.lfHeight = 16;
	logFont.lfWeight = FW_BOLD;
	strcpy(logFont.lfFaceName, "MS Shell Dlg 2");
	m_wndTagsPanelFont.CreateFontIndirect(&logFont);
	pTagsPanelInfo->pwndButton->SetFont(&m_wndTagsPanelFont);

	// Create the dock panes
	GetDockingPaneManager()->HideClient(TRUE);
	GetDockingPaneManager()->m_nSplitterGap = s_minPanelSize; // Sets the minimum size of a pane

	CXTPDockingPane *pDockPaneFragments = CreateDockingPane("Fragments", &m_wndFragmentSplitter, IDW_FRAGMENTS_PANE, CRect(0, 0, 270, 300), dockLeftOf);
	pDockPaneFragments->SetOptions(xtpPaneNoCloseable);

	CXTPDockingPane *pDockPaneSequences = CreateDockingPane("Sequences", m_wndSequenceBrowser, IDW_SEQUENCES_PANE, CRect(0, 0, 270, 300), dockRightOf);
	pDockPaneSequences->SetOptions(xtpPaneNoCloseable);

	CXTPDockingPane *pDockPaneTransitions = CreateDockingPane("Transitions", m_wndTransitionBrowser, IDW_TRANSITIONS_PANE, CRect(0, 0, 270, 300), dockRightOf);
	pDockPaneTransitions->SetOptions(xtpPaneNoCloseable);

	GetDockingPaneManager()->AttachPane(pDockPaneSequences, pDockPaneFragments);
	GetDockingPaneManager()->AttachPane(pDockPaneTransitions, pDockPaneFragments);
	pDockPaneFragments->Select();

	CXTPDockingPane* pDockPaneFragmentEditor = CreateDockingPane("Fragment Editor", &m_wndFragmentEditorPage, IDW_FRAGMENT_EDITOR_PANE, CRect(0, 0, 500, 500), dockRightOf, pDockPaneFragments);
	CXTPDockingPane* pDockPaneTransitionEditor = CreateDockingPane("Transition Editor", &m_wndTransitionEditorPage, IDW_TRANSITION_EDITOR_PANE, CRect(0, 0, 500, 500), dockRightOf, pDockPaneFragments);
	CXTPDockingPane* pDockPanePreviewer = CreateDockingPane("Previewer", &m_wndPreviewerPage, IDW_PREVIEWER_PANE, CRect(0, 0, 500, 500), dockRightOf, pDockPaneFragments);
	CXTPDockingPane* pDockPaneErrorReport = CreateDockingPane("Mannequin Error Report", &m_wndErrorReport, IDW_ERROR_REPORT_PANE, CRect(0, 0, 500, 500), dockRightOf, pDockPaneFragments);
	GetDockingPaneManager()->AttachPane(pDockPaneTransitionEditor, pDockPaneFragmentEditor);
	GetDockingPaneManager()->AttachPane(pDockPanePreviewer, pDockPaneFragmentEditor);
	GetDockingPaneManager()->AttachPane(pDockPaneErrorReport, pDockPaneFragmentEditor);

	// Create the menu bar
	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu( _T("Menu Bar"), IDR_MANNEQUIN_DIALOG );	
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(false);

	EnableMenuCommand(ID_MANNEQUIN_TOOLS_LISTUSEDANIMATIONSCURRENTPREVIEW, false);

	m_wndFragmentBrowser->SetOnSelectedItemEditCallback( functor( *this, &CMannequinDialog::OnFragmentBrowserSelectedItemEdit ) );
	m_wndFragmentBrowser->SetOnScopeContextChangedCallback( functor( *this, &CMannequinDialog::OnFragmentBrowserScopeContextChanged ) );
	UpdateAnimationDBEditorMenuItemEnabledState();

	LoadLayouts();

	GetDockingPaneManager()->ShowPane(IDW_FRAGMENTS_PANE);

	{
		LoadNewPreviewFile(Path::GamePathToFullPath(gSettings.mannequinSettings.defaultPreviewFile));

		ActivateErrorReport();
		if (!m_wndErrorReport.HasErrors())
		{
			GetDockingPaneManager()->ShowPane(IDW_FRAGMENT_EDITOR_PANE);
		}
	}

	return TRUE;
}

void CMannequinDialog::SaveLayouts()
{
	m_wndFragmentEditorPage.SaveLayout();
	m_wndTransitionEditorPage.SaveLayout();
	m_wndPreviewerPage.SaveLayout();

	MannUtils::SaveSplitterToRegistry(_T("Settings\\Mannequin\\FragmentBrowserLayout"), _T("FragmentSplitter"), m_wndFragmentSplitter);
}

void CMannequinDialog::LoadLayouts()
{
	AutoLoadFrameLayout("MannequinEditor");

	m_wndFragmentEditorPage.LoadLayout();
	m_wndTransitionEditorPage.LoadLayout();
	m_wndPreviewerPage.LoadLayout();

	MannUtils::LoadSplitterFromRegistry(_T("Settings\\Mannequin\\FragmentBrowserLayout"), _T("FragmentSplitter"), m_wndFragmentSplitter, s_minPanelSize);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (m_wndFragmentEditorPage.TrackPanel())
	{
		m_wndFragmentEditorPage.TrackPanel()->OnEditorNotifyEvent(event);
	}
	switch (event)
	{
	case eNotify_OnCloseScene:
		ClearContextEntities();
		ClearContextViewData();
		break;
	case eNotify_OnBeginSceneOpen:
	case eNotify_OnBeginNewScene:
	case eNotify_OnEndLoad:
		ReloadContextEntities();
		break;
	//case eNotify_OnBeginNewScene:
	//case eNotify_OnCloseScene:
	//	SetCurrentSequence(NULL);
	//	break;
	//case eNotify_OnBeginSceneOpen:
	//	m_bInLevelLoad = true;
	//	m_wndTrackProps.Reset();
	//	break;
	//case eNotify_OnEndSceneOpen:
	//	m_bInLevelLoad = false;
	//	ReloadSequences();
	//	break;
	//case eNotify_OnMissionChange:
	//	if (!m_bInLevelLoad)
	//		ReloadSequences();
	//	break;
	case eNotify_OnUpdateSequencerKeySelection:
		m_wndFragmentEditorPage.KeyProperties()->OnKeySelectionChange();
		m_wndPreviewerPage.KeyProperties()->OnKeySelectionChange();
		m_wndTransitionEditorPage.KeyProperties()->OnKeySelectionChange();
		m_wndFragmentEditorPage.CreateLocators();
		break;
	case eNotify_OnUpdateSequencer:
		m_wndTransitionEditorPage.OnUpdateTV();
		m_wndPreviewerPage.OnUpdateTV();
		if (m_wndFragmentEditorPage.TrackPanel())
		{
			m_wndFragmentEditorPage.Nodes()->SetSequence(m_wndFragmentEditorPage.TrackPanel()->GetSequence());
		}
		break;
	case eNotify_OnUpdateSequencerKeys:
		break;
	case eNotify_OnIdleUpdate:
		Update();
		break;
	}
}

void CMannequinDialog::OnRender()
{
	//if (m_sequenceAnalyser)
	//{
	//	bool isDraggingTime = m_wndKeys->IsDraggingTime();
	//	float dsTime = m_wndKeys->GetTime();

	//	const float XPOS = 200.0f, YPOS = 60.0f, FONT_SIZE = 2.0f, FONT_COLOUR[4] = {1,1,1,1};
	//	gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, FONT_SIZE, FONT_COLOUR, false, "%s %f", isDraggingTime ? "Dragging Time" : "Not Dragging", dsTime);
	//}	
}

void CMannequinDialog::Update()
{
	m_wndFragmentBrowser->Update();

	if (m_bPreviewFileLoaded)
	{
		m_wndFragmentEditorPage.Update();
		m_wndPreviewerPage.Update();
		m_wndTransitionEditorPage.Update();
	}
}

bool CMannequinDialog::CheckChangedData()
{
	return m_pFileChangeWriter->ShowFileManager() != eFMR_Cancel;
}

void CMannequinDialog::OnMoveLocator(EMannequinEditorMode editorMode, uint32 refID, const char *locatorName, const QuatT &transform)
{
	SFragmentHistoryContext *historyContext = NULL;
	if (editorMode == eMEM_Previewer)
	{
		historyContext = m_wndPreviewerPage.FragmentHistory();
	}
	else if (editorMode == eMEM_TransitionEditor)
	{
		historyContext = m_wndTransitionEditorPage.FragmentHistory();
	}
	else
	{
		historyContext = &m_wndFragmentEditorPage.TrackPanel()->GetFragmentHistory();
	}

	if ( refID < historyContext->m_history.m_items.size() )
	{
		historyContext->m_history.m_items[refID].param.value = transform;

		const uint32 numTracks = historyContext->m_tracks.size();
		for (uint32 i=0; i<numTracks; i++)
		{
			CSequencerTrack *animTrack = historyContext->m_tracks[i];
			if (animTrack->GetParameterType() == SEQUENCER_PARAM_PARAMS)
			{
				CParamKey paramKey;
				const int numKeys = animTrack->GetNumKeys();
				for (int k=0; k<numKeys; k++)
				{
					animTrack->GetKey(k, &paramKey);

					if (paramKey.historyItem == refID)
					{
						paramKey.parameter.value = transform;
						animTrack->SetKey(k, &paramKey);
						break;
					}
				}
			}
		}
	}
	else
	{
		CSequencerUtils::SelectedKeys selectedKeys;
		CSequencerUtils::GetSelectedKeys(m_wndFragmentEditorPage.TrackPanel()->GetSequence(), selectedKeys);

		uint iIndex = refID - historyContext->m_history.m_items.size();

		assert(iIndex < selectedKeys.keys.size());

		CSequencerUtils::SelectedKey& selectedKey = selectedKeys.keys[iIndex];

		const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
		assert(paramType == SEQUENCER_PARAM_PROCLAYER);

		CProcClipKey* key = (CProcClipKey*)selectedKey.pTrack->GetKey(selectedKey.nKey);

		assert(key != NULL);
		if (key->pParams)
		{
			key->pParams->OnEditorMovedTransformGizmo(transform);
			selectedKey.pTrack->OnChange();
		}
	}

	FragmentEditor()->KeyProperties()->ForceUpdate();
}

void CMannequinDialog::EditFragmentByKey(const CFragmentKey &key, int contextID)
{
	if (key.fragmentID != FRAGMENT_ID_INVALID)
	{
		EditFragment(key.fragmentID, key.tagStateFull, contextID);
	}
}

void CMannequinDialog::EditFragment(FragmentID fragID, const SFragTagState &tagState, int contextID)
{
	m_wndFragmentEditorPage.SetTagState(fragID, tagState);

	ActionScopes scopeMask = m_contexts.m_controllerDef->GetScopeMask(fragID, tagState);
	ActionScopes filteredScopeMask = scopeMask & m_contexts.viewData[eMEM_FragmentEditor].m_pActionController->GetActiveScopeMask();

	if (filteredScopeMask)
	{
		if (contextID >= 0)
		{
			const uint32 numScopes = m_contexts.m_scopeData.size();
			for (uint32 s=0; s<numScopes; s++)
			{
				if (m_contexts.m_scopeData[s].contextID == contextID)
				{
					if (m_contexts.m_scopeData[s].context[eMEM_FragmentEditor])
					{
						m_wndFragmentBrowser->SetScopeContext(m_contexts.m_scopeData[s].context[eMEM_FragmentEditor]->dataID);
					}
					break;
				}
			}
		}
		m_wndFragmentBrowser->SelectFragment(fragID, tagState);
		m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state = tagState.globalTags;
		m_wndFragmentEditorPage.Nodes()->SetSequence(m_wndFragmentEditorPage.TrackPanel()->GetSequence());
	}
}

void CMannequinDialog::EditTransitionByKey(const struct CFragmentKey &key, int contextID)
{
	if (key.transition && (key.tranFragFrom != FRAGMENT_ID_INVALID) && (key.tranFragTo != FRAGMENT_ID_INVALID))
	{
		TTransitionID transition(key.tranFragFrom, key.tranFragTo, key.tranTagFrom, key.tranTagTo, key.tranBlendUid);
		m_wndTransitionBrowser->SelectAndOpenRecord(transition);
	}
}

void CMannequinDialog::OnSave()
{
	m_wndFragmentEditorPage.TrackPanel()->FlushChanges();

	if (m_pFileChangeWriter->ShowFileManager() == eFMR_NoChanges)
	{
		CryMessageBox("There are no modified files", "CryMannequin", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		// Force a reload of the key properties to clear out the per-property "modified" flag
		m_wndFragmentEditorPage.KeyProperties()->ForceUpdate();
	}

	m_wndFragmentEditorPage.TrackPanel()->SetCurrentFragment();
}

void CMannequinDialog::OnReexportAll()
{
	m_wndFragmentEditorPage.TrackPanel()->FlushChanges();

	LoadMannequinFolder();

	if (m_pFileChangeWriter->ShowFileManager(true) == eFMR_NoChanges)
	{
		CryMessageBox("There are no modified files", "CryMannequin", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		// Force a reload of the key properties to clear out the per-property "modified" flag
		m_wndFragmentEditorPage.KeyProperties()->ForceUpdate();
	}

	m_wndFragmentEditorPage.TrackPanel()->SetCurrentFragment();
}

void CMannequinDialog::Validate()
{
	const uint32 numContexts = m_contexts.m_contextData.size();
	m_wndErrorReport.Clear();
	for (uint32 i=0; i<numContexts; i++)
	{
		SScopeContextData &contextDef = m_contexts.m_contextData[i];
		Validate(contextDef);
	}

	ActivateErrorReport();
}

void CMannequinDialog::OnNewSequence()
{
	m_wndPreviewerPage.OnNewSequence();
	GetDockingPaneManager()->ShowPane(CMannequinDialog::IDW_PREVIEWER_PANE);
}

void CMannequinDialog::OnLoadSequence()
{
	m_wndPreviewerPage.OnLoadSequence();
	GetDockingPaneManager()->ShowPane(CMannequinDialog::IDW_PREVIEWER_PANE);
}

void CMannequinDialog::OnSaveSequence()
{
	m_wndPreviewerPage.OnSaveSequence();
}

void CMannequinDialog::OnUpdateNewSequence(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bPreviewFileLoaded);
}

void CMannequinDialog::OnUpdateLoadSequence(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bPreviewFileLoaded);
}

void CMannequinDialog::OnUpdateSaveSequence(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bPreviewFileLoaded);
}

void CMannequinDialog::OnViewFragmentEditor()
{
	if (GetDockingPaneManager()->IsPaneClosed(IDW_FRAGMENT_EDITOR_PANE))
		GetDockingPaneManager()->ShowPane(IDW_FRAGMENT_EDITOR_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_FRAGMENT_EDITOR_PANE);
}

void CMannequinDialog::OnViewPreviewer()
{
	if (GetDockingPaneManager()->IsPaneClosed(IDW_PREVIEWER_PANE))
		GetDockingPaneManager()->ShowPane(IDW_PREVIEWER_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_PREVIEWER_PANE);
}

void CMannequinDialog::OnViewTransitionEditor()
{
	if (GetDockingPaneManager()->IsPaneClosed(IDW_TRANSITION_EDITOR_PANE))
		GetDockingPaneManager()->ShowPane(IDW_TRANSITION_EDITOR_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_TRANSITION_EDITOR_PANE);
}

void CMannequinDialog::OnViewErrorReport()
{
	if (GetDockingPaneManager()->IsPaneClosed(IDW_ERROR_REPORT_PANE))
		GetDockingPaneManager()->ShowPane(IDW_ERROR_REPORT_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_ERROR_REPORT_PANE);
}

void CMannequinDialog::OnUpdateViewFragmentEditor(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_nID == ID_VIEW_FRAGMENTEDITOR)
	{
		if (GetDockingPaneManager()->IsPaneClosed(IDW_FRAGMENT_EDITOR_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);
		else
			pCmdUI->SetCheck(BST_CHECKED);
	}
}

void CMannequinDialog::OnUpdateViewPreviewer(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_nID == ID_VIEW_PREVIEWER)
	{
		if (GetDockingPaneManager()->IsPaneClosed(IDW_PREVIEWER_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);
		else
			pCmdUI->SetCheck(BST_CHECKED);
	}
}

void CMannequinDialog::OnUpdateViewTransitionEditor(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_nID == ID_VIEW_TRANSITIONEDITOR)
	{
		if (GetDockingPaneManager()->IsPaneClosed(IDW_TRANSITION_EDITOR_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);
		else
			pCmdUI->SetCheck(BST_CHECKED);
	}
}

void CMannequinDialog::OnUpdateViewErrorReport(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_nID == ID_VIEW_ERRORREPORT)
	{
		if (GetDockingPaneManager()->IsPaneClosed(IDW_ERROR_REPORT_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);
		else
			pCmdUI->SetCheck(BST_CHECKED);
	}
}

struct SErrorContext
{
	CMannErrorReportDialog  *errorReport;
	const SScopeContextData	*contextDef;
};

void OnErrorCallback(const SMannequinErrorReport &errorReport, CMannErrorRecord::ESeverity errorSeverity, void *_context)
{
	SErrorContext *errContext = (SErrorContext *) _context;

	CMannErrorRecord err;
	err.error					= errorReport.error;
	err.type					= errorReport.errorType;
	err.file					= errContext->contextDef->database->GetFilename();
	err.severity			= errorSeverity;
	err.flags					= CMannErrorRecord::FLAG_NOFILE;
	err.fragmentID		= errorReport.fragID;
	err.tagState			= errorReport.tags;
	err.fragmentIDTo	= errorReport.fragIDTo;
	err.tagStateTo		= errorReport.tagsTo;
	err.fragOptionID	= errorReport.fragOptionID;
	err.contextDef		= errContext->contextDef;
	errContext->errorReport->AddError(err);
}

void ErrorCallback(const SMannequinErrorReport &errorReport, void *_context)
{
	OnErrorCallback(errorReport, CMannErrorRecord::ESEVERITY_ERROR, _context);
}

void WarningCallback(const SMannequinErrorReport &errorReport, void *_context)
{
	OnErrorCallback(errorReport, CMannErrorRecord::ESEVERITY_WARNING, _context);
}

void CMannequinDialog::Validate(const SScopeContextData &contextDef)
{
	SErrorContext errorContext;
	errorContext.contextDef  = &contextDef;
	errorContext.errorReport = &m_wndErrorReport;

	IAnimationDatabase *animDB = contextDef.database;

	if(animDB)
	{
		//--- TODO: Add in an option to perform the full warning level of the error checking
		animDB->Validate(contextDef.animSet, &ErrorCallback, NULL, &errorContext);
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnDestroy()
{
	SaveLayouts();

	__super::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
bool CMannequinDialog::CanClose()
{
	return CheckChangedData();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnFragmentBrowserSelectedItemEdit( FragmentID fragmentId, CString &tags )
{
	UpdateForFragment(fragmentId, tags);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::StopEditingFragment()
{
	CXTPDockingPaneManager* pDockingPaneManager = GetDockingPaneManager();
	assert(pDockingPaneManager);

	CXTPDockingPane* pFragmentEditorTagsPane = pDockingPaneManager->FindPane(IDW_FRAGMENT_EDITOR_PANE);
	assert(pFragmentEditorTagsPane);

	pFragmentEditorTagsPane->SetTitle("Fragment Editor");
	m_wndFragmentsRollup.RenamePage(0, "No Fragment Selected");
	m_wndTagsPanel->EnableWindow(FALSE);
	m_wndFragmentEditorPage.Reset();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::FindFragmentReferences(const CString &fragmentName)
{
	CXTPDockingPaneManager* pDockingPaneManager = GetDockingPaneManager();
	if (pDockingPaneManager == NULL)
	{
		return;
	}

	pDockingPaneManager->ShowPane(CMannequinDialog::IDW_TRANSITIONS_PANE, TRUE);
	m_wndTransitionBrowser->FindFragmentReferences(fragmentName);

	if (!m_wndTransitionEditorPage.IsWindowVisible())
	{
		m_wndTransitionEditorPage.ClearSequence();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::FindTagReferences(const CString &tagName)
{
	CXTPDockingPaneManager* pDockingPaneManager = GetDockingPaneManager();
	if (pDockingPaneManager == NULL)
	{
		return;
	}

	CString filteredName = tagName;
	filteredName.Replace('+', ' ');

	pDockingPaneManager->ShowPane(CMannequinDialog::IDW_TRANSITIONS_PANE, TRUE);
	m_wndTransitionBrowser->FindTagReferences(filteredName);

	if (!m_wndTransitionEditorPage.IsWindowVisible())
	{
		m_wndTransitionEditorPage.ClearSequence();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::FindClipReferences(const CClipKey &key)
{
	CString icafName = key.GetFileName();
	int startIndex = icafName.ReverseFind('/') + 1;
	icafName.Delete(0, startIndex);

	CXTPDockingPaneManager* pDockingPaneManager = GetDockingPaneManager();
	if (pDockingPaneManager == NULL)
	{
		return;
	}

	pDockingPaneManager->ShowPane(CMannequinDialog::IDW_TRANSITIONS_PANE, TRUE);
	m_wndTransitionBrowser->FindClipReferences(icafName);

	if (!m_wndTransitionEditorPage.IsWindowVisible())
	{
		m_wndTransitionEditorPage.ClearSequence();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::UpdateForFragment(FragmentID fragmentID, CString &tags)
{
	CXTPDockingPaneManager* pDockingPaneManager = GetDockingPaneManager();
	assert(pDockingPaneManager);

	CXTPDockingPane* pFragmentEditorTagsPane = pDockingPaneManager->FindPane(IDW_FRAGMENT_EDITOR_PANE);
	assert(pFragmentEditorTagsPane);

	if (fragmentID == FRAGMENT_ID_INVALID)
	{
		StopEditingFragment();
	}
	else
	{
		const char* fragmentName = m_contexts.m_controllerDef->m_fragmentIDs.GetTagName(fragmentID);
		CString title;
		title.Format("Fragment Editor : %s [%s]", fragmentName, tags.GetBuffer());
		pFragmentEditorTagsPane->SetTitle(title);

		title.Format("%s [%s]", fragmentName, tags.GetBuffer());
		m_wndFragmentsRollup.RenamePage(0, title);

		m_wndTagsPanel->EnableWindow(TRUE);

		m_wndFragmentEditorPage.UpdateSelectedFragment();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnMenuLoadPreviewFile()
{
	const bool canDiscardChanges = CheckChangedData();
	if ( !canDiscardChanges )
	{
		return;
	}

	const CString folder = Path::GetGameFolder() + CString( "/Animations/Mannequin/Preview/" );
	CFileUtil::CreateDirectory( folder );
	CString filename;

	const char* setupFileFilter = "Setup XML Files (*.xml)|*.xml";

	const bool userSelectedPreviewFile = CFileUtil::SelectSingleFile( EFILE_TYPE_ANY, filename, setupFileFilter, folder );
	if ( !userSelectedPreviewFile )
	{
		return;
	}
	
	LoadNewPreviewFile( filename );
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnContexteditor()
{
	CMannContextEditorDialog dialog;
	dialog.DoModal();

	// Brute force approach for the time being.
	SavePreviewFile( m_contexts.previewFilename );
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnAnimDBEditor()
{
	CRY_ASSERT(FragmentBrowser());
	const int scopeContextId = FragmentBrowser()->GetScopeContextId();
	if (scopeContextId < 0 || m_contexts.m_contextData.size() <= scopeContextId)
	{
		return;
	}

	IAnimationDatabase* const pAnimDB = m_contexts.m_contextData[scopeContextId].database;
	if (pAnimDB == NULL)
	{
		return;
	}

	CMannAnimDBEditorDialog dialog(pAnimDB);
	dialog.DoModal();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnTagDefinitionEditor()
{
	const CString filename(m_contexts.previewFilename);
	CMannTagDefEditorDialog dialog(Path::GetFile(filename));
	dialog.DoModal();

	// ensure fragment browser is updated if tags were deleted
	FragmentBrowser()->RebuildAll();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::ReloadContextEntities()
{
	// Brute force approach for the time being.
	LoadNewPreviewFile( m_contexts.previewFilename );
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::ClearContextEntities()
{
	// Clear context entities
	m_contexts.m_contextData.clear();
	m_contexts.m_scopeData.clear();
	m_contexts.backgroundProps.clear();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::ClearContextViewData()
{
	for (uint32 em = 0; em < eMEM_Max; em++)
	{
		SAFE_RELEASE(m_contexts.viewData[em].m_pActionController);
		SAFE_DELETE(m_contexts.viewData[em].m_pAnimContext);

		for (uint32 i=0; i<m_contexts.m_contextData.size(); i++)
		{
			SScopeContextData &context = m_contexts.m_contextData[i];
			SAFE_RELEASE(context.viewData[em].m_pActionController);
			SAFE_DELETE(context.viewData[em].m_pAnimContext);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::SetKeyProperty( const char* propertyName, const char* propertyValue )
{
	if (m_wndFragmentEditorPage.KeyProperties())
	{
		m_wndFragmentEditorPage.KeyProperties()->SetVarValue( propertyName, propertyValue );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::UpdateTagList(FragmentID fragID)
{
	const CTagDefinition *tagDef = (fragID != FRAGMENT_ID_INVALID) ? m_contexts.m_controllerDef->GetFragmentTagDef(fragID) : NULL;

	if (tagDef != m_lastFragTagDef)
	{
		PopulateTagList(tagDef);
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::PopulateTagList(const CTagDefinition *tagDef)
{
	m_lastFragTagDef = tagDef;

	m_tagVars->DeleteAllVariables();
	m_tagControls.Init(m_tagVars, m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef());

	if (tagDef)
	{
		m_fragTagControls.Init(m_tagVars, *tagDef);
	}

	m_wndTagsPanel->DeleteVars();
	m_wndTagsPanel->SetVarBlock(m_tagVars.get(), functor(*this, &CMannequinDialog::OnInternalVariableChange));

	SetTagStateOnCtrl(SFragTagState());
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnInternalVariableChange( IVariable *pVar )
{
	if (!m_bRefreshingTagCtrl)
	{
		SFragTagState tagState = GetTagStateFromCtrl();

		m_wndFragmentEditorPage.TrackPanel()->FlushChanges();

		Previewer()->SetUIFromHistory();
		TransitionEditor()->SetUIFromHistory();
		FragmentBrowser()->SetTagState(tagState);

		m_wndFragmentEditorPage.UpdateSelectedFragment();

		UpdateForFragment(FragmentBrowser()->GetSelectedFragmentId(), FragmentBrowser()->GetSelectedNodeText());
	}
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::SetTagStateOnCtrl(const SFragTagState &tagState)
{
	m_bRefreshingTagCtrl = true;
	m_tagControls.Set(tagState.globalTags);
	if (m_lastFragTagDef)
	{
		m_fragTagControls.Set(tagState.fragmentTags);
	}
	m_bRefreshingTagCtrl = false;
}

//////////////////////////////////////////////////////////////////////////
SFragTagState CMannequinDialog::GetTagStateFromCtrl() const
{
	SFragTagState ret;
	ret.globalTags = m_tagControls.Get();
	if (m_lastFragTagDef)
	{
		ret.fragmentTags = m_fragTagControls.Get();
	}

	return ret;
}

void CMannequinDialog::LoadMannequinFolder()
{
	// Load all files from the mannequin folder
	IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

	// Animation databases
	CFileUtil::FileArray adbFiles;
	CFileUtil::ScanDirectory(MANNEQUIN_FOLDER, "*.adb", adbFiles);
	for (CFileUtil::FileArray::const_iterator itAdbs = adbFiles.begin(); itAdbs != adbFiles.end(); ++itAdbs)
	{
		mannequinSys.GetAnimationDatabaseManager().Load(MANNEQUIN_FOLDER + itAdbs->filename);
	}

	// XMLs: can be tag definitions or controller definition files
	CFileUtil::FileArray xmlFiles;
	CFileUtil::ScanDirectory(MANNEQUIN_FOLDER, "*.xml", xmlFiles);
	for (CFileUtil::FileArray::const_iterator itXmls = xmlFiles.begin(); itXmls != xmlFiles.end(); ++itXmls)
	{
		const CTagDefinition* pTagDef = mannequinSys.GetAnimationDatabaseManager().LoadTagDefs(MANNEQUIN_FOLDER + itXmls->filename, true);
		if (!pTagDef)
		{
			// Failed to load as a tag definition: try as a controller definition file
			mannequinSys.GetAnimationDatabaseManager().LoadControllerDef(MANNEQUIN_FOLDER + itXmls->filename);
		}
	}
}

void CMannequinDialog::ActivateErrorReport()
{
	GetDockingPaneManager()->ShowPane(IDW_ERROR_REPORT_PANE);

	m_wndErrorReport.Refresh();
}

void CMannequinDialog::OnFragmentBrowserScopeContextChanged()
{
	UpdateAnimationDBEditorMenuItemEnabledState();
}

void CMannequinDialog::UpdateAnimationDBEditorMenuItemEnabledState()
{
	CXTPControl* const pAnimDBEditorCommand = GetCommandBars()->FindControl(xtpControlButton, ID_FILE_ANIMDBEDITOR, TRUE, TRUE);
	if (pAnimDBEditorCommand)
	{
		CRY_ASSERT(FragmentBrowser());
		const int scopeContextId = FragmentBrowser()->GetScopeContextId();

		const bool validScopeContextId = (0 <= scopeContextId || scopeContextId < m_contexts.m_contextData.size());
		const IAnimationDatabase* const pAnimDB = validScopeContextId ? m_contexts.m_contextData[scopeContextId].database : NULL;

		pAnimDBEditorCommand->SetFlags(xtpFlagManualUpdate);
		pAnimDBEditorCommand->SetEnabled((pAnimDB != NULL));
	}
}

void CMannequinDialog::EnableContextEditor(bool enable)
{
	CXTPControl* const pContextEditorCommand = GetCommandBars()->FindControl(xtpControlButton, ID_FILE_CONTEXTEDITOR, TRUE, TRUE);
	if (pContextEditorCommand)
	{
		pContextEditorCommand->SetFlags(xtpFlagManualUpdate);
		pContextEditorCommand->SetEnabled(enable);
	}
}


namespace MannequinInfoHelper
{
	struct SAnimAssetReportData
	{
		SAnimAssetReportData() : szBspace(""){}
		SAnimAssetReport assetReport;
		const char* szBspace;
	};
	typedef std::vector<SAnimAssetReportData> TAnimAssetsList;
	struct SAnimAssetCollector
	{
		SAnimAssetCollector(IAnimationSet& _animSet) : animSet(_animSet){}
		TAnimAssetsList assetList;
		IAnimationSet& animSet;
	};

	void OnAnimAssetReport(const SAnimAssetReport& animAssetReport, void* const pCallbackData)
	{
		SAnimAssetCollector* const pCollector = static_cast<SAnimAssetCollector*>(pCallbackData);
		SAnimAssetReportData reportData;
		reportData.assetReport = animAssetReport;
		pCollector->assetList.push_back(reportData);
		
		// Also register sub-animations (i.e. examples within blend-spaces)
		DynArray<int> subAnimIDs;
		pCollector->animSet.GetSubAnimations(subAnimIDs, animAssetReport.animID);
		const uint32 numSubAnims = subAnimIDs.size();
		for (uint32 itSubAnims = 0; itSubAnims < numSubAnims; ++itSubAnims)
		{
			const int subAnimID = subAnimIDs[itSubAnims];
			SAnimAssetReportData subReportData;
			subReportData.szBspace              = reportData.assetReport.pAnimPath;
			subReportData.assetReport.animID    = subAnimID;
			subReportData.assetReport.pAnimName = pCollector->animSet.GetNameByAnimID(subAnimID);
			subReportData.assetReport.pAnimPath = pCollector->animSet.GetFilePathByID(subAnimID);
			pCollector->assetList.push_back(subReportData);
		}
	}

	void ListUsedAnimationsHeader(FILE* pFile)
	{
		assert(pFile);

		fprintf(pFile, "Animation Name,");
		fprintf(pFile, "Blend-space,");
		fprintf(pFile, "Animation Path,");
		fprintf(pFile, "Skeleton Path,");
		fprintf(pFile, "Animation Events Path,");
		fprintf(pFile, "Animation Database Path,");
		fprintf(pFile, "Fragment Name,");
		fprintf(pFile, "Fragment TagState,");
		fprintf(pFile, "Destination Fragment Name,");
		fprintf(pFile, "Destination Fragment TagState,");
		fprintf(pFile, "Option Index,");
		fprintf(pFile, "Preview File Path");
		fprintf(pFile, "\n");
	}

	void ListUsedAnimations(FILE* pFile, const SMannequinContexts& contexts)
	{
		assert(pFile);

		const size_t scopeContextDataCount = contexts.m_contextData.size();
		for (size_t contextDataIndex = 0; contextDataIndex < scopeContextDataCount; ++contextDataIndex)
		{
			const SScopeContextData& scopeContextData = contexts.m_contextData[contextDataIndex];
			IAnimationDatabase* pAnimationDatabase = scopeContextData.database;
			IAnimationSet* pAnimationSet = scopeContextData.animSet;
			ICharacterInstance* pCharacterInstance = scopeContextData.viewData[eMEM_FragmentEditor].charInst.get();

			if (pAnimationDatabase == NULL || pAnimationSet == NULL  || pCharacterInstance == NULL)
			{
				continue;
			}

			SAnimAssetCollector animAssetCollector(*pAnimationSet);
			pAnimationDatabase->EnumerateAnimAssets(pAnimationSet, &OnAnimAssetReport, &animAssetCollector);

			for (size_t i = 0; i < animAssetCollector.assetList.size(); ++i)
			{
				const SAnimAssetReport& animAssetReport = animAssetCollector.assetList[i].assetReport;

				fprintf(pFile, "%s,", animAssetReport.pAnimName);

				fprintf(pFile, "%s,", animAssetCollector.assetList[i].szBspace);

				fprintf(pFile, "%s,", animAssetReport.pAnimPath ? animAssetReport.pAnimPath : "");

				const char* const modelPath = pCharacterInstance->GetIDefaultSkeleton().GetModelFilePath();
				fprintf(pFile, "%s,", modelPath);

				const char* const animEventsFile = pCharacterInstance->GetModelAnimEventDatabase();
				fprintf(pFile, "%s,", animEventsFile ? animEventsFile : "");

				const char* const animationDatabasePath = pAnimationDatabase->GetFilename();
				fprintf(pFile, "%s,", animationDatabasePath);

				const char* const fragmentName = scopeContextData.database->GetFragmentDefs().GetTagName(animAssetReport.fragID);
				fprintf(pFile, "%s,", fragmentName);

				char tagList[512];
				MannUtils::FlagsToTagList(tagList, arraysize(tagList), animAssetReport.tags, animAssetReport.fragID, *contexts.m_controllerDef, "");
				fprintf(pFile, "%s,", tagList);
				
				const char* const fragmentNameTo = animAssetReport.fragIDTo != FRAGMENT_ID_INVALID ? scopeContextData.database->GetFragmentDefs().GetTagName(animAssetReport.fragIDTo) : "";
				fprintf(pFile, "%s,", fragmentNameTo);

				char tagListTo[512] = {0};
				if (animAssetReport.fragIDTo != FRAGMENT_ID_INVALID)
				{
					MannUtils::FlagsToTagList(tagList, arraysize(tagList), animAssetReport.tagsTo, animAssetReport.fragIDTo, *contexts.m_controllerDef, "");
				}
				fprintf(pFile, "%s,", tagListTo);

				fprintf(pFile, "%d,", animAssetReport.fragOptionID);

				fprintf(pFile, "%s", contexts.previewFilename.c_str());

				fprintf(pFile, "\n");
			}
		}
	}
}


void CMannequinDialog::OnToolsListUsedAnimations()
{
	CString outputFilename = "MannequinUsedAnimations.csv";
	const CString saveFilters = "CSV Files (*.csv)|*.csv||";
	const bool userSaveOk = CFileUtil::SelectSaveFile(saveFilters, "csv", Path::GetGameFolder(), outputFilename);
	if (!userSaveOk)
	{
		return;
	}

	CWaitProgress waitProgress("Gathering list of used animations");

	FILE* pFile = fopen(static_cast<const char*>(outputFilename), "wt");
	if (pFile == NULL)
	{
		return;
	}

	MannequinInfoHelper::ListUsedAnimationsHeader(pFile);

	std::vector<string> previewFiles;
	const char* const filePattern = "*.xml";

	SDirectoryEnumeratorHelper dirHelper;
	dirHelper.ScanDirectoryRecursive("", "Animations/Mannequin/Preview/", filePattern, previewFiles);

	const size_t previewFileCount = previewFiles.size();
	for (size_t previewFileIndex = 0; previewFileIndex < previewFileCount; ++previewFileIndex)
	{
		const bool continueProcessing = waitProgress.Step( ( previewFileIndex * 100 ) / previewFileCount );
		if (!continueProcessing)
		{
			break;
		}

		const string& previewFile = previewFiles[previewFileIndex];
		const bool previewFileLoadSuccess = InitialiseToPreviewFile(previewFile.c_str());
		if (previewFileLoadSuccess)
		{
			MannequinInfoHelper::ListUsedAnimations(pFile, m_contexts);
		}
	}

	LoadNewPreviewFile(NULL); // Make the last loaded preview file valid

	fclose(pFile);
}


void CMannequinDialog::OnToolsListUsedAnimationsForCurrentPreview()
{
	CString outputFilename;
	outputFilename.Append("MannequinUsedAnimations_");
	outputFilename.Append(PathUtil::GetFileName(m_contexts.previewFilename).c_str());
	outputFilename.Append(".csv");
	const CString saveFilters = "CSV Files (*.csv)|*.csv||";
	const bool userSaveOk = CFileUtil::SelectSaveFile(saveFilters, "csv", Path::GetGameFolder(), outputFilename);
	if (!userSaveOk)
	{
		return;
	}

	CWaitProgress waitProgress("Gathering list of used animations");

	FILE* pFile = fopen(static_cast<const char*>(outputFilename), "wt");
	if (pFile == NULL)
	{
		return;
	}

	MannequinInfoHelper::ListUsedAnimationsHeader(pFile);
	MannequinInfoHelper::ListUsedAnimations(pFile, m_contexts);

	fclose(pFile);
}


void CMannequinDialog::EnableMenuCommand(uint32 commandId, bool enable)
{
	CXTPControl* const pContextEditorCommand = GetCommandBars()->FindControl(xtpControlButton, commandId, TRUE, TRUE);
	if (pContextEditorCommand)
	{
		pContextEditorCommand->SetFlags(xtpFlagManualUpdate);
		pContextEditorCommand->SetEnabled(enable);
	}
}
