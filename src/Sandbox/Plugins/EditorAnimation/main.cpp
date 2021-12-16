// (c) 2001-2012 Crytek GmbH
#include "pch.h"

#include "platform_impl.h"
#include "IEditorClassFactory.h"
#include "ICommandManager.h"
#include "IPlugin.h"
#include "IEventLoopHook.h"
#include "IResourceSelectorHost.h"
#include "AnimationCompressionManager.h"
#include "CharacterTool/CharacterToolForm.h"
#include "../EditorCommon/QtViewPane.h"

// just for CGFContent:
#include <VertexFormats.h>
#include <Cry_Geo.h>
#include <TypeInfo_impl.h>
#include <Common_TypeInfo.h>
#include <IIndexedMesh_info.h>
#include <CGFContent_info.h>
// ^^^

// we need this pile of crap...
class CXmlArchive;
struct SRayHitInfo;
#include <Cry_Geo.h>

#include "Serialization.h"
#include "Serialization/Pointers.h"
#include "Serialization/ITextInputArchive.h"
#include "Serialization/ITextOutputArchive.h"

#include "CharacterTool/CharacterToolForm.h"
#include "CharacterTool/CharacterToolSystem.h"

using Serialization::SharedPtr;

static IEditor* g_pEditor;

void Log( const char *format, ... )
{
	va_list args;
	va_start(args,format);
	g_pEditor->GetSystem()->GetILog()->LogV( ILog::eAlways, format, args );
	va_end(args);
}

IEditor* GetIEditor()
{
	return g_pEditor;
}

CharacterTool::System* g_pCharacterToolSystem;

// ---------------------------------------------------------------------------

class CEditorAnimationPlugin : public IPlugin
{
public:
	CEditorAnimationPlugin(IEditor* pEditor)
	: m_paneRegistered(false)
	{
	}

	void Init()
	{
		m_paneRegistered = RegisterQtViewPane<CharacterTool::CharacterToolForm>(GetIEditor(), "Character Tool", "Animation");
		RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());

		g_pCharacterToolSystem = new CharacterTool::System();
		g_pCharacterToolSystem->Initialize();

		g_pCharacterToolSystem->animationCompressionManager.reset(new CAnimationCompressionManager());
	}


	void Release()
	{
		if (g_pCharacterToolSystem)
		{
			delete g_pCharacterToolSystem;
			g_pCharacterToolSystem = 0;
		}

		UnregisterCommands();
		if (m_paneRegistered)
			UnregisterQtViewPane<CharacterTool::CharacterToolForm>();
		delete this;
	}

	void RegisterCommands()
	{
		ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();

	}

	void UnregisterCommands()
	{
		ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
		pCommandManager->UnregisterCommand("character_tool", "open_editor");
	}

	// implements IEditorNotifyListener
	void OnEditorNotify( EEditorNotifyEvent event )
	{
		switch (event)
		{
		case eNotify_OnInit:
			break;
		case eNotify_OnSelectionChange:
		case eNotify_OnEndNewScene:
		case eNotify_OnEndSceneOpen:
			break;
		}
	}

	// implements IPlugin
	void ShowAbout() { }
	const char * GetPluginGUID() { return 0; }
	DWORD GetPluginVersion() { return 0x01; }
	const char * GetPluginName() { return "EditorPhysics"; }
	bool CanExitNow() { return true; }
	void Serialize(FILE *hFile, bool bIsStoring) { }
	void ResetContent() { }
	bool CreateUIElements() { return false; }
	bool ExportDataToGame(const char * pszGamePath) { return false; }
private:

	bool m_paneRegistered;
};

//////////////////////////////////////////////////////////////////////////-

static HINSTANCE g_hInstance = 0;

PLUGIN_API IPlugin * CreatePluginInstance(PLUGIN_INIT_PARAM * pInitParam)
{
	g_pEditor = pInitParam->pIEditorInterface;
	ModuleInitISystem(g_pEditor->GetSystem(), "EditorAnimation");

	CEditorAnimationPlugin* pPlugin = new CEditorAnimationPlugin(pInitParam->pIEditorInterface);

	pPlugin->Init();

	return pPlugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
     g_hInstance = hinstDLL;
   }
	 
   return(TRUE);
}
