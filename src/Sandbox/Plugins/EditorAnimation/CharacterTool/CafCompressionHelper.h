// (c) 2001-2012 Crytek GmbH
#pragma once

#include "Strings.h"

struct SAnimSettings;

namespace CafCompressionHelper
{
	bool CompressAnimationForPreview( string* outputPath, string* errorMessage, const string& animationPath, const SAnimSettings& animSettings, bool ignorePresets, int sessionIndex );
	bool MoveCompressionResult(string* errorMessage, const char* createdFile, const char* destinationFile);
	void CleanUpCompressionResult(const char* createdFile);
	bool CompressAnimation( const string& animationPath, string& outErrorMessage, bool* failReported );

	bool CheckIfNeedUpdate( const char* animationPath );
}
