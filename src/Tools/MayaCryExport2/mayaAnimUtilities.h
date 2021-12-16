
#ifndef __MAYAANIMUTILITIES_H__
#define __MAYAANIMUTILITIES_H__

#include "mayaUtilities.h"

class cryExportOptions;

float getTimeFromFrame( int frame );
void GetAnimationRange( int &firstFrame, int &lastFrame, const cryExportOptions& exportOptions );

#endif // __MAYAANIMUTILITIES_H__