#include "StdAfx.h"

#include "ColladaExportWriter.h"
#include "ColladaWriter.h"
#include "IExportSource.h"
#include "PathHelpers.h"
#include "ResourceCompilerHelper.h"
#include "IExportContext.h"
#include "ProgressRange.h"
#include "XMLWriter.h"
#include "XMLPakFileSink.h"
#include "ISettings.h"
#include "SingleAnimationExportSourceAdapter.h"
#include "GeometryExportSourceAdapter.h"
#include "ModelData.h"
#include "MaterialData.h"
#include "GeometryFileData.h"
#include "FileSystemHelpers.h"
#include "CBAHelpers.h"
#include "ModuleHelpers.h"
#include "PropertyHelpers.h"
#include "StringHelpers.h"
#include <ctime>
#include <list>

namespace
{
	class ResourceCompilerLogListener : public IResourceCompilerListener
	{
	public:
		ResourceCompilerLogListener(IExportContext* context): context(context) {}
		virtual void OnRCMessage(MessageSeverity severity, const char* text)
		{
			IExportContext::MessageSeverity logSeverity = (IExportContext::MessageSeverity)severity;

			// Normal RC text should just be debug.
			if (severity == MessageSeverity_Info)
				logSeverity = IExportContext::MessageSeverity_Debug;

			context->Log(logSeverity, text);
		}
	private:
		IExportContext* context;
	};
}

// Include build information - generated in pre-build step.
#include "Generated_BuildInfo.h"

void ColladaExportWriter::Export(IExportSource* source, IExportContext* context)
{
	// Create an object to report on our progress to the export context.
	ProgressRange progressRange(context, &IExportContext::SetProgress);

	// Log build information.
	Log(context, IExportContext::MessageSeverity_Info, "Exporter build created on " BUILD_DATE_STRING);
	Log(context, IExportContext::MessageSeverity_Info, "Build created by username " BUILD_USER_NAME);

#ifdef STLPORT
	Log(context, IExportContext::MessageSeverity_Info, "Using STLport C++ Standard Library implementation");
#else //STLPORT
	Log(context, IExportContext::MessageSeverity_Info, "Using Microsoft (tm) C++ Standard Library implementation");
#endif //STLPORT

#ifndef NDEBUG
	Log(context, IExportContext::MessageSeverity_Info, "******DEBUG BUILD******");
#else //_DEBUG
	Log(context, IExportContext::MessageSeverity_Info, "Release build.");
#endif //_DEBUG

	Log(context, IExportContext::MessageSeverity_Debug, "Bit count == %d.", (sizeof(void*) * 8));

	std::string exePath = StringHelpers::ConvertString<string>(ModuleHelpers::GetCurrentModulePath(ModuleHelpers::CurrentModuleSpecifier_Executable));
	Log(context, IExportContext::MessageSeverity_Debug, "Application path: %s", exePath.c_str());

	std::string exporterPath = StringHelpers::ConvertString<string>(ModuleHelpers::GetCurrentModulePath(ModuleHelpers::CurrentModuleSpecifier_Library));
	Log(context, IExportContext::MessageSeverity_Debug, "Exporter path: %s", exporterPath.c_str());

	bool const bExportCompressed = (GetSetting<int>(context->GetSettings(), _T("ExportCompressedCOLLADA"), 1)) != 0;
	Log(context, IExportContext::MessageSeverity_Debug, "ExportCompressedCOLLADA key: %d", (bExportCompressed ? 1 : 0));

	std::string const exportExtension = bExportCompressed ? ".dae.zip" : ".dae";

	// Log the start time.
	{
		char buf[1024];
		std::time_t t = std::time(0);
		std::strftime(buf, sizeof(buf) / sizeof(buf[0]), "%H:%M:%S on %a, %d/%m/%Y", std::localtime(&t));
		Log(context, IExportContext::MessageSeverity_Info, "Export begun at %s", buf);
	}

	// Select the name of the directory to export to.
	std::string const originalExportDirectory = source->GetExportDirectory();
	if (originalExportDirectory.empty())
	{
		throw IExportContext::NeedSaveError("Scene must be saved before exporting.");
	}

	GeometryFileData geometryFileData;
	std::vector<std::string> colladaGeometryFileNameList;
	std::vector<std::string> assetGeometryFileNameList;
	typedef std::vector<std::pair<std::pair<int, int>, std::string> > AnimationFileNameList;
	AnimationFileNameList animationFileNameList;
	{
		CurrentTaskScope currentTask(context, "dae");

		// Choose the files to which to export all the animations.
		std::list<SingleAnimationExportSourceAdapter> animationExportSources;
		std::list<GeometryExportSourceAdapter> geometryExportSources;
		typedef std::vector<std::pair<std::string, IExportSource*> > ExportList;
		ExportList exportList;
		std::vector<int> geometryFileIndices;
		{
			ProgressRange readProgressRange(progressRange, 0.2f);

			source->ReadGeometryFiles(context, &geometryFileData);

			for (int geometryFileIndex = 0; geometryFileIndex < geometryFileData.GetGeometryFileCount(); ++geometryFileIndex)
			{
				std::string const geometryFileName = geometryFileData.GetGeometryFileName(geometryFileIndex);
				int const fileTypeInt = geometryFileData.GetProperties(geometryFileIndex).filetypeInt;
				bool const hasGeometry = (fileTypeInt != CRY_FILE_TYPE_CAF);

				if (hasGeometry && !geometryFileName.empty())
				{
					geometryFileIndices.push_back(geometryFileIndex);
				}
			}

			if (!geometryFileIndices.empty())
			{
				std::string name = PathHelpers::RemoveExtension(PathHelpers::GetFilename(source->GetDCCFileName()));
				std::replace(name.begin(), name.end(), ' ', '_');
				std::string const colladaPath = PathHelpers::Join(originalExportDirectory, name + exportExtension);
				colladaGeometryFileNameList.push_back(colladaPath);

				geometryExportSources.push_back(GeometryExportSourceAdapter(source, &geometryFileData, geometryFileIndices));
				exportList.push_back(std::make_pair(colladaPath, &geometryExportSources.back()));
			}

			for (int geometryFileIndex = 0; geometryFileIndex < geometryFileData.GetGeometryFileCount(); ++geometryFileIndex)
			{
				std::string const geometryFileName = geometryFileData.GetGeometryFileName(geometryFileIndex);
				int const fileTypeInt = geometryFileData.GetProperties(geometryFileIndex).filetypeInt;
				bool const hasGeometry = (fileTypeInt != CRY_FILE_TYPE_CAF);

				if (hasGeometry && !geometryFileName.empty())
				{
					std::string extension = "missingextension";
					if (fileTypeInt == CRY_FILE_TYPE_CGF)
						extension = "cgf";
					else if ((fileTypeInt == CRY_FILE_TYPE_CGA) || (fileTypeInt == (CRY_FILE_TYPE_CGA|CRY_FILE_TYPE_ANM)))
						extension = "cga";
					else if (fileTypeInt == CRY_FILE_TYPE_ANM)
						extension = "anm";
					else if (fileTypeInt == CRY_FILE_TYPE_CHR || (fileTypeInt == (CRY_FILE_TYPE_CHR|CRY_FILE_TYPE_CAF)))
						extension = "chr";

					std::string safeGeometryFileName = geometryFileName;
					std::replace(safeGeometryFileName.begin(), safeGeometryFileName.end(), ' ', '_');
					std::string const assetPath = PathHelpers::Join(originalExportDirectory, safeGeometryFileName + "." + extension);
					assetGeometryFileNameList.push_back(assetPath);
				}

				for (int animationIndex = 0; animationIndex < source->GetAnimationCount(); ++animationIndex)
				{
					std::string const animationName = source->GetAnimationName(&geometryFileData, geometryFileIndex, animationIndex);

					// Animations beginning with an underscore should be ignored.
					bool const ignoreAnimation = animationName.empty() || (animationName[0] == '_');

					if (!ignoreAnimation)
					{
						std::string safeAnimationName = animationName;
						std::replace(safeAnimationName.begin(), safeAnimationName.end(), ' ', '_');

						std::string exportPath = PathHelpers::Join(originalExportDirectory, safeAnimationName + exportExtension);
						animationFileNameList.push_back(std::make_pair(std::make_pair(animationIndex, geometryFileIndex), exportPath));
						animationExportSources.push_back(SingleAnimationExportSourceAdapter(source, &geometryFileData, geometryFileIndex, animationIndex));
						exportList.push_back(std::make_pair(exportPath, &animationExportSources.back()));
					}
				}
			}
		}

		// Export the COLLADA file to the chosen file.
		{
			ProgressRange exportProgressRange(progressRange, 0.6f);

			size_t const daeCount = exportList.size();
			float const daeProgressRangeSlice = 1.0f / (daeCount > 0 ? daeCount : 1);
			for (ExportList::iterator itFile = exportList.begin(); itFile != exportList.end(); ++itFile)
			{
				const std::string& colladaFileName = (*itFile).first;
				IExportSource* fileExportSource = (*itFile).second;

				ProgressRange animationExportProgressRange(exportProgressRange, daeProgressRangeSlice);

				try
				{
					Log(context, IExportContext::MessageSeverity_Info, "Exporting to file '%s'", colladaFileName.c_str());

					// Try to create the directory for the file.
					FileSystemHelpers::EnsureDirectoryExists(StringHelpers::ConvertString<tstring>(PathHelpers::GetDirectory(colladaFileName)));

					bool ok;

					if (bExportCompressed)
					{
						IPakSystem* pakSystem = (context ? context->GetPakSystem() : 0);	
						if (!pakSystem)
						{
							throw IExportContext::PakSystemError("No pak system provided.");
						}

						std::string const archivePath = colladaFileName;
						std::string archiveRelativePath = colladaFileName.substr(0, colladaFileName.length() - exportExtension.length()) + ".dae";
						archiveRelativePath = PathHelpers::GetFilename(archiveRelativePath);

						XMLPakFileSink sink(pakSystem, archivePath, archiveRelativePath);
						ok = ColladaWriter::Write(fileExportSource, context, &sink, animationExportProgressRange);
					}
					else
					{
						XMLFileSink fileSink(colladaFileName);
						ok = ColladaWriter::Write(fileExportSource, context, &fileSink, animationExportProgressRange);
					}

					if (!ok)
					{
						// FIXME: erase the resulting file somehow
						Log(context, IExportContext::MessageSeverity_Error, "Failed to export '%s'", colladaFileName.c_str());
						return;
					}
				}
				catch (FileSystemHelpers::DirectoryCreationFailedError e)
				{
					Log(context, IExportContext::MessageSeverity_Error, "Unable to create directory: %s", e.what());
				}
				catch (IXMLSink::OpenFailedError e)
				{
					Log(context, IExportContext::MessageSeverity_Error, "Unable to open output file: %s", e.what());
				}
				catch (...)
				{
					Log(context, IExportContext::MessageSeverity_Error, "Unexpected crash in COLLADA exporter");
				}
			}
		}
	}

	// Run the resource compiler on the COLLADA file to generate the CAFs.
	{
		ProgressRange compilerProgressRange(progressRange, 0.075f);

		CurrentTaskScope currentTask(context, "rc");

		size_t const daeCount = animationFileNameList.size();
		float const animationProgressRangeSlice = 1.0f / (daeCount > 0 ? daeCount : 1);
		for (AnimationFileNameList::iterator itFile = animationFileNameList.begin(); itFile != animationFileNameList.end(); ++itFile)
		{
			std::string colladaFileName = (*itFile).second;

			ProgressRange animationCompileProgressRange(compilerProgressRange, animationProgressRangeSlice);
			ResourceCompilerLogListener listener(context);
			CResourceCompilerHelper rcHelper;
			Log(context, IExportContext::MessageSeverity_Info, "Calling RC to generate uncompressed CAF file: %s", colladaFileName.c_str());
			rcHelper.CallResourceCompiler(
				StringHelpers::ConvertString<tstring>(colladaFileName).c_str(), 
				StringHelpers::ConvertString<tstring>("/refresh").c_str(), 
				&listener);
			Log(context, IExportContext::MessageSeverity_Debug, "RC finished: %s", colladaFileName.c_str());
		}
	}

	// Run the resource compiler on the COLLADA file to generate the geometry assets.
	{
		ProgressRange compilerProgressRange(progressRange, 0.075f);

		CurrentTaskScope currentTask(context, "rc");

		size_t const daeCount = colladaGeometryFileNameList.size();
		float const assetProgressRangeSlice = 1.0f / (daeCount > 0 ? daeCount : 1);
		for (size_t i = 0; i < daeCount; ++i)
		{
			const std::string& colladaFileName = colladaGeometryFileNameList[i];

			ProgressRange assetCompileProgressRange(compilerProgressRange, assetProgressRangeSlice);
			ResourceCompilerLogListener listener(context);
			CResourceCompilerHelper rcHelper;
			Log(context, IExportContext::MessageSeverity_Info, "Calling RC to generate raw asset file: %s", colladaFileName.c_str());
			rcHelper.CallResourceCompiler(
				StringHelpers::ConvertString<tstring>(colladaFileName).c_str(), 
				StringHelpers::ConvertString<tstring>("/refresh").c_str(), 
				&listener);
			Log(context, IExportContext::MessageSeverity_Debug, "RC finished: %s", colladaFileName.c_str());
		}
	}

	{
		// Create an RC helper - do it outside the loop, since it queries the registry on construction.
		ResourceCompilerLogListener listener(context);
		CResourceCompilerHelper rcHelper;

		// Check the registry to see whether we should compress the animations or not.
		int processAnimations = GetSetting<int>(context->GetSettings(), _T("CompressCAFs"), 1);

		// Run the resource compiler again on the generated CAF files to compress/process them.
		// TODO: This should not be necessary, the RC should be modified so that CAFs are automatically
		// compressed when exported from COLLADA.
		if (!processAnimations)
		{
			Log(context, IExportContext::MessageSeverity_Warning, "CompressCAFs registry key set to 0 - not compressing CAFs");
		}
		else
		{
			Log(context, IExportContext::MessageSeverity_Debug, "CompressCAFs not set or set to 1 - compressing CAFs");

			CurrentTaskScope currentTask(context, "compress");
			ProgressRange compressRange(progressRange, 0.025f);

			size_t const cafCount = animationFileNameList.size();
			float const animationProgressRangeSlice = 1.0f / (cafCount > 0 ? cafCount : 1);
			for (AnimationFileNameList::iterator itFile = animationFileNameList.begin(); itFile != animationFileNameList.end(); ++itFile)
			{
				std::string colladaFileName = (*itFile).second;
				ProgressRange animationProgressRange(compressRange, animationProgressRangeSlice);

				// Assume the RC generated the CAF file using the take name and adding .CAF.
				std::string cafPath = colladaFileName.substr(0, colladaFileName.length() - exportExtension.length()) + ".caf";

				std::string cbaPath = StringHelpers::ConvertString<string>(CBAHelpers::FindCBAFileForFile(
					StringHelpers::ConvertString<tstring>(cafPath), context->GetPakSystem()));

				if (cbaPath.empty())
				{
					Log(context, IExportContext::MessageSeverity_Error, "Unable to find CBA file for file \"%s\" (looked for a root game directory that contains a relative path of \"Animations/Animations.cba\"", cafPath.c_str());
				}
				else
				{
					char buffer[2048];
					sprintf(buffer, "/file=\"%s\" /refresh /SkipDba", cafPath.c_str());
					Log(context, IExportContext::MessageSeverity_Info, "Calling RC to compress CAF file: (CBA file = %s) %s", cbaPath.c_str(), buffer);
					rcHelper.CallResourceCompiler(StringHelpers::ConvertString<tstring>(cbaPath).c_str(), StringHelpers::ConvertString<tstring>(buffer).c_str(), &listener);
					Log(context, IExportContext::MessageSeverity_Debug, "RC finished: %s %s", cbaPath.c_str(), buffer);
				}
			}
		}

		// Check the registry to see whether we should optimize the geometry files or not.
		int optimizeGeometry = GetSetting<int>(context->GetSettings(), _T("OptimizeAssets"), 1);

		// Run the resource compiler again on the generated geometry files to compress/process them.
		// TODO: This should not be necessary, the RC should be modified so that assets are automatically
		// compressed when exported from COLLADA.
		if (!optimizeGeometry)
		{
			Log(context, IExportContext::MessageSeverity_Warning, "OptimizeAssets registry key set to 0 - not compressing CAFs");
		}
		else
		{
			Log(context, IExportContext::MessageSeverity_Debug, "OptimizeAssets not set or set to 1 - optimizing geometry");

			CurrentTaskScope currentTask(context, "compress");
			ProgressRange compressRange(progressRange, 0.025f);

			size_t const assetCount = assetGeometryFileNameList.size();
			float const assetProgressRangeSlice = 1.0f / (assetCount > 0 ? assetCount : 1);
			for (size_t i = 0; i < assetCount; ++i)
			{
				const std::string& assetFileName = assetGeometryFileNameList[i];
				ProgressRange animationProgressRange(compressRange, assetProgressRangeSlice);

				// note: we skip some asset types because we know that they are "optimized" already
				if (StringHelpers::EndsWithIgnoreCase(assetFileName, ".anm") || StringHelpers::EndsWithIgnoreCase(assetFileName, ".chr"))
				{
					Log(context, IExportContext::MessageSeverity_Info, "Calling RC to optimize asset \"%s\"", assetFileName.c_str());
					rcHelper.CallResourceCompiler(StringHelpers::ConvertString<tstring>(assetFileName).c_str(), StringHelpers::ConvertString<tstring>("/refresh").c_str(), &listener);
					Log(context, IExportContext::MessageSeverity_Debug, "RC finished: %s", assetFileName.c_str());
				}
			}
		}
	}

	// Delete .rcdone files created by RC to avoid polluting user's folders
	for (size_t i = 0; i < colladaGeometryFileNameList.size(); ++i)
	{
		const std::string& colladaFileName = colladaGeometryFileNameList[i];
		remove((colladaFileName + ".rcdone").c_str());
	}

	// Log the end time.
	{
		char buf[1024];
		std::time_t t = std::time(0);
		std::strftime(buf, sizeof(buf) / sizeof(buf[0]), "%H:%M:%S on %a, %d/%m/%Y", std::localtime(&t));
		Log(context, IExportContext::MessageSeverity_Info, "Export finished at %s", buf);
	}
}
