#include "StdAfx.h"
#include "ChunkFileHelpers.h"
#include "..\..\CryEngine\Cry3DEngine\CGF\ChunkFile.h"


bool ChunkFileHelpers::RealignChunks(const string& sourceFile, const string& outputFile)
{
	CChunkFile chunkFile;

	if (!chunkFile.Read(sourceFile.c_str()))
	{
		const char* errorMsg = chunkFile.GetLastError();
		if (!errorMsg || !errorMsg[0])
		{
			errorMsg = "UNKNOWN ERROR";
		}
		RCLogError("Failed to load chunk file %s - %s", sourceFile.c_str(), errorMsg);
		return false;
	}

	switch (chunkFile.GetFileHeader().Version)
	{
	case ChunkFileVersion:
		RCLog("RealignChunks: file %s is already aligned", sourceFile.c_str());
		return true;
	case ChunkFileVersion_Align:
		// Force remove of the read only flag
		SetFileAttributes(outputFile, FILE_ATTRIBUTE_ARCHIVE);
		if (!chunkFile.Write(outputFile))
		{
			RCLogError("RealignChunks: failed to write file %s", outputFile.c_str());
			return false;
		}
		RCLog("RealignChunks: aligning %s to %s: done", sourceFile.c_str(), outputFile.c_str());
		return true;
	default:
		RCLogError("RealignChunks: Unknown chunk file version %d found in %s", chunkFile.GetFileHeader().Version, sourceFile.c_str());
		return false;
	}
}
