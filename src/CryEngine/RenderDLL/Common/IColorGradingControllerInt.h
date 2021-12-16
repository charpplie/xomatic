#include DEVIRTUALIZE_HEADER_FIX(IColorGradingController.h)

#ifndef I_COLOR_GRADING_CONTROLLER_INT_H
#define I_COLOR_GRADING_CONTROLLER_INT_H

#pragma once


#include <IColorGradingController.h>


UNIQUE_IFACE struct IColorGradingControllerInt : public IColorGradingController
{
	virtual void RT_SetLayers(const SColorChartLayer* pLayers, uint32 numLayers) = 0;
};


#endif // #ifndef I_COLOR_GRADING_CONTROLLER_INT_H
