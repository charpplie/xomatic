/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2007.
---------------------------------------------------------------------
File name:   CombatLog.h
$Id$
$DateTime$
Description: Logs Combat numbers to help designer balancing the game
---------------------------------------------------------------------
History:
- 12:04:2007 : Created by Ricardo Pillosu

*********************************************************************/
#ifndef _COMBAT_LOG_H_
#define _COMBAT_LOG_H_

#include "ICombatLog.h"

class CCombatLog : public ICombatLog
{

  public:

    /*$1- Singleton Stuff ----------------------------------------------------*/
    static CCombatLog&  ref();
    static bool         Create();
    void                Shutdown();

    /*$1- Basics -------------------------------------------------------------*/
    void  Init();

    /*$1- Utils --------------------------------------------------------------*/
    void Log( EntityId IdShooter, EntityId IdVictim, const char* format, ... ) PRINTF_PARAMS(4, 5);

  protected:

    /*$1- Creation and destruction via singleton -----------------------------*/
    CCombatLog();
    virtual ~CCombatLog();

  private:

    /*$1- Members ------------------------------------------------------------*/
    bool                m_bInit;
    static CCombatLog*  m_pInstance;
};
#endif // _COMBAT_LOG_H_
