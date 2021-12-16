#ifndef __FRONTENDPARSER_H__
#define __FRONTENDPARSER_H__

#include "FrontEnd/MenuData.h"

//////////////////////////////////////////////////////////////////////////


class CFrontEndParser
{
public:

	enum EFlashScreens
	{
		eFlS_MainMenu,
		eFlS_IngameMenu,
		eFlS_LoadingScreen,
		eFlS_IdleScreen
	};

	CFrontEndParser();
	virtual ~CFrontEndParser();

	void Read(CMenuData& outData, const bool gameRunning, const bool gameSelect) const;
	const char* AssembleCurrentPath(const bool gameRunning, const bool gameSelect) const;

private:

	void ReadFromFile(CMenuData& outData, const char* path) const;
	CMenuScreen* ReadScreen(const IItemParamsNode* child) const;
};


//////////////////////////////////////////////////////////////////////////

#endif
