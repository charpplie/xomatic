#include "StdAfx.h"
#include "ExportFunctions.h"
#include "materialExport.h"
#include "AnimationExportSource.h"
#include "GeometryExportSource.h"
#include "ILogFile.h"
#include "ResourceCompilerHelper.h"
#include "Export/ExportStatusWindow.h"
#include "Export/IExportContext.h"
#include "Export/ColladaExportWriter.h"
#include "ISettings.h"
#include "PathHelpers.h"
#include "LogFile.h"
#include "PakSystem.h"
#include "DebugCallStack.h"

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

class MayaCryExportContext : public IExportContext
{
public:
	MayaCryExportContext(ExportStatusWindow& statusWindow, ILogFile* logFile, IPakSystem* pakSystem, ISettings* settings)
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

class MayaCryExportSettings : public ISettings
{
public:
	MayaCryExportSettings();

	// ISettings
	virtual bool GetSettingString(char* buffer, int bufferSize, const char* key);
	virtual bool GetSettingInt(int& value, const char* key);

private:
	std::string GetModuleName() const;

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
				std::string logPath;
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
				MayaCryExportSettings settings;
				MayaCryExportContext context(window, &logFile, &pakSystem, &settings);
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
			/*	MessageBox(
					GetCOREInterface()->GetMAXHWnd(),
					"This scene has never been saved. The exporter needs the scene to be saved to disk so that\n"
					"it can figure out where to export files to.\n"
					"\n"
					"Please save the scene and try exporting again.",
					"Exporting Problem",
					MB_OK | MB_ICONERROR); */

				attemptCount = 0; // No point retrying, will fail again.
			}
			catch (RCPathMissingError e)
			{
			/*	MessageBox(
					GetCOREInterface()->GetMAXHWnd(),
					"The Exporter does not know where to find the Resource Compiler. This is stored in the bin32\n"
					"directory under the main build path.\n"
					"\n",
					"Exporting Problem",
					MB_OK | MB_ICONERROR);*/
				if (attemptCount > 1)
					rcNeedsConfig = true;
			}

			// If exporting failed because the RC path was not configured, let the user do so now.
			if (rcNeedsConfig)
				ConfigureRC();
		}
	}
}

void ExportFunctions::Export( cryExportOptions &options )
{
	if( options.exportType == kExportType_anim )
	{
		AnimationExportSource animSource;
		ExportFromSource( &animSource );
	}
	else if( options.exportType == kExportType_geom )
	{
		GeometryExportSource geomSource;
		ExportFromSource( &geomSource );
	}
	else if( options.exportType == kExportType_material )
	{
		generateMaterialFiles( );
	}
	else
	{
		// Unknown export type.
	}
}

void ExportFunctions::GetMetaData(SExportMetaData& metaData)
{
	strcpy( metaData.authoring_tool,"CryENGINE Maya COLLADA Exporter" );

	{
		metaData.source_data[0] = 0;
		MFileIO fio;
		MString currentFilename = fio.currentFile();

		std::string source = "file://";
		source.append( currentFilename.asChar() );
		strncpy( metaData.source_data, source.c_str(), sizeof(metaData.source_data)-1 );
		metaData.source_data[sizeof(metaData.source_data)-1] = 0;
	}
	{
		metaData.author[0] = 0;
		char szNameBuffer[sizeof(metaData.author)];
		memset(szNameBuffer, 0, sizeof(szNameBuffer));
		DWORD dwSize = sizeof(szNameBuffer);
		if (!::GetUserNameA(szNameBuffer, &dwSize))
		{
			szNameBuffer[0] = 0;
		}
		strcpy(metaData.author, szNameBuffer);
	}

	metaData.up_axis = SExportMetaData::Y_UP;
	if( MGlobal::isZAxisUp() )
		metaData.up_axis = SExportMetaData::Z_UP;
	metaData.fMeterUnit = 1.0f;
}

void MayaCryExportContext::SetProgress(float progress)
{
	m_statusWindow.SetProgress(progress);
}

void MayaCryExportContext::SetCurrentTask(const std::string& id)
{
	m_statusWindow.SetCurrentTask(id);
}

void MayaCryExportContext::Log(MessageSeverity severity, const char* message)
{
	m_statusWindow.Log((ExportStatusWindow::MessageSeverity) severity, message);
	if (m_logFile)
		m_logFile->Log((ILogFile::MessageSeverity)severity, message);
}

IPakSystem* MayaCryExportContext::GetPakSystem()
{
	return m_pakSystem;
}

ISettings* MayaCryExportContext::GetSettings()
{
	return m_settings;
}

void MayaCryExportContext::GetRootPath(char* buffer, int bufferSize)
{
	CEngineSettingsManager settingsManager;

	std::string rootPath = settingsManager.GetRootPath();

	if (rootPath.empty())
		settingsManager.CallSettingsDialog(0);

	strncpy(buffer, rootPath.c_str(), bufferSize);
}

MayaCryExportSettings::MayaCryExportSettings()
: m_engineSettings(GetModuleName().c_str())
{
}

bool MayaCryExportSettings::GetSettingString(char* buffer, int bufferSize, const char* key)
{
	string value;
	if (m_engineSettings.GetModuleSpecificEntry(key, value))
	{
		strncpy(buffer, value.c_str(), bufferSize);
		return true;
	}

	return false;
}

bool MayaCryExportSettings::GetSettingInt(int& value, const char* key)
{
	return (m_engineSettings.GetModuleSpecificEntry(key, value));
}

std::string MayaCryExportSettings::GetModuleName() const
{
	return "MayaCryExport";
}
