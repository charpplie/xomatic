/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 5:11:2009: Created by Filipe Amim

*************************************************************************/
#pragma once

#ifndef COVER_AND_LEAN_DRAW_H
#define COVER_AND_LEAN_DRAW_H

class CPlayer;


void DebugDrawEdgeReference(CPlayer* pPlayer, const CCoverAndLean::SEdgeReference& edgeReference, bool canLean, const Vec3& viewPosition, const Vec3& viewFrontVec, const Vec3& viewLeanVec, float leanAmount);
void DebugDrawCurrentBox(const CCoverAndLean::SBox& currentBox);


#endif
