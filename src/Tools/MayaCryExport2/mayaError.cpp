/******************************************************************************
** Andrew Rayson - March 2005
** Error message display for the maya plugins.
******************************************************************************/

#include "StdAfx.h"
#include <stdarg.h>
#include "mayaError.h"

bool displayYesNoQuestion( const char *message, const char *dialogName )
{
	MCommandResult cr;	
	MString s;
	char commandBuffer[1024];

	if( !dialogName )
		dialogName = "Cry Export";

	sprintf(commandBuffer,"confirmDialog -title \"%s\" -message \"%s\" -button \"Yes\" -button \"No\" -defaultButton \"Yes\" ",dialogName, message);
	s = commandBuffer;
	MString result = MGlobal::executeCommandStringResult(s,false,false, NULL);
	if( result == "No" )
		return false;
	return true;
}

void displayErrorMessage( const char *message, const char *dialogName )
{
	MCommandResult cr;	
	MString s;
	char commandBuffer[1024];

	if( !dialogName )
		dialogName = "Cry Export";

	//std::cout << "Display error message : " << message << std::endl;

	sprintf(commandBuffer,"confirmDialog -title \"%s\" -message \"%s\" -button \"OK\" -defaultButton \"OK\" ",dialogName,message);
	s = commandBuffer;
	MGlobal::executeCommand(s,cr,false,false);
}

void displayErrorMessageToScriptEditor( const char *message )
{
    MCommandResult cr;	
    MString s;
    char commandBuffer[1024];
    
    // Print to the script editor
    sprintf(commandBuffer,"warning (\"%s\\n\");",message);
    s = commandBuffer;
    MGlobal::executeCommand(s,cr,false,false);
}

bool outputErrorMessage(errorPriority pri,const char *message, ...)
{
	va_list argptr;
	char outString[1024];
	MCommandResult cr;	
	MString s;

	va_start( argptr, message );
	vsprintf(outString, message, argptr);
	va_end( argptr );	

	if(pri > PRI_LOW)
	{
		displayErrorMessageToScriptEditor( outString );
		displayErrorMessage( outString );
	}

	if(pri == PRI_CRITICAL)
	{
		Assert(0);
	}

	return true;
}

bool outputErrorMessageArray(MStringArray &messageArray)
{
	int i,numMessages,outputMessages;

	numMessages = messageArray.length();

	if(numMessages > MAX_ARRAY_MESSAGES)
		outputMessages = MAX_ARRAY_MESSAGES;
	else
		outputMessages = numMessages;

	for(i = 0;i<outputMessages;i++)
	{
		outputErrorMessage(PRI_LOW,messageArray[i].asChar());
	}
	if(outputMessages != numMessages)
	{
		for(i = outputMessages;i<numMessages;i++)
		{
			displayErrorMessageToScriptEditor(messageArray[i].asChar());
		}
		outputErrorMessage(PRI_LOW,"%d other mesages have been printed to the script editor",numMessages-outputMessages);
	}

	return true;
}

static MStringArray s_errorMessageArray;
void clearErrorMessageArray( void )
{
    s_errorMessageArray.clear();
}

void addErrorMessage( MString &errorMessage )
{
    if( errorMessage.length() > 0 )
    {
		s_errorMessageArray.append( errorMessage );
    }
}

void displayErrorMessageArray( const MString &dialogTitle, int maxDisplayErrors )
{
	int i, numErrors, displayed;

	numErrors = s_errorMessageArray.length();
	displayed = 0;

	for(i = 0;i<numErrors;i++)
	{
		displayErrorMessageToScriptEditor( s_errorMessageArray[i].asChar() );
		if( displayed < maxDisplayErrors )
			displayErrorMessage( s_errorMessageArray[i].asChar(), dialogTitle.asChar() );
		displayed++;
	}

	if( numErrors > maxDisplayErrors )
	{
		int extraErrors = numErrors - maxDisplayErrors;
		char buffer[1024];
		sprintf( buffer, "%d other error messages have not been shown. You can see them in the script editor.", extraErrors );
		displayErrorMessage( buffer );
	}
}

