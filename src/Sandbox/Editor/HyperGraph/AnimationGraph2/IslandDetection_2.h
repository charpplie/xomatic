// routines for detecting islands of states

#ifndef __ISLANDDETECTION2_H__
#define __ISLANDDETECTION2_H__

#pragma once

#include <set>
#include "AnimationGraph_2.h"

typedef std::vector<CAGState2Ptr> TAGIsland;
typedef std::list<TAGIsland> TAGIslands;

void GenerateIslands( TAGIslands& islands, CAnimationGraph2Ptr pGraph );
CString GenerateIslandReport( const TAGIslands& islands );
CString GenerateIslandReport( CAnimationGraph2Ptr pGraph );

#endif
