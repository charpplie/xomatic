#ifndef  _ASSETNAME_CRYTOOLS_HEADER_
#define _ASSETNAME_CRYTOOLS_HEADER_

#pragma once

// convenience class to work with suffix like dirt_ddn.dds
// only the last suffix is used ( e.g. "test_ddn_bump.dds" has bump as suffix
// extension is always kept
// comparison is case in sensitive
// path and drive is part of the name e.g. "c:\temp\dirt_bump.tif"
class CSuffixUtil
{
public:

	// Arguments:
	//   szInput - must not be 0, e.g. "dirt_ddn_bump.dds"
	//   szSuffix - must not be 0 e.g. "bump"
	static bool CheckForSuffix( const char *szInputName, const char *szSuffix )
	{	
		assert(szInputName);
		assert(szSuffix);

		size_t nLen = strlen(szSuffix);

		const char *p=szInputName, *pLastUnderscore=0;

		while(*p!=0 && *p!='.')		// find last _
		{
			if(*p=='_')
				pLastUnderscore=p;

			++p;
		}

		if(pLastUnderscore)
		{
			pLastUnderscore++;																					// jump over suffix start character

			if(strnicmp(pLastUnderscore,szSuffix,nLen)==0)							// if this starts with the suffix name
			if(pLastUnderscore[nLen]=='.' || pLastUnderscore[nLen]==0)	// this is the full suffix name
				return true;																							// then we found it
		}

		return false;		// not found
	}

	// removes existing suffix and adds new one if provided
	// not highly optimized
	// Arguments:
	//   szSuffix - can be 0 to remove the suffix only
	static void ClearAndSetSuffix( string &sName, const char *szSuffix )
	{
		const string sNameCpy = sName;

		const char *p = sNameCpy.c_str(), *pLastUnderscore=0;

		while(*p!=0 && *p!='.')															// find last _
		{
			if(*p=='_')
				pLastUnderscore=p;

			++p;
		}

		sName.resize(pLastUnderscore-sNameCpy.c_str());			// reduce to name

		if(szSuffix)
		{
			sName.append("_");
			sName.append(szSuffix);
		}

		if(*p=='.')																					// if there was extension
			sName.append(p);																	// add extension
	}
};





#endif // _ASSETNAME_CRYTOOLS_HEADER_