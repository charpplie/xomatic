#ifndef __GAMEACTIONS_H__
#define __GAMEACTIONS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IActionMapManager.h>

#define DECL_ACTION(name) ActionId name;
class CGameActions
{
public:
	CGameActions();
#include "GameActions.actions"

	void Init();
	ILINE IActionFilter*	FilterNoMove() const {	return m_pFilterNoMove;	}
	ILINE IActionFilter*	FilterNoMouse() const {	return m_pFilterNoMouse;	}
	ILINE IActionFilter*	FilterInVehicleSuitMenu() const {	return m_pFilterInVehicleSuitMenu;	}
	ILINE IActionFilter*	FilterSuitMenu() const {	return m_pFilterSuitMenu;	}
	ILINE IActionFilter*	FilterFreezeTime() const {	return m_pFilterFreezeTime;	}
	ILINE IActionFilter*	FilterNoVehicleExit() const {	return m_pFilterNoVehicleExit;	}
	ILINE IActionFilter*	FilterMPRadio() const {	return m_pFilterMPRadio;	}
	ILINE IActionFilter*	FilterCutscene() const {	return m_pFilterCutscene;	}
	ILINE IActionFilter*	FilterCutsceneNoPlayer() const {	return m_pFilterCutsceneNoPlayer;	}
	ILINE IActionFilter*	FilterNoMPSelectMenuOpen() const {	return m_pFilterNoMPSelectMenuOpen;	}
	ILINE IActionFilter*	FilterNoMapOpen() const {	return m_pFilterNoMapOpen;	}
	ILINE IActionFilter*	FilterNoObjectivesOpen() const {	return m_pFilterNoObjectivesOpen;	}
	ILINE IActionFilter*	FilterVehicleNoSeatChangeAndExit() const {	return m_pFilterVehicleNoSeatChangeAndExit;	}
	ILINE IActionFilter*	FilterNoConnectivity() const {	return m_pFilterNoConnectivity;	}
	ILINE IActionFilter*	FilterSuitActive() const { return m_pFilterSuitActive; }
	ILINE IActionFilter*	FilterWeaponModScreen() const { return m_pFilterWeaponModScreen; }
	ILINE IActionFilter*	FilterTweakMenu() const { return m_pFilterTweakMenu; }
	ILINE IActionFilter*	FilterWeaponMenu() const { return m_pFilterWeaponMenu; }
	ILINE IActionFilter*	FilterIngameMenu() const { return m_pFilterIngameMenu; }
	ILINE IActionFilter*	FilterStrikePointer() const { return m_pFilterStrikePointer; }
	ILINE IActionFilter*	FilterUseKeyOnly() const { return m_pFilterUseKeyOnly; }


private:
	void	CreateFilterNoMove();
	void	CreateFilterNoMouse();
	void	CreateFilterInVehicleSuitMenu();
	void	CreateFilterSuitMenu();
	void	CreateFilterFreezeTime();
	void	CreateFilterNoVehicleExit();
	void	CreateFilterMPRadio();
	void	CreateFilterCutscene();
	void	CreateFilterCutsceneNoPlayer();
	void	CreateFilterNoMPSelectMenuOpen();
	void	CreateFilterNoMapOpen();
	void	CreateFilterNoObjectivesOpen();
	void	CreateFilterVehicleNoSeatChangeAndExit();
	void	CreateFilterNoConnectivity();
	void	CreateFilterSuitActive();
	void  CreateFilterWeaponModScreen();
	void	CreateFilterTweakMenu();
	void  CreateFilterWeaponMenu();
	void CreateFilterIngameMenu();
	void CreateFilterStrikePointer();
	void CreateFilterUseKeyOnly();

	IActionFilter*	m_pFilterNoMove;
	IActionFilter*	m_pFilterNoMouse;
	IActionFilter*	m_pFilterInVehicleSuitMenu;
	IActionFilter*	m_pFilterSuitMenu;
	IActionFilter*	m_pFilterFreezeTime;
	IActionFilter*	m_pFilterNoVehicleExit;
	IActionFilter*	m_pFilterMPRadio;
	IActionFilter*	m_pFilterCutscene;
	IActionFilter*	m_pFilterCutsceneNoPlayer;
	IActionFilter*	m_pFilterNoMPSelectMenuOpen;
	IActionFilter*	m_pFilterNoMapOpen;
	IActionFilter*	m_pFilterNoObjectivesOpen;
	IActionFilter*	m_pFilterVehicleNoSeatChangeAndExit;
	IActionFilter*	m_pFilterNoConnectivity;
	IActionFilter*	m_pFilterSuitActive;
	IActionFilter*  m_pFilterWeaponModScreen;
	IActionFilter*	m_pFilterTweakMenu;
	IActionFilter*  m_pFilterWeaponMenu;
	IActionFilter*  m_pFilterIngameMenu;
	IActionFilter*	m_pFilterStrikePointer;
	IActionFilter*	m_pFilterUseKeyOnly;
};
#undef DECL_ACTION

extern CGameActions* g_pGameActions;

#endif //__GAMEACTIONS_H__
