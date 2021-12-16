#include "StdAfx.h"
#include "CBAHelpers.h"
#include "../PathHelpers.h"
#include "StringHelpers.h"

tstring CBAHelpers::FindCBAFileForFile(const tstring& filePath, IPakSystem* pakSystem)
{
	const tstring relativeCBAPath = _T("Animations/Animations.cba");

	// Here we just search upwards from the current directory, looking for a directory that
	// contains a file at the relative path "Animations/Animations.cba". This is designed to
	// handle root Game paths that differ from the default "Game".
	tstring rootDirCandidate = PathHelpers::GetDirectory(filePath);
	tstring rootDir;
	while (!rootDirCandidate.empty())
	{
		tstring cbaCandidatePath = PathHelpers::Join(rootDirCandidate, relativeCBAPath);
		if (PakSystemFile* file = pakSystem->Open(StringHelpers::ConvertString<string>(cbaCandidatePath).c_str(), "r"))
		{
			// File exists, we have found the correct root path.
			pakSystem->Close(file);

			rootDir = rootDirCandidate;
			break;
		}

		rootDirCandidate = PathHelpers::GetDirectory(rootDirCandidate);
	}

	return (rootDir.empty() ? rootDir : PathHelpers::Join(rootDir, relativeCBAPath));
}
