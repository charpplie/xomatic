#ifndef __TRANSITIONREPORT2_H__
#define __TRANSITIONREPORT2_H__

#pragma once

#include "AnimationGraph_2.h"

void FindLongTransitions( CString& out, CAnimationGraph2Ptr pGraph, size_t minCost );

#endif
