#ifndef __CBAHELPERS_H__
#define __CBAHELPERS_H__

#include "IPakSystem.h"

namespace CBAHelpers
{
	tstring FindCBAFileForFile(const tstring& filePath, IPakSystem* pakSystem);
}

#endif //__CBAHELPERS_H__
