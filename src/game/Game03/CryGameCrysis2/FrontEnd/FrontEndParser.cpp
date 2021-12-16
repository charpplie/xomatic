#include "StdAfx.h"

#include "FrontEndParser.h"
#include "IItemSystem.h"
#include "GameCVars.h"

//////////////////////////////////////////////////////////////////////////


CFrontEndParser::CFrontEndParser()
{
}



CFrontEndParser::~CFrontEndParser()
{
}



void CFrontEndParser::Read(CMenuData& outData, const bool gameRunning, const bool gameSelect) const
{
	const char* path = AssembleCurrentPath(gameRunning, gameSelect);
	outData.SetPath(path);
	ReadFromFile(outData, path);
}



const char* CFrontEndParser::AssembleCurrentPath(const bool gameRunning, const bool gameSelect) const
{
#if !defined(XENON) && !defined(PS3)
	const char* platform = "PC";
#else
#if defined(PS3)
	const char* platform = "PS3";
#else
	const char* platform = "Xbox360";
#endif
#endif

	const char* mode = "singleplayer";
	if(g_pGameCVars->g_multiplayerDefault || gEnv->bMultiplayer)
	{
		mode = "multiplayer";
	}

	static CryFixedStringT<64> path;
	if (gameSelect)
		path.Format("Libs/UI/config/navigation_%s_gameSplit.xml", platform);
	else if(gameRunning)
		path.Format("Libs/UI/config/navigation_%s_%s_ingame.xml", platform, mode);
	else
		path.Format("Libs/UI/config/navigation_%s_%s.xml", platform, mode);
	
	return path.c_str();
}



void CFrontEndParser::ReadFromFile(CMenuData& outData, const char* path) const
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile(path);
	if(!xml)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "FrontEnd: Failed to load front end setup xml file '%s'", path);
		return;
	}

	IItemParamsNode* paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xml);

	const int childXmlNodeCount = paramNode->GetChildCount();
	outData.ReserveScreens(childXmlNodeCount);
	for(int i = 0; i < childXmlNodeCount; ++i)
	{
		CMenuScreen* pScreen = ReadScreen(paramNode->GetChild(i));
		if(pScreen)
			outData.AddScreen(pScreen);
	}
}



CMenuScreen* CFrontEndParser::ReadScreen(const IItemParamsNode* child) const
{
	if(!child)
		return NULL;

	if(stricmp(child->GetName(), "Screen"))
		return NULL;

	CMenuScreen* pScreen = new CMenuScreen();
	pScreen->Initialize(child);

	return pScreen;
}


//////////////////////////////////////////////////////////////////////////