#include "stdafx.h"
#include "FFMPEGPlugin.h"
#include "CryString.h"
typedef CryStringT<char> string;
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include "Util/PathUtil.h"

namespace PluginInfo
{
	const char* kName = "FFMPEG Writer";
	const char* kGUID = "{D2A3A44A-00FF-4341-90BA-89A473F44A65}";
	const int kVersion = 1;
}

extern IEditor* g_pEditor;

void CFFMPEGPlugin::Release()
{
	g_pEditor->GetICommandManager()->UnregisterCommand(COMMAND_MODULE, COMMAND_NAME);
	delete this;
}

void CFFMPEGPlugin::ShowAbout()
{
}

const char* CFFMPEGPlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD CFFMPEGPlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CFFMPEGPlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CFFMPEGPlugin::CanExitNow()
{
	return true;
}

static void Command_FFMPEGEncode(const char *input, const char *output, const char *codec, int bitRateinKb, const char *etc)
{
	CString ffmpegCmdLine, outTxt;
	char dllPath[_MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), dllPath, sizeof(dllPath));
	char buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	_splitpath(dllPath, drive, dir, 0, 0);
	_makepath(buffer, drive, dir, 0, 0);
	CString pathOnly = buffer;
	ffmpegCmdLine.Format("%s..\\Editor\\Plugins\\ffmpeg.exe -i %s -vcodec %s -b %dk %s -strict experimental -y %s", 
		pathOnly, input, codec, bitRateinKb, etc, output);
	g_pEditor->GetSystem()->GetILog()->Log("Executing \"%s\" from FFMPEGPlugin...", ffmpegCmdLine.GetString());
	g_pEditor->ExecuteConsoleApp(ffmpegCmdLine, outTxt, true, true);
	g_pEditor->GetSystem()->GetILog()->Log("FFMPEG execution done.");
}

void CFFMPEGPlugin::RegisterTheCommand()
{
	CommandManagerHelper::RegisterCommand(g_pEditor->GetICommandManager(), 
		COMMAND_MODULE, COMMAND_NAME, "Encodes a video using ffmpeg.", 
		"plugin.ffmpeg_encode 'input.avi' 'result.mov' 'libx264' 200 30", 
		functor(Command_FFMPEGEncode));
}

void CFFMPEGPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}