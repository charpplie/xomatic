//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012
// ------------------------------------------------------------------------
//  File name:   AlembicCompiler.cpp
//  Created:     26/10/2012 by Axel Gneiting
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AlembicCompiler.h"
#include "AlembicCompileDialog.h"
#include "Dialogs/ResourceCompilerDialog.h"

bool CAlembicCompiler::CompileAlembic(CString &fileName, const CString &fullPath)
{
	const CString rcFilePath = Path::GetExecutableParentDirectory() / Path::GamePathToFullPath(fullPath);
	
	const CString configPath = Path::ReplaceExtension(rcFilePath, "cbc");
	XmlNodeRef config = XmlHelpers::LoadXmlFromFile(configPath);
	CAlembicCompileDialog dialog(config);
	
	if (dialog.DoModal() == IDOK)
	{
		const CString upAxis = dialog.GetUpAxis();
		const CString playbackFromMemory = dialog.GetPlaybackFromMemory();
		const CString blockCompressionFormat = dialog.GetBlockCompressionFormat();
		const CString meshPrediction = dialog.GetMeshPrediction();
		const CString useBFrames = dialog.GetUseBFrames();
		const uint indexFrameDistance = dialog.GetIndexFrameDistance();
		const double positionPrecision = dialog.GetPositionPrecision();
		const CString vertexIndexFormat = (sizeof(vtx_idx) == sizeof(uint16)) ? "u16" : "u32";

		CString randomFileName;
		randomFileName.Preallocate(16);

		srand(clock());
		for (unsigned int i = 0; i < 16; ++i)
		{
			switch(rand() % 3)
			{
			case 0:
				randomFileName += (char)('a' + (rand() % 26));
				break;
			case 1:
				randomFileName += (char)('A' + (rand() % 26));
				break;
			case 2:
				randomFileName += (char)('0' + (rand() % 10));
				break;
			}
		}

		CString additionalSettings;
		additionalSettings.Format("/refresh /threads=\"processors\" /overwritefilename=\"%s\" /upAxis=\"%s\" /playbackFromMemory=\"%s\""
			" /blockCompressionFormat=\"%s\" /meshPrediction=\"%s\" /useBFrames=\"%s\" /indexFrameDistance=\"%d\" /positionPrecision=\"%g\""
			" /vertexIndexFormat=\"%s\"", 
			randomFileName, upAxis, playbackFromMemory, blockCompressionFormat, meshPrediction, useBFrames, indexFrameDistance, positionPrecision, vertexIndexFormat);
		
		CString randomFilePath;
		Path::ReplaceFilename(rcFilePath, randomFileName, randomFilePath);
		randomFilePath = Path::ReplaceExtension(randomFilePath, CRY_GEOM_CACHE_FILE_EXT);

		bool bResult = false;

		CResourceCompilerDialog rcDialog(rcFilePath, additionalSettings, [&](const CResourceCompilerHelper::ERcCallResult result) {
			if (result == CResourceCompilerHelper::eRcCallResult_success)
			{
				// Move temp RC target to final file name				
				CString targetPath = Path::ReplaceExtension(rcFilePath, CRY_GEOM_CACHE_FILE_EXT);
				CFileUtil::MoveFile(randomFilePath, targetPath);

				// Return name of generated file
				fileName = Path::ReplaceExtension(fileName, CRY_GEOM_CACHE_FILE_EXT);		
				bResult = true;
			}
			else
			{
				CFileUtil::DeleteFileA(randomFilePath);
				bResult = false;
			}
		});

		rcDialog.DoModal();
		return bResult;
	}

	return false;
}
