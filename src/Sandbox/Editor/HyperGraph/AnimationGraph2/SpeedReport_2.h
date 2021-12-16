#ifndef __SPEEDREPORT2_H__
#define __SPEEDREPORT2_H__

#pragma once

#include "ICryAnimation.h"
#include "AnimationGraph_2.h"

CString GenerateSpeedReport2( IAnimationSet * pAnimSet );
CString MatchMovementSpeedsToAnimations2( IAnimationSet * pAnimSet, CAnimationGraph2 * pGraph );
CString GenerateBadCALReport2( IAnimationSet * pAnimSet, CAnimationGraph2 * pGraph );
CString GenerateNullNodesWithNoForceLeaveReport2( CAnimationGraph2 * pGraph );
CString GenerateOrphanNodesReport2( CAnimationGraph2 * pGraph );
CString GenerateDeadInputsReport2( CAnimationGraph2 * pGraph );

#endif
