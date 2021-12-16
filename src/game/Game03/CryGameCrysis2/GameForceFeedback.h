/********************************************************************
  CryGame Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  File name:   GameForceFeedback.h
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 26:6:2009					: Created by Tim Furnish

*********************************************************************/

#ifndef __GameForceFeedback_H__
#define __GameForceFeedback_H__

void	GameForceFeedback_ForEntity			(EntityId entityID, const SFFOutputEvent &event);
void	GameForceFeedback_Always				(const SFFOutputEvent &event);

#ifndef CRY_UNIT_NO_TESTING
void	GameForceFeedback_ResetCount		();
int		GameForceFeedback_GetCount			();
#endif

#endif
