#include "StdAfx.h"
#include "Export.h"
#include "AnimationExportSource.h"
#include "GeometryExportSource.h"
#include "ILogFile.h"
#include "ResourceCompilerHelper.h"
#include "Export/ExportStatusWindow.h"
#include "Export/IExportContext.h"
#include "Export/ColladaExportWriter.h"
#include "Export/ExportNodeHelpers.h"
#include "ISettings.h"
#include "PathHelpers.h"
#include "LogFile.h"
#include "PakSystem.h"
#include "DebugCallStack.h"
#include "StringHelpers.h"
#include "Export/IExportSource.h"
#include "check.h"

#include <xsi_uitoolkit.h>
#include <xsi_model.h>
#include <xsi_application.h>

struct RCPathMissingErrorTag {};
typedef Exception<RCPathMissingErrorTag> RCPathMissingError;

namespace
{
	void CheckResourceCompiler(ILogFile* log)
	{
		log->Log(ILogFile::MessageSeverity_Debug, "Checking resource compiler.");
		CResourceCompilerHelper rcHelper;
		switch (rcHelper.CallResourceCompiler(0, 0, 0, false, false, CResourceCompilerHelper::eRcExePath_registry, true, true))
		{
		case CResourceCompilerHelper::eRcCallResult_notFound:
			log->Log(ILogFile::MessageSeverity_Error, "Resource compiler not found.");
			throw RCPathMissingError("RC cannot be executed.");
		case CResourceCompilerHelper::eRcCallResult_error:
			log->Log(ILogFile::MessageSeverity_Error, "Resource compiler error.");
			throw RCPathMissingError("RC cannot be executed.");
		case CResourceCompilerHelper::eRcCallResult_success:
			log->Log(ILogFile::MessageSeverity_Debug, "Resource compiler check passed.");
			break;
		default:
			log->Log(ILogFile::MessageSeverity_Error, "Resource compiler unknown failure.");
			throw RCPathMissingError("RC cannot be executed.");
		}
	}
}

class XSICryExportContext : public IExportContext
{
public:
	XSICryExportContext(ExportStatusWindow& statusWindow, ILogFile* logFile, IPakSystem* pakSystem, ISettings* settings)
		: m_statusWindow(statusWindow), m_logFile(logFile), m_pakSystem(pakSystem), m_settings(settings) {}

	// IExportContext
	virtual void SetProgress(float progress);
	virtual void SetCurrentTask(const std::string& id);
	virtual void Log(MessageSeverity severity, const char* message);
	virtual IPakSystem* GetPakSystem();
	virtual ISettings* GetSettings();
	virtual void GetRootPath(char* buffer, int bufferSize);

private:
	ExportStatusWindow& m_statusWindow;
	ILogFile* m_logFile;
	IPakSystem* m_pakSystem;
	ISettings* m_settings;
};

class XSICryExportSettings : public ISettings
{
public:
	XSICryExportSettings();

	// ISettings
	virtual bool GetSettingString(TCHAR* buffer, int bufferSize, const TCHAR* key);
	virtual bool GetSettingInt(int& value, const TCHAR* key);

private:
	tstring GetModuleName() const;

	CEngineSettingsManager m_engineSettings;
};

namespace
{
	void ConfigureRC()
	{
		CResourceCompilerHelper rcHelper;
		rcHelper.ResourceCompilerUI(0);
	}

	void ExportFromSource(IExportSource* source)
	{
		if (!Diagnostics(false))
		{
			return;			
		}

		ColladaExportWriter writer;

		bool retry = true;
		bool rcNeedsConfig = false;

		// In some cases we might need to try exporting several times, so loop until we decide to stop.
		for (int attemptCount = 2; attemptCount > 0; --attemptCount)
		{
			rcNeedsConfig = false;

			try
			{
				// Create the log file.
				string logPath;
				{
					std::string const filename = source->GetDCCFileName();
					std::string const dir = source->GetExportDirectory();
					if (filename.empty() || dir.empty())
					{
						throw IExportContext::NeedSaveError("Scene must be saved before exporting.");
					}
					logPath = PathHelpers::Join(dir, PathHelpers::RemoveExtension(PathHelpers::GetFilename(filename)) + ".exportlog");
				}
				LogFile logFile(logPath);

				// Check whether the RC exists and can be found.
				CheckResourceCompiler(&logFile);

				std::vector<std::pair<std::string, std::string> > tasks;
				tasks.push_back(std::make_pair("dae", "Generating intermediate file (*.dae)."));
				tasks.push_back(std::make_pair("rc", "Compiling intermediate file to engine format."));
				tasks.push_back(std::make_pair("compress", "Compressing animations in engine format."));
				ExportStatusWindow window(400, 600, tasks);

				PakSystem pakSystem;
				XSICryExportSettings settings;
				XSICryExportContext context(window, &logFile, &pakSystem, &settings);
				if (!logFile.IsOpen())
				{
					char buffer[2048];
					sprintf(buffer, "Unable to open log file for writing: \"%s\"", logPath.c_str());
					context.Log(IExportContext::MessageSeverity_Warning, buffer);
				}

				// Handle any crashes.
				DebugCallStack::ErrorHandlerScope errorHandlerScope(&logFile);

				// Perform the export.
				writer.Export(source, &context);

				attemptCount = 0; // If we got here, make no more attempts to export.
			}
			catch (IExportContext::NeedSaveError e)
			{
				// get the XSIUIToolkit object
				LONG result;
				XSI::UIToolkit XSIUIToolkit;
				XSIUIToolkit.MsgBox(
					L"This scene has never been saved. The exporter needs the scene to be saved to disk so that\n"
					L"it can figure out where to export files to.\n"
					L"\n"
					L"Please save the scene and try exporting again.",
					XSI::siMsgOkOnly | XSI::siMsgExclamation,
					L"Exporting Problem",
					result);

				attemptCount = 0; // No point retrying, will fail again.
			}
			catch (RCPathMissingError e)
			{
				LONG result;
				XSI::UIToolkit XSIUIToolkit;
				XSIUIToolkit.MsgBox(
					L"The Exporter does not know where to find the Resource Compiler. This is stored in the bin32\n"
					L"directory under the main build path.\n"
					L"\n",
					XSI::siMsgOkOnly | XSI::siMsgExclamation,
					L"Exporting Problem",
					result);
				if (attemptCount > 1)
					rcNeedsConfig = true;
			}

			// If exporting failed because the RC path was not configured, let the user do so now.
			if (rcNeedsConfig)
				ConfigureRC();
		}
	}
}

void Export::Export(const XSI::CRefArray& exportNodes)
{
	GeometryExportSource source(exportNodes);
	ExportFromSource(&source);
}

namespace
{
	void RecurseAndFindModelsWithAnimations(XSI::CRefArray& models, XSI::Model& model)
	{
		XSI::CRefArray sources = model.GetSources();
		if (sources.GetCount() > 0)
			models.Add(model.GetRef());

		for (int childIndex = 0, childCount = model.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			RecurseAndFindModelsWithAnimations(models, XSI::Model(model.GetChildren().GetItem(childIndex)));
	}
}

void Export::ExportAnims()
{
	XSI::Application application;
	XSI::CRefArray models;
	RecurseAndFindModelsWithAnimations(models, application.GetActiveSceneRoot());

	for (int modelIndex = 0, modelCount = models.GetCount(); modelIndex < modelCount; ++modelIndex)
	{
		XSI::Model model = models[modelIndex];
		XSI::CRefArray sources = model.GetSources();
		for (int sourceIndex = 0, sourceCount = sources.GetCount(); sourceIndex < sourceCount; ++sourceIndex)
		{
			XSI::ActionSource source = sources[sourceIndex];

			XSI::CRefArray roots = model.GetChildren();
			AnimationExportSource exportSource(source, roots);
			ExportFromSource(&exportSource);
		}
	}
}

namespace
{
	void RecurseHierarchyAndFindExportNodes(XSI::CRefArray& exportNodes, const XSI::X3DObject& object)
	{
		// Check whether this node is the root of a geometry file.
		if (ExportNodeHelpers::IsCryExportNode(object))
		{
			std::string geometryFileName = ExportNodeHelpers::GetGeometryFileName(object);

			// Add the model to the model data.
			exportNodes.Add(object.GetRef());
		}
		else
		{
			// Recurse to the node's children.
			for (int childIndex = 0, childCount = object.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			{
				RecurseHierarchyAndFindExportNodes(exportNodes, XSI::X3DObject(object.GetChildren().GetItem(childIndex)));
			}
		}
	}
}

XSI::CRefArray Export::FindAllExportNodes()
{
	XSI::CRefArray exportNodes;
	XSI::Application application;
	RecurseHierarchyAndFindExportNodes(exportNodes, application.GetActiveSceneRoot());

	return exportNodes;
}

//////////////////////////////////////////////////////////////////////////
void Export::GetMetaData( SExportMetaData &metaData )
{
	strcpy( metaData.authoring_tool,"CryENGINE XSI COLLADA Exporter" );
	metaData.up_axis = SExportMetaData::Y_UP;
	metaData.fMeterUnit = 1.0f;

	XSI::Application app;
	XSI::Scene scn = app.GetActiveProject().GetActiveScene();
	XSI::CString fullFilename = scn.GetParameterValue(L"Filename");
	XSI::CString urlFilename = XSI::CString(L"file://") + fullFilename;
	strcpy( metaData.source_data,urlFilename.GetAsciiString() );

	{
		char szNameBuffer[sizeof(metaData.author)];
		memset(szNameBuffer, 0, sizeof(szNameBuffer));
		DWORD dwSize = sizeof(szNameBuffer);
		if (!::GetUserNameA(szNameBuffer, &dwSize))
		{
			szNameBuffer[0] = 0;
		}
		strcpy(metaData.author, szNameBuffer);
	}
}

void XSICryExportContext::SetProgress(float progress)
{
	m_statusWindow.SetProgress(progress);
}

void XSICryExportContext::SetCurrentTask(const std::string& id)
{
	m_statusWindow.SetCurrentTask(id);
}

void XSICryExportContext::Log(MessageSeverity severity, const char* message)
{
	m_statusWindow.Log((ExportStatusWindow::MessageSeverity) severity, message);
	if (m_logFile)
		m_logFile->Log((ILogFile::MessageSeverity)severity, message);
}

IPakSystem* XSICryExportContext::GetPakSystem()
{
	return m_pakSystem;
}

ISettings* XSICryExportContext::GetSettings()
{
	return m_settings;
}

void XSICryExportContext::GetRootPath(char* buffer, int bufferSize)
{
	CEngineSettingsManager settingsManager;

	std::string rootPath = StringHelpers::ConvertString<string>(settingsManager.GetRootPath());

	if (rootPath.empty())
		settingsManager.CallSettingsDialog(0); // TODO: Get top-level window.

	strncpy(buffer, rootPath.c_str(), bufferSize);
}

XSICryExportSettings::XSICryExportSettings()
: m_engineSettings(GetModuleName().c_str())
{
}

bool XSICryExportSettings::GetSettingString(TCHAR* buffer, int bufferSize, const TCHAR* key)
{
	tstring value;
	if (m_engineSettings.GetModuleSpecificEntry(key, value))
	{
		_tcsncpy(buffer, value.c_str(), bufferSize);
		return true;
	}

	return false;
}

bool XSICryExportSettings::GetSettingInt(int& value, const TCHAR* key)
{
	return (m_engineSettings.GetModuleSpecificEntry(key, value));
}

tstring XSICryExportSettings::GetModuleName() const
{
	return L"XSICryExport";
}
