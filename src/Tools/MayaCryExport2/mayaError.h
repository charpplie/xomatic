/******************************************************************************
** Andrew Rayson - March 2005
** Error message display for the maya plugins.
******************************************************************************/

#ifndef __MAYAERROR_H__
#define __MAYAERROR_H__

#include "mayaAssert.h"
#include "mayaIncludes.h"

#define MAX_ARRAY_MESSAGES (100)

typedef enum errorPriority
{
    PRI_LOW = 0,
    PRI_NORM = 1,
    PRI_HIGH = 2,
    PRI_CRITICAL = 3,

    PRI_NUM
} errorPriority;

bool displayYesNoQuestion( const char *message, const char *dialogName = NULL );

void displayErrorMessage( const char *message, const char *dialogName = NULL );
void displayErrorMessageToScriptEditor( const char *message );

bool outputErrorMessage(errorPriority pri,const char *message, ...);
bool outputErrorMessageArray(MStringArray &messageArray);

void clearErrorMessageArray( void );
void addErrorMessage( MString &errorMessage );
void displayErrorMessageArray( const MString &dialogTitle, int maxDisplayErrors );

#endif // __MAYAERROR_H__
