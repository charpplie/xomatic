/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Perk console commands
*************************************************************************/

#include "StdAfx.h"
#include "Perk.h"
#include "GameCVars.h"
#include "Player.h"
#include "Game.h"
#include "GameRules.h"
#include "IPerk.h"

struct SPerkNameAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const { return ePerk_Last; };
	virtual const char* GetValue( int nIndex ) const { return CPerk::GetInstance()->GetPerkData(nIndex)->GetIDName(); };
};

struct SCategoryAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const { return ePerkSlot_Last*2; };
	virtual const char* GetValue( int nIndex ) const
	{
		switch (nIndex / ePerkSlot_Last)
		{
			case 0:
			return CPerk::s_categoryList[nIndex].m_name;

			case 1:
			return CPerk::s_categoryList[nIndex - ePerkSlot_Last].m_colourName;
		}

		assert(false);
		return NULL;
	}
};

static SPerkNameAutoComplete s_perkNameAutoComplete;
static SCategoryAutoComplete s_categoryAutoComplete;

void CPerk::CmdPerk(IConsoleCmdArgs* pCmdArgs)
{
	CPlayer * pPlayer = NULL;

	IActor * pActor = g_pGame->GetIGameFramework()->GetClientActor();
	pPlayer = (pActor && pActor->IsPlayer()) ? static_cast<CPlayer*>(pActor) : NULL;

	if (! pPlayer)
	{
		GameWarning("[perk command] Unable to find local player");
		return;
	}

	if (pCmdArgs->GetArgCount() == 1)
	{
		const int pad_size = 4;
		std::vector<EPerks> perksVec;
		std::vector< std::vector<EPerks> > perksSlotsVec(ePerkSlot_Last, perksVec);
		std::vector<int> perksSlotSize(ePerkSlot_Last, 0);
		for(int i = 0; i < ePerk_Last; i++)
		{
			EPerkCategory slot = GetInstance()->m_perkList[i].m_slot;
			perksSlotsVec[slot].push_back((EPerks)i);
			int nameLength = strlen(GetInstance()->m_perkList[i].GetIDName());
			perksSlotSize[slot] = MAX(perksSlotSize[slot], nameLength);
		}
		string strHeader("");
		for(int i = 0; i < ePerkSlot_Last; i++)
		{
			const SPerkCategoryData * slotData = & s_categoryList[i];
			int nameLength = strlen(slotData->m_name) + 2;
			perksSlotSize[i] = MAX(perksSlotSize[i], nameLength);
			int numPadding = perksSlotSize[i] - nameLength + pad_size;
			strHeader += string(slotData->m_colour) + "[" + slotData->m_name + "]" + string(' ',MAX(numPadding,0));
		}
		CryLogAlways("%s",strHeader.c_str());
		bool loop = false;
		int row = 0;
		do
		{
			loop = false;
			string strRow("");
			for(int i = 0; i < ePerkSlot_Last; i++)
			{
				const SPerkCategoryData * slotData = & s_categoryList[i];
				string element;
				int nameSize = 0;
				if (row < perksSlotsVec[i].size())
				{
					EPerks perk = perksSlotsVec[i][row];
					nameSize = strlen(GetInstance()->GetPerkData(perk)->GetIDName());
					if (pPlayer->IsPerkSet(perk))
					{
						element = slotData->m_colour;
					}
					else
					{
						element = ((GetInstance()->IsPerkAllowed(perk) & ePerkValid_AllValid) ? "$1" : "$9");
					}
					element += GetInstance()->GetPerkData(perk)->GetIDName();
					loop = true;
				}
				int numPadding = perksSlotSize[i] - nameSize + pad_size;
				element += string(' ',MAX(numPadding,0));
				strRow += element;
			}
			if (loop)
			{
				CryLogAlways("%s",strRow.c_str());
				row++;
			}
		} while(loop);
	}
	else
	{
		for(int argIndex = 1; argIndex < pCmdArgs->GetArgCount(); argIndex++)
		{
			char specialChar = '\0';
			const char* matching = pCmdArgs->GetArg(argIndex);
			
			if (matching[0] == '-' || matching[0] == '+')
			{
				specialChar = matching[0];
				++ matching;
			}

			EPerks perkNumber = GetInstance()->FindPerkNumberByName(matching);

			if (perkNumber != ePerk_Null)
			{
				if (GetInstance()->IsPerkAllowed(perkNumber) & ePerkValid_AllValid)
				{
					switch (specialChar)
					{
						case '+':
						pPlayer->SetPerkActive(perkNumber, true);
						break;

						case '-':
						pPlayer->SetPerkActive(perkNumber, false);
						break;

						default:
						assert (specialChar == '\0');
						pPlayer->SetPerkActive(perkNumber, ! pPlayer->IsPerkSet(perkNumber));
						break;
					}
				}
				else
				{
					GameWarning("[perk command] The '%s' perk is not currently available!", matching);
				}
			}
			else
			{
				GameWarning("[perk command] Unable to match \"%s\"\n", matching);
			}
		}
	}
}

void CPerk::CmdPerkTier(IConsoleCmdArgs* pCmdArgs)
{
	CPlayer * pPlayer = NULL;

	IActor * pActor = g_pGame->GetIGameFramework()->GetClientActor();
	pPlayer = (pActor && pActor->IsPlayer()) ? static_cast<CPlayer*>(pActor) : NULL;

	if (! pPlayer)
	{
		GameWarning("[perk command] Unable to find local player");
		return;
	}

	if (pCmdArgs->GetArgCount() == 3)
	{
		const char* matching = pCmdArgs->GetArg(1);
		EPerks perkNumber = GetInstance()->FindPerkNumberByName(matching);
		if (perkNumber != ePerk_Null)
		{
			IPerk* perkInstance = pPlayer->GetPerkInstance(perkNumber);
			if(perkInstance)
			{
				int tier = atoi(pCmdArgs->GetArg(2));
				perkInstance->SetTier((IPerk::EPerkTier) tier);
				CryLog("[perkTier command] Set \"%s\" to tier %d\n", matching, tier);
			}
			else
			{
				GameWarning("[perkTier command] Unable to find perk instance for \"%s\"\n", matching);
			}
		}
		else
		{
			GameWarning("[perkTier command] Unable to match \"%s\"\n", matching);
		}
		
	
	}
	else
	{
		GameWarning("Usage: perkTier [perkName] [tier]\n");
	}
}

void CPerk::CmdPerks(IConsoleCmdArgs* pCmdArgs)
{
	CPlayer * pPlayer = NULL;

	IActor * pActor = g_pGame->GetIGameFramework()->GetClientActor();
	pPlayer = (pActor && pActor->IsPlayer()) ? static_cast<CPlayer*>(pActor) : NULL;

	if (! pPlayer)
	{
		GameWarning("[perks command] Unable to find local player");
		return;
	}

	switch (pCmdArgs->GetArgCount())
	{
	case 1:
		{
			for(int i = 0; i < ePerk_Last; i++)
			{
				if(pPlayer->IsPerkSet((EPerks)i))
				{
					const SPerkCategoryData * slotData = & s_categoryList[GetInstance()->GetPerkData(i)->m_slot];
					CryLogAlways("    $1%s %s[%s] $1%s\n", GetInstance()->GetPerkData(i)->GetIDName(), slotData->m_colour, slotData->m_name, GetInstance()->GetPerkData(i)->GetDescription().c_str() );
				}
			}
		}
		break;

	case 2:
		{
			int slot = -1;
			const char* slotInput = pCmdArgs->GetArg(1);
			for(int i = 0; i < ePerkSlot_Last; i++)
			{
				const SPerkCategoryData * slotData = & s_categoryList[i];
				if(stricmp(slotInput, slotData->m_name) == 0 || stricmp(slotInput, slotData->m_colourName) == 0)
				{
					slot = i;
				}
			}
			if(slot == -1)
			{
				GameWarning("[perks command] Unable to match slot \"%s\"", slotInput);
				return;
			}
			const SPerkCategoryData * slotData = & s_categoryList[slot];
			for(int i = 0; i < ePerk_Last; i++)
			{
				if(GetInstance()->GetPerkData(i)->m_slot == slot)
				{
					CryLogAlways("    $1%s %s[%s] $1%s\n", GetInstance()->GetPerkData(i)->GetIDName(), slotData->m_colour, slotData->m_name, GetInstance()->GetPerkData(i)->GetDescription().c_str());
				}
			}
		}
		break;

	default:
		GameWarning ("[perks command] Unexpected number of arguments: %d\n", pCmdArgs->GetArgCount());
		break;
	}
}

void CPerk::CmdPerksClear(IConsoleCmdArgs* pCmdArgs)
{
	CPlayer * pPlayer = NULL;

	IActor * pActor = g_pGame->GetIGameFramework()->GetClientActor();
	pPlayer = (pActor && pActor->IsPlayer()) ? static_cast<CPlayer*>(pActor) : NULL;

	if (! pPlayer)
	{
		GameWarning("[perksclear command] Unable to find local player");
		return;
	}
	
	for(int i = 0; i < ePerk_Last; i++)
	{
		EPerks perk = (EPerks) i;
		if(pPlayer->IsPerkSet(perk))
		{
			pPlayer->SetPerkActive(perk, false);
		}
	}
}

void CPerk::CmdPerkLockUnlock(IConsoleCmdArgs* pCmdArgs, bool lockIt)
{
	bool showUsage = false;
	const char * commandName = pCmdArgs->GetArg(0);

	CPerk* perkInstance = CPerk::GetInstance();

	uint64 oldBits = ((uint32) perkInstance->GetVars()->perk_lockedPerksA) + (((uint64)(uint32)perkInstance->GetVars()->perk_lockedPerksB) << 32);

	if (pCmdArgs->GetArgCount() <= 1)
	{
		const int minimumGap = 2;
		const int eachColumn = 17;
		const int numColumns = 6;

		for (int listNum = 0; listNum < 2; ++listNum)
		{
			string message(listNum ? "Available perks:" : "Locked perks:");

			int count = numColumns;
			bool found = false;

			CryLogAlways("%s", "");

			for(int i = 0; i < ePerk_Last; ++ i)
			{
				if(((PERKBIT(i) & oldBits) ? true : false) == (listNum == 0))
				{
					int columnWidth = eachColumn;

					const string & name = perkInstance->GetPerkData(i)->GetIDName();
					int nameLength = name.length();

					while (nameLength + minimumGap > columnWidth)
					{
						columnWidth += eachColumn;
						++ count;
					}

					if (count >= numColumns)
					{
						CryLogAlways("%s", message.c_str());
						message = string("   ");
						count -= 6;
					}

					const SPerkCategoryData * pSlotData = & CPerk::s_categoryList[perkInstance->GetPerkData(i)->m_slot];
					message = (string(message) + pSlotData->m_colour + name + string(' ', columnWidth - nameLength));
					found = true;
					++ count;
				}
			}

			CryLogAlways("%s", found ? (message.c_str()) : (listNum ? "No available perks!" : "No locked perks!\n"));
		}
	}

	else if (gEnv->bMultiplayer && !gEnv->bServer)
	{
		GameWarning("The %s command is only available to the server", commandName);
	}
	else
	{
		uint64 newBits = oldBits;

		if (gEnv->bMultiplayer)
		{
			GameWarning("It's best to call %s from the front-end - some players may have picked perks already", commandName);
		}

		if (pCmdArgs->GetArgCount() == 2 && 0 == stricmp("all", pCmdArgs->GetArg(1)))
		{
			if (lockIt)
			{
				CryLogAlways ("Locking all perks!");
				newBits = PERKBIT(ePerk_Last) - 1;
			}
			else
			{
				CryLogAlways ("Unlocking all perks!");
				newBits = 0;
			}
		}
		else
		{
			for(int argIndex = 1; argIndex < pCmdArgs->GetArgCount(); argIndex++)
			{
				const char* matching = pCmdArgs->GetArg(argIndex);
				EPerks perkNumber = perkInstance->FindPerkNumberByName(matching);

				if (perkNumber != ePerk_Null)
				{
					uint64 bit = PERKBIT(perkNumber);
					bool wasLocked = (newBits & bit) ? true : false;

					if (wasLocked != lockIt)
					{
						CryLogAlways ("%s perk '%s' [bit %d ie. 0x%llx]", lockIt ? "Locking" : "Unlocking", matching, perkNumber, bit);
						if (lockIt)
						{
							newBits |= bit;
						}
						else
						{
							newBits &= ~bit;
						}
					}
					else
					{
						CryLogAlways ("The '%s' perk is already %s [bit %d ie. 0x%llx]", matching, lockIt ? "locked" : "unlocked", perkNumber, bit);
					}
				}
				else
				{
					GameWarning("The %s command was unable to find a perk called \"%s\"\n", commandName, matching);
					showUsage = true;
				}
			}
		}

		if (newBits != oldBits)
		{
			int32 newValueA = (int32) (0xFFFFFFFF & (newBits));
			int32 newValueB = (int32) (0xFFFFFFFF & (newBits >> 32));

			CryLogAlways ("Locked perks bitfield changed from 0x%llx to 0x%llx", oldBits, newBits);

			perkInstance->m_vars.perk_lockedPerksA = newValueA;
			perkInstance->m_vars.perk_lockedPerksB = newValueB;

			if (gEnv->bMultiplayer)
			{
				CHANGED_NETWORK_STATE(g_pGame->GetGameRules(), eEA_GameServerC);
			}

			showUsage = false;
		}
	}

	if (showUsage)
	{
		CryLogAlways("USAGE: %s <perk name> (<perk name>...)", commandName);
	}
}

void CPerk::CmdPerkLock(IConsoleCmdArgs* pCmdArgs)
{
	CmdPerkLockUnlock(pCmdArgs, true);
}

void CPerk::CmdPerkUnlock(IConsoleCmdArgs* pCmdArgs)
{
	CmdPerkLockUnlock(pCmdArgs, false);
}

void CPerk::RegisterPerkCommands()
{
	IConsole * console = gEnv->pConsole;

	if (console)
	{
		console->AddCommand("perkLock",   CmdPerkLock,   0, CVARHELP("Lock perk availability (server only)"));
		console->AddCommand("perkUnlock", CmdPerkUnlock, 0, CVARHELP("Unlock perk availability (server only)"));
		console->AddCommand("perk",       CmdPerk,       0, CVARHELP("Enable and disable perks for local player"));
		console->AddCommand("perkTier",   CmdPerkTier,   0, CVARHELP("Get or set Perk tier"));
		console->AddCommand("perks",      CmdPerks,      0, CVARHELP("See what perks you have and their descriptions"));
		console->AddCommand("perksClear", CmdPerksClear, 0, CVARHELP("Clear perks for local player"));

		console->RegisterAutoComplete("perk",				& s_perkNameAutoComplete);
		console->RegisterAutoComplete("perkTier",		& s_perkNameAutoComplete);
		console->RegisterAutoComplete("perkLock",		& s_perkNameAutoComplete);
		console->RegisterAutoComplete("perkUnlock",	& s_perkNameAutoComplete);
		console->RegisterAutoComplete("perks",			& s_categoryAutoComplete);
	}
}
