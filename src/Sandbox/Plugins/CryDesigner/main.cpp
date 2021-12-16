#include "StdAfx.h"
#include "platform_impl.h"
#include "Include/IPlugin.h"
#include "Objects/DesignerBrushObject.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "Tools/AreaSolidEditTool.h"
#include "Tools/BrushDesignerEditTool.h"
#include "Tools/ClipVolumeEditTool.h"
#include "Core/BrushDesignerBinarySaveLoad.h"
#include "Core/BrushDesignerConverter.h"
#include "Core/BrushExporter.h"
#include "GameExporter.h"
#include "Util/BoostPythonHelpers.h"

IEditor* g_pEditor = NULL;
HINSTANCE g_hInst = NULL;

DECLARE_PYTHON_MODULE(designer);

class CEditorDesigner : public IPlugin
{
public:

	void Init()
	{
		IEditorClassFactory *cf = GetIEditor()->GetClassFactory();
		cf->RegisterClass(new CAreaSolidClassDesc);
		cf->RegisterClass(new CClipVolumeObjectClassDesc);
		cf->RegisterClass(new CDesignerBrushObjectClassDesc);
		cf->RegisterClass(new CSolidBrushObjectClassDesc);
		CRegistrationContext rc;
		rc.pCommandManager = GetIEditor()->GetCommandManager();
		rc.pClassFactory = (CClassFactory*)GetIEditor()->GetClassFactory();
		CBrushDesignerEditTool::RegisterTool(rc);
		CAreaSolidEditTool::RegisterTool(rc);
		CClipVolumeEditTool::RegisterTool(rc);
	}
	void Release() override{}
	void ShowAbout() override {}
	const char * GetPluginGUID() override  { return ""; }
	DWORD GetPluginVersion() override { return 1; }
	const char * GetPluginName() override { return "CryDesigner"; }
	bool CanExitNow() override { return true; }
	void OnEditorNotify(EEditorNotifyEvent aEventId) override
	{
		switch( aEventId )
		{
		case eNotify_OnEndLoad:
			{
				CBrushDesignerBinarySaveLoad brushDesignerBinarySaveLoad;
				brushDesignerBinarySaveLoad.Load();
				brushDesignerBinarySaveLoad.UpdateAllDesignerObjects(false);
			}
			break;
		case eNotify_OnConvertToDesignerObjects:
			{
				CBrushDesignerConverter designerConverter;
				if( !designerConverter.ConvertToDesignerObject() )
					CryMessageBox("The selected object(s) can't be converted.","Warning",MB_OK);
			}
			break;
		case eNotify_OnExportBrushes:
			{
				CGameExporter* pGameExporter = CGameExporter::GetCurrentExporter();
				if( pGameExporter )
				{
					CBrushExporter brushExport;
					CString path = Path::RemoveBackslash(Path::GetPath(pGameExporter->GetLevelPack().m_sPath));
					brushExport.ExportBrushes(path, pGameExporter->GetLevelPack().m_pakFile);
				}
			}
			break;
		}
		CEditTool* pEditTool = GetIEditor()->GetEditTool();
		if( pEditTool && pEditTool->IsKindOf(RUNTIME_CLASS(CBrushDesignerEditTool)) )
			((CBrushDesignerEditTool*)pEditTool)->OnEditorNotifyEvent(aEventId);
	}
};

IEditor* GetIEditor()
{
	return g_pEditor;
}

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return 0;
	}
	CEditorDesigner* pDesignerPlugin = new CEditorDesigner;
	g_pEditor = pInitParam->pIEditorInterface;
	pDesignerPlugin->Init();
	return pDesignerPlugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		g_hInst = hinstDLL;		
	}

	return TRUE;
}