#include "StdAfx.h"
#include "stdio.h"
// Need to include this here for the MFnPlugin header
#include <vector>
#include <maya/MFnPlugin.h>

#include "plugin.h"
#include "mayaIncludes.h"

#include "MayaCryExport.h"
#include "EngineSettingsManager.h"
#include "cryEngineSettingsPlugin.h"
#include "cryMayaSupportPlugin.h"

///////////////////////////////////////////////////
// Plugin Functions
MStatus initializePlugin( MObject obj )
{
	MStatus status;

	MFnPlugin plugin(obj, "Crytek GmbH", VersionString.c_str(), "Any");

	//Register the translator with the system (NAME, image, creator func. mel script
	status = plugin.registerFileTranslator("MayaCryExport","",MayaCryExport::creator,"MayaCryExportOpts","",true);
	if (!status)
	{
		status.perror ("Error registering MayaCryExport translator");
		return status;
	}

	MFnPlugin settingsPlugin(obj, "Crytek GmbH", "1.0", "Any");
	status = settingsPlugin.registerCommand("cryEngineSettingsPlugin", cryEngineSettingsPlugin::creator);
	if (!status)
	{
		status.perror ("Error registering the cryEngineSettingsPlugin");
		return status;
	}

	MFnPlugin validatePlugin(obj, "Crytek GmbH", "1.0", "Any");
	status = validatePlugin.registerCommand("cryMayaCheckDegradedFaces", cryMayaCheckDegradedFaces::creator);
	if (!status)
	{
		status.perror ("Error registering the cryMayaCheckDegradedFaces");
		return status;
	}

	MFnPlugin supportPlugin(obj, "Crytek GmbH", "1.0", "Any");
	status = supportPlugin.registerCommand("cryMayaSupportPlugin", cryMayaSupportPlugin::creator);
	if (!status)
	{
		status.perror ("Error registering the cryMayaSupportPlugin");
		return status;
	}

	CEngineSettingsManager settingsManager;
	std::string returnVal = settingsManager.GetRootPath();
	if (returnVal.empty())
	{
		settingsManager.CallSettingsDialog(0);
	}
	
	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus status;
	MFnPlugin plugin (obj);

	status = plugin.deregisterFileTranslator ("MayaCryExport");
	if (!status)
	{
		status.perror("Error un-registering MayaCryExport translator");
		return status;
	}

	MFnPlugin settingsPlugin(obj);
	status = settingsPlugin.deregisterCommand("cryEngineSettingsPlugin");
	if (!status)
	{
		status.perror("Error un-registering the cryEngineSettingsPlugin");
		return status;
	}

	MFnPlugin validatePlugin(obj);
	status = validatePlugin.deregisterCommand("cryMayaCheckDegradedFaces");
	if (!status)
	{
		status.perror("Error un-registering the cryMayaCheckDegradedFaces");
	}

	MFnPlugin supportPlugin(obj);
	status = supportPlugin.deregisterCommand("cryMayaSupportPlugin");
	if (!status)
	{
		status.perror("Error un-registering the cryMayaSupportPlugin");
	}

	return status;
}
