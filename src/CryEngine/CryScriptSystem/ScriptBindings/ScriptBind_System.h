////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   ScriptBind_System.h
//  Version:     v1.00
//  Created:     8/7/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScriptBind_System_h__
#define __ScriptBind_System_h__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <IScriptSystem.h>

struct ISystem;
struct ILog;
struct IRenderer;
struct IConsole;
struct IInput;
struct ITimer;
struct IEntitySystem;
struct I3DEngine;
struct IPhysicalWorld;
struct ICVar;

/*! 
<title System>
Syntax: System

This class implements script-functions for exposing the System functionalities

REMARKS:
this object doesn't have a global mapping(is not present as global variable into the script state)

IMPLEMENTATIONS NOTES:
These function will never be called from C-Code. They're script-exclusive.
*/

class CScriptBind_System : public CScriptableBase
{ 
public:
	CScriptBind_System(IScriptSystem *pScriptSystem,ISystem *pSystem);
	virtual ~CScriptBind_System();
	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

public:
	// <title CreateDownload>
	// Syntax: System.CreateDownload()
	int CreateDownload(IFunctionHandler *pH);
	// <title LoadFont>
	// Syntax: System.LoadFont(const char *pszName)
	// Arguments:
	//		pszName - Font name.
	// Description:
	//		Loads a font.
	int LoadFont(IFunctionHandler *pH); 
	// <title ExecuteCommand>
	// Syntax: System.ExecuteCommand(const char *szCmd)
	// Arguments:
	//		szCmd - Command string.
	// Description:
	//		Executes a command.
	int ExecuteCommand(IFunctionHandler *pH);
	// <title LogToConsole>
	// Syntax: System.LogToConsole(const char *sText)
	// Arguments:
	//		sText - Text to be logged.
	// Description:
	//		Logs a message to the console.
	int LogToConsole(IFunctionHandler *pH);
	// <title ClearConsole>
	// Syntax: System.ClearConsole()
	// Description:
	//		Clears the console.
	int ClearConsole(IFunctionHandler *pH);
	// <title Log>
	// Syntax: System.Log(const char *sText)
	// Arguments:
	//		sText - Text to be logged.
	// Description:
	//		Logs a message.
	int Log(IFunctionHandler *pH);
	// <title LogAlways>
	// Syntax: System.LogAlways(const char *sText)
	// Arguments:
	//		sText - Text to be logged.
	// Description:
	//		Logs important data that must be printed regardless verbosity.
	int LogAlways(IFunctionHandler *pH);
	// <title Warning>
	// Syntax: System.Warning(const char *sText)
	// Arguments:
	//		sText - Text to be logged.
	// Description:
	//		Shows a message text with the warning severity.
	int Warning(IFunctionHandler *pH);
	// <title Error>
	// Syntax: System.Error(const char *sText)
	// Arguments:
	//		sText - Text to be logged.
	// Description:
	//		Shows a message text with the error severity.
	int Error(IFunctionHandler *pH);
	// <title IsEditor>
	// Syntax: System.IsEditor()
	// Description:
	//		Checks if the system is the editor.
	int IsEditor(IFunctionHandler *pH);
	// <title GetCurrTime>
	// Syntax: System.GetCurrTime()
	// Description:
	//		Gets the current time.
	int GetCurrTime(IFunctionHandler *pH);
	// <title GetCurrAsyncTime>
	// Syntax: System.GetCurrAsyncTime()
	// Description:
	//		Gets the current asynchronous time.
	int GetCurrAsyncTime(IFunctionHandler *pH);
	// <title GetFrameTime>
	// Syntax: System.GetFrameTime()
	// Description:
	//		Gets the frame time.
	int GetFrameTime(IFunctionHandler *pH);
	// <title GetLocalOSTime>
	// Syntax: System.GetLocalOSTime()
	// Description:
	//		Gets the local operating system time.
	int GetLocalOSTime(IFunctionHandler *pH);
	// <title GetUserName>
	// Syntax: System.GetUserName()
	// Description:
	//		Gets the username on this machine.
	int GetUserName(IFunctionHandler *pH);
	// <title ShowConsole>
	// Syntax: System.ShowConsole(int nParam)
	// Arguments:
	//		nParam - 1 to show the console, 0 to hide.
	// Description:
	//		Shows/hides the console.
	int ShowConsole(IFunctionHandler *pH);

	// <title CheckHeapValid>
	// Syntax: System.CheckHeapValid(const char *name)
	// Arguments:
	//		name - Name string.
	// Description:
	//		Checks the heap validity.
	int CheckHeapValid(IFunctionHandler *pH);

	// <title GetConfigSpec>
	// Syntax: System.GetConfigSpec()
	// Description:
	//		Gets the config specification.
	int GetConfigSpec(IFunctionHandler *pH);
	// <title IsMultiplayer>
	// Syntax: System.IsMultiplayer()
	// Description:
	//		Checks if the game is multiplayer.
	int IsMultiplayer(IFunctionHandler *pH);

	// <title GetEntity>
	// Syntax: System.GetEntity(unsigned int entityId)
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Gets an entity from its ID.
	int GetEntity(IFunctionHandler *pH);
	// <title GetEntityClass>
	// Syntax: System.GetEntityClass(unsigned int entityId)
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Gets an entity class from its ID.
	int GetEntityClass(IFunctionHandler *pH);

	// <title GetEntities>
	// Syntax: System.GetEntities(Vec3 center, float radius)
	// Arguments:
	//		center - Center position vector for the area where to get entities.
	//		radius - Radius of the area.
	// Description:
	//		Gets all the entities contained in the specific area of the level.
	int GetEntities(IFunctionHandler *pH);
	// <title GetEntitiesByClass>
	// Syntax: System.GetEntitiesByClass(const char* EntityClass)
	// Arguments:
	//		EntityClass - Entity class name.
	// Description:
	//		Gets all the entities of the specified class.
	int GetEntitiesByClass(IFunctionHandler *pH, const char* EntityClass);
	// <title GetEntitiesInSphere>
	// Syntax: System.GetEntitiesInSphere( Vec3 center, float radius )
	// Arguments:
	//		center - Center position vector for the sphere where to look at entities.
	//		radius - Radius of the sphere.
	// Description:
	//		Gets all the entities contained into the specified sphere.
	int GetEntitiesInSphere(IFunctionHandler *pH, Vec3 center, float radius);
	// <title GetEntitiesInSphereByClass>
	// Syntax: System.GetEntitiesInSphereByClass( Vec3 center, float radius, const char* EntityClas )
	// Arguments:
	//		center - Center position vector for the sphere where to look at entities.
	//		radius - Radius of the sphere.
	//		EntityClass - Entity class name.
	// Description:
	//		Gets all the entities contained into the specified sphere for the specific class name.
	int GetEntitiesInSphereByClass(IFunctionHandler *pH, Vec3 center, float radius, const char* EntityClass);
	// <title GetPhysicalEntitiesInBox>
	// Syntax: System.GetPhysicalEntitiesInBox( Vec3 center, float radius )
	// Arguments:
	//		center - Center position vector for the area where to look at entities.
	//		radius - Radius of the sphere.
	// Description:
	//		Gets all the entities contained into the specified area.
	int GetPhysicalEntitiesInBox(IFunctionHandler *pH, Vec3 center, float radius);
	// <title GetPhysicalEntitiesInBoxByClass>
	// Syntax: System.GetPhysicalEntitiesInBoxByClass( Vec3 center, float radius, const char *className )
	// Arguments:
	//		center - Center position vector for the area where to look at entities.
	//		radius - Radius of the sphere.
	//		className - Entity class name.
	// Description:
	//		Gets all the entities contained into the specified area for the specific class name.
	int GetPhysicalEntitiesInBoxByClass(IFunctionHandler *pH, Vec3 center, float radius, const char *className);
	// <title GetNearestEntityByClass>
	// Syntax: System.GetNearestEntityByClass( Vec3 center, float radius, const char *className )
	// Arguments:
	//		center - Center position vector for the area where to look at entities.
	//		radius - Radius of the sphere.
	//		className - Entity class name.
	// Description:
	//		Gets the nearest entity with the specified class.
	int GetNearestEntityByClass(IFunctionHandler *pH, Vec3 center, float radius, const char *className);

	// <title GetEntityByName>
	// Syntax: System.GetEntityByName( const char *sEntityName )
	// Description:
	//    Retrieve entity table for the first entity with specified name.
	//    If multiple entities with same name exist, first one found is returned.
	// Arguments:
	//    sEntityName - Name of the entity to search.
	int GetEntityByName(IFunctionHandler *pH,const char *sEntityName);

	// <title GetEntityIdByName>
	// Syntax: System.GetEntityIdByName( const char *sEntityName )
	// Description:
	//    Retrieve entity Id for the first entity with specified name.
	//    If multiple entities with same name exist, first one found is returned.
	// Arguments:
	//    sEntityName - Name of the entity to search.
	int GetEntityIdByName(IFunctionHandler *pH,const char *sEntityName);

	// <title DrawLabel>
	// Syntax: System.DrawLabel( Vec3 vPos, float fSize, const char *text, [optional] float r, [optional] float g, [optional] float b, [optional] float alpha  )
	// Arguments:
	//		vPos	- Position vector.
	//		fSize	- Size for the label.
	//		text	- Text of the label.
	//		r		- Red component for the label color. Default is 1.
	//		g		- Green component for the label color. Default is 1.
	//		b		- Blue component for the label color. Default is 1.
	//		alpha	- Alpha component for the label color. Default is 1.
	// Description:
	//		Draws a label with the specified parameter.
	int DrawLabel(IFunctionHandler *pH);
	// <title DeformTerrain>
	// Syntax: System.DeformTerrain()
	// Description:
	//		Deforms the terrain.
	int DeformTerrain(IFunctionHandler *pH);
	// <title DeformTerrainUsingMat>
	// Syntax: System.DeformTerrainUsingMat()
	// Description:
	//		Deforms the terrain using material.
	int DeformTerrainUsingMat(IFunctionHandler *pH);
	// <title ApplyForceToEnvironment>
	// Syntax: System.ApplyForceToEnvironment(Vec3 pos, float force, float radius)
	// Arguments:
	//		pos		- Position of the force.
	//		force	- Strength of the force.
	//		radius	- Area where the force has effects.
	// Description:
	//		Applies a force to the environment.
	int ApplyForceToEnvironment(IFunctionHandler *pH);

	// <title ScreenToTexture>
	// Syntax: System.ScreenToTexture()
	int ScreenToTexture(IFunctionHandler *pH);
	// <title DrawTriStrip>
	// Syntax: System.DrawTriStrip(ScriptHandle handle, int nMode, SmartScriptTable vtxs, float r, float g, float b, float alpha )
	// Arguments:
	//		handle	- .
	//		nMode	- .
	//		vtx		- .
	//		r		- Red component for the label color. Default is 1.
	//		g		- Green component for the label color. Default is 1.
	//		b		- Blue component for the label color. Default is 1.
	//		alpha	- Alpha component for the label color. Default is 1.
	// Description:
	//		Draws a triangle strip.
	int DrawTriStrip(IFunctionHandler *pH);
	// <title DrawLine>
	// Syntax: System.DrawLine(Vec3 p1, Vec3 p2, float r, float g, float b, float alpha )
	// Arguments:
	//		p1		- Start position of the line.
	//		p2		- End position of the line.
	//		r		- Red component for the label color. Default is 1.
	//		g		- Green component for the label color. Default is 1.
	//		b		- Blue component for the label color. Default is 1.
	//		alpha	- Alpha component for the label color. Default is 1.
	// Description:
	//		Draws a line.
	int DrawLine(IFunctionHandler *pH);
	// <title Draw2DLine>
	// Syntax: System.Draw2DLine(float p1x, float p1y, float p2x, float p2y,  float r, float g, float b, float alpha )
	// Arguments:
	//		p1x		- X value of the start point of the line.
	//		p1y		- Y value of the start point of the line.
	//		p2x		- X value of the end point of the line.
	//		p2y		- Y value of the end point of the line.
	//		r		- Red component for the label color. Default is 1.
	//		g		- Green component for the label color. Default is 1.
	//		b		- Blue component for the label color. Default is 1.
	//		alpha	- Alpha component for the label color. Default is 1.
	// Description:
	//		Draws a 2D line.
	int Draw2DLine(IFunctionHandler *pH);
	// <title DrawText>
	// Syntax: System.DrawText(float x, float y,const char* text,const char* font, float size, float p2y,  float r, float g, float b, float alpha )
	//		x		- X position for the text.
	//		y		- Y position for the text.
	//		text	- Text to be displayed.
	//		font	- Font name.
	//		size	- Text size.
	//		r		- Red component for the label color. Default is 1.
	//		g		- Green component for the label color. Default is 1.
	//		b		- Blue component for the label color. Default is 1.
	//		alpha	- Alpha component for the label color. Default is 1.
	// Description:
	//		Draws a text.
	int DrawText(IFunctionHandler *pH);
	// <title SetGammaDelta>
	// Syntax: System.SetGammaDelta( float fDelta )
	// Arguments:
	//		fDelta - Delta value.
	// Description:
	//		Sets the gamma/delta value.
	int SetGammaDelta(IFunctionHandler *pH);
	// <title DrawRectShader>
	// Syntax: System.DrawRectShader( const char *pszShaderName, float x, float y, float w, float h, float r, float g, float b, float a )
	// Arguments:
	//		pszShaderName - Shader name.
	//		x			- Rectangular x position.
	//		y			- Rectangular y position.
	//		w			- Rectangular width.
	//		h			- Rectangular height.
	// Description:
	//		Use the shader to draw a rectangular.
	int DrawRectShader(IFunctionHandler *pH);
	// <title DrawRectShader>
	// Syntax: System.DrawRectShader( const char *pszShaderName)
	// Arguments:
	//		pszShaderName - Shader name.
	// Description:
	//		NOT SUPPORTED ANYMORE.
	int SetScreenShader(IFunctionHandler *pH);

	// <title SetPostProcessFxParam>
	// Syntax: System.SetPostProcessFxParam( const char * pszEffectParam, value )
	// Arguments:
	//		pszEffectParam	- Parameter for the post processing effect.
	//		value			- Value for the parameter.
	// Description:
	//		Sets a post processing effect parameter value.
	int SetPostProcessFxParam(IFunctionHandler *pH); 
	// <title GetPostProcessFxParam>
	// Syntax: System.GetPostProcessFxParam( const char * pszEffectParam, value )
	// Arguments:
	//		pszEffectParam	- Parameter for the post processing effect.
	//		value			- Value for the parameter.
	// Description:
	//		Gets a post processing effect parameter value.
	int GetPostProcessFxParam(IFunctionHandler *pH);  

	// <title SetScreenFx>
	// Syntax: System.SetScreenFx( const char * pszEffectParam, value )
	// Arguments:
	//		pszEffectParam	- Parameter for the post processing effect.
	//		value			- Value for the parameter.
	// Description:
	//		Sets a post processing effect parameter value.
	int SetScreenFx(IFunctionHandler *pH);
	// <title GetScreenFx>
	// Syntax: System.GetScreenFx( const char * pszEffectParam, value )
	// Arguments:
	//		pszEffectParam	- Parameter for the post processing effect.
	//		value			- Value for the parameter.
	// Description:
	//		Gets a post processing effect parameter value.
	int GetScreenFx(IFunctionHandler *pH);//const char*, const char*    

	// <title SetCVar>
	// Syntax: System.SetCVar( const char *sCVarName, value )
	// Arguments:
	//		sCVarName	- Name of the variable.
	//		value		- Value of the variable.
	// Description:
	//		Sets the value of a CVariable.
	int SetCVar(IFunctionHandler *pH);
	// <title GetCVar>
	// Syntax: System.GetCVar( const char *sCVarName)
	// Arguments:
	//		sCVarName	- Name of the variable.
	// Description:
	//		Gets the value of a CVariable.
	int GetCVar(IFunctionHandler *pH);
	// <title AddCCommand>
	// Syntax: System.AddCCommand( const char *sCCommandName, const char *sCommand, const char *sHelp)
	// Arguments:
	//		sCCommandName	- C command name.
	//		sCommand		- Command string.
	//		sHelp			- Help for the command usage.
	// Description:
	//		Adds a C command to the system.
	int AddCCommand(IFunctionHandler *pH);

	// <title SetScissor>
	// Syntax: System.SetScissor( float x, float y, float w, float h )
	// Arguments:
	//		x - X position.
	//		y -	Y position.
	//		w - Width size.
	//		h - Height size.
	// Description:
	//		Sets scissor info.
	int SetScissor(IFunctionHandler *pH);


	// CW: added for script based system analysis

	// <title GetCPUQuality>
	// Syntax: System.GetCPUQuality()
	// Description:
	//		Gets the CPU quality.
	int GetCPUQuality( IFunctionHandler *pH );
	// <title GetGPUQuality>
	// Syntax: System.GetGPUQuality()
	// Description:
	//		Gets the GPU quality.
	int GetGPUQuality( IFunctionHandler *pH );
	// <title GetSystemMem>
	// Syntax: System.GetSystemMem()
	// Description:
	//		Gets the amount of the memory for the system.
	int GetSystemMem( IFunctionHandler *pH );
	// <title GetVideoMem>
	// Syntax: System.GetVideoMem()
	// Description:
	//		Gets the amount of the video memory for the system.
	int GetVideoMem( IFunctionHandler *pH );
	// <title IsPS20Supported>
	// Syntax: System.IsPS20Supported()
	// Description:
	//		Checks if the PS20 is supported.
	int IsPS20Supported( IFunctionHandler *pH );
	// <title IsHDRSupported>
	// Syntax: System.IsHDRSupported()
	// Description:
	//		Checks if the HDR is supported.
	int IsHDRSupported( IFunctionHandler *pH );

	// <title SetBudget>
	// Syntax: System.SetBudget(int sysMemLimitInMB, int videoMemLimitInMB, float frameTimeLimitInMS, int soundChannelsPlayingLimit, int soundMemLimitInMB, int numDrawCallsLimit )
	// Arguments:
	//		sysMemLimitInMB		- Limit of the system memory in MB.
	//		videoMemLimitInMB	- Limit of the video memory in MB.
	//		frameTimeLimitInMS	- Limit in the frame time in MS.
	//		soundChannelsPlayingLimit	- Limit of the sound channels playing.
	//		soundMemLimitInMB	- Limit of the sound memory in MB.
	//		numDrawCallsLimit	- Limit of the draw calls.
	// Description:
	//		Sets system budget.
	int SetBudget( IFunctionHandler *pH );
	// <title SetVolumetricFogModifiers>
	// Syntax: System.SetVolumetricFogModifiers( float gobalDensityModifier, float atmosphereHeightModifier )
	// Arguments:
	//		gobalDensityModifier		- Modifier for the global density.
	//		atmosphereHeightModifier	- Modifier for the atmosphere height.
	// Description:
	//		Sets the volumetric fog modifiers.
	int SetVolumetricFogModifiers( IFunctionHandler *pH );

	// <title SetWind>
	// Syntax: System.SetWind( Vec3 vWind )
	// Arguments:
	//		vWind - Wind direction.
	// Description:
	//		Sets the wind direction.
	int SetWind( IFunctionHandler *pH );
	// <title SetWind>
	// Syntax: System.SetWind()
	// Description:
	//		Gets the wind direction.
	int GetWind( IFunctionHandler *pH );

	// <title GetSurfaceTypeIdByName>
	// Syntax: System.GetSurfaceTypeIdByName( const char* surfaceName )
	// Arguments:
	//		surfaceName - Surface name.
	// Description:
	//		Gets the surface type identifier by its name.
	int GetSurfaceTypeIdByName( IFunctionHandler *pH, const char* surfaceName);
	// <title GetSurfaceTypeNameById>
	// Syntax: System.GetSurfaceTypeNameById( int surfaceId )
	// Arguments:
	//		surfaceId - Surface identifier.
	// Description:
	//		Gets the surface type name by its identifier.
	int GetSurfaceTypeNameById( IFunctionHandler *pH, int surfaceId );

	// <title RemoveEntity>
	// Syntax: System.RemoveEntity( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Removes the specified entity.
	int RemoveEntity(IFunctionHandler *pH, ScriptHandle entityId);
	// <title SpawnEntity>
	// Syntax: System.SpawnEntity( SmartScriptTable params )
	// Arguments:
	//		params - Entity parameters.
	// Description:
	//		Spawns an entity.
	int SpawnEntity(IFunctionHandler *pH, SmartScriptTable params);
	// <title ActivateLight>
	// Syntax: System.ActivateLight()
	// Description:
	//		NOT SUPPORTED ANYMORE.
	int ActivateLight(IFunctionHandler *pH); //name, activate
	//	int ActivateMainLight(IFunctionHandler *pH); //pos, activate
	//	int SetSkyBox(IFunctionHandler *pH); //szShaderName, fBlendTime, bUseWorldBrAndColor

	// <title SetWaterVolumeOffset>
	// Syntax: System.SetWaterVolumeOffset()
	// Description:
	//		SetWaterLevel is not supported by 3dengine for now.
	int SetWaterVolumeOffset(IFunctionHandler *pH);
	// <title IsValidMapPos>
	// Syntax: System.IsValidMapPos( Vec3 v )
	// Arguments:
	//		v	- Position vector.
	// Description:
	//		Checks if the position is a valid map position.
	int IsValidMapPos(IFunctionHandler *pH);
	// <title EnableMainView>
	// Syntax: System.EnableMainView()
	// Description:
	//		Feature unimplemented.
	int EnableMainView(IFunctionHandler *pH);
	// <title EnableOceanRendering>
	// Syntax: System.EnableOceanRendering()
	// Arguments:
	//		bOcean - True to activate the ocean rendering, false to deactivate it.
	// Description:
	//		Enables/disables ocean rendering.
	int EnableOceanRendering(IFunctionHandler *pH);
	// <title ScanDirectory>
	// Syntax: System.ScanDirectory( const char *pszFolderName, int nScanMode )
	// Arguments:
	//		pszFolderName	- Folder name.
	//		nScanMode		- Scan mode for the folder. Can be:
	//								SCANDIR_ALL
	//								SCANDIR_FILES
	//								SCANDIR_SUBDIRS
	// Description:
	//		Scans a directory.
	int	ScanDirectory(IFunctionHandler *pH);
	// <title DebugStats>
	// Syntax: System.DebugStats( bool cp )
	int DebugStats(IFunctionHandler *pH);
	// <title ViewDistanceSet>
	// Syntax: System.ViewDistanceSet( float fViewDist )
	// Arguments:
	//		fViewDist - View distance.
	// Description:
	//		Sets the view distance.
	int ViewDistanceSet(IFunctionHandler *pH);
	// <title ViewDistanceSet>
	// Syntax: System.ViewDistanceSet()
	// Description:
	//		Gets the view distance.
	int ViewDistanceGet(IFunctionHandler *pH);
	// <title GetOutdoorAmbientColor>
	// Syntax: System.GetOutdoorAmbientColor()
	// Description:
	//		Gets the outdoor ambient color.
	int GetOutdoorAmbientColor(IFunctionHandler *pH);
	// <title GetOutdoorAmbientColor>
	// Syntax: System.GetOutdoorAmbientColor( Vec3 v3Color )
	//		v3Color - Outdoor ambient color value.
	// Description:
	//		Sets the outdoor ambient color.
	int SetOutdoorAmbientColor(IFunctionHandler *pH);
	// <title GetTerrainElevation>
	// Syntax: System.GetTerrainElevation( Vec3 v3Pos )
	// Arguments:
	//		v3Pos - Position of the terraint to be checked.
	// Description:
	//		Gets the terrain elevation of the specified position.
	int GetTerrainElevation(IFunctionHandler *pH);
	//	int SetIndoorColor(IFunctionHandler *pH);
	// <title ActivatePortal>
	// Syntax: System.ActivatePortal( Vec3 vPos, bool bActivate, ScriptHandle nID )
	// Arguments:
	//		vPos		- Position vector.
	//		bActivate	- True to activate the portal, false to deactivate.
	//		nID			- Entity identifier.
	// Description:
	//		Activates/deactivates a portal.
	int	ActivatePortal(IFunctionHandler *pH);
	// <title DumpMMStats>
	// Syntax: System.DumpMMStats()
	// Description:
	//		Dumps the MM statistics.
	int DumpMMStats(IFunctionHandler *pH);
	// <title EnumAAFormats>
	// Syntax: System.EnumAAFormats( int m_Width, int m_Height, int m_BPP )
	// Description:
	//		Enumerates the AA formats.
	int EnumAAFormats(IFunctionHandler *pH);
	// <title EnumDisplayFormats>
	// Syntax: System.EnumDisplayFormats()
	// Description:
	//		Enumerates display formats.
	int EnumDisplayFormats(IFunctionHandler *pH);
	// <title IsPointIndoors>
	// Syntax: System.IsPointIndoors( Vec3 vPos )
	// Arguments:
	//		vPos - Position vector.
	// Description:
	//		Checks if a point is indoors.
	int IsPointIndoors(IFunctionHandler *pH);
	// <title SetConsoleImage>
	// Syntax: System.SetConsoleImage( const char *pszName, bool bRemoveCurrent )
	// Arguments:
	//		pszName - Texture image.
	//		bRemoveCurrent - True to remove the current image, false otherwise.
	// Description:
	//		Sets the console image.
	int SetConsoleImage(IFunctionHandler *pH);
	// <title ProjectToScreen>
	// Syntax: System.ProjectToScreen( Vec3 vec )
	// Arguments:
	//		vec - Position vector.
	// Description:
	//		Projects to the screen.
	int ProjectToScreen(IFunctionHandler *pH, Vec3 vec);
	// <title EnableHeatVision>
	// Syntax: System.EnableHeatVision()
	// Description:
	//		Is not supported anymore.
	int EnableHeatVision(IFunctionHandler *pH);
	// <title ShowDebugger>
	// Syntax: System.ShowDebugger()
	// Description:
	//		Shows the debugger.
	int ShowDebugger(IFunctionHandler *pH);
	// <title DumpMemStats>
	// Syntax: System.DumpMemStats( bool bUseKB )
	// Arguments:
	//		bUseKB - True to use KB, false otherwise.
	// Description:
	//		Dumps memory statistics.
	int DumpMemStats (IFunctionHandler *pH);
	// <title DumpMemoryCoverage>
	// Syntax: System.DumpMemoryCoverage( bool bUseKB )
	// Description:
	//		Dumps memory coverage.
	int DumpMemoryCoverage(IFunctionHandler *pH);
	// <title ApplicationTest>
	// Syntax: System.ApplicationTest( const char *pszParam )
	// Arguments:
	//		pszParam - Parameters.
	// Description:
	//		Test the application with the specified parameters.
	int ApplicationTest (IFunctionHandler *pH);
	// <title QuitInNSeconds>
	// Syntax: System.QuitInNSeconds( float fInNSeconds )
	// Arguments:
	//		fInNSeconds - Number of seconds before quitting.
	// Description:
	//		Quits the application in the specified number of seconds.
	int QuitInNSeconds(IFunctionHandler *pH);
	// <title DumpWinHeaps>
	// Syntax: System.DumpWinHeaps()
	// Description:
	//		Dumps windows heaps.
	int DumpWinHeaps (IFunctionHandler *pH);
	// <title Break>
	// Syntax: System.Break()
	// Description:
	//		Breaks the application with a fatal error message.
	int Break(IFunctionHandler *pH);
	// <title SetViewCameraFov>
	// Syntax: System.SetViewCameraFov( float fov )
	// Description:
	//		Sets the view camera fov.
	int SetViewCameraFov(IFunctionHandler *pH, float fov);
	// <title GetViewCameraFov>
	// Syntax: System.GetViewCameraFov()
	// Description:
	//		Gets the view camera fov.
	int GetViewCameraFov(IFunctionHandler *pH);
	// <title IsPointVisible>
	// Syntax: System.IsPointVisible( Vec3 point )
	// Arguments:
	//		point - Point vector.
	// Description:
	//		Checks if the specified point is visible.
	int IsPointVisible(IFunctionHandler *pH, Vec3 point);
	// <title GetViewCameraPos>
	// Syntax: System.GetViewCameraPos()
	// Description:
	//		Gets the view camera position.
	int GetViewCameraPos(IFunctionHandler *pH);
	// <title GetViewCameraDir>
	// Syntax: System.GetViewCameraDir()
	// Description:
	//		Gets the view camera direction.
	int GetViewCameraDir(IFunctionHandler *pH);
	// <title GetViewCameraAngles>
	// Syntax: System.GetViewCameraAngles()
	// Description:
	//		Gets the view camera angles.
	int GetViewCameraAngles(IFunctionHandler *pH);
	// <title RayWorldIntersection>
	// Syntax: System.RayWorldIntersection(Vec3 vPos, Vec3 vDir, int nMaxHits, int iEntTypes)
	// Arguments:
	//		vPos		- Position vector.
	//		vDir		- Direction vector.
	//		nMaxHits	- Maximum number of hits.
	//		iEntTypes	- .
	// Description:
	//		Shots rays into the world.
	int RayWorldIntersection(IFunctionHandler *pH);
	// <title RayTraceCheck>
	// Syntax: System.RayTraceCheck(Vec3 src, Vec3 dst, int	skipId1, int skipId2)
	int RayTraceCheck(IFunctionHandler *pH);
	// <title BrowseURL>
	// Syntax: System.BrowseURL(const char *szURL)
	// Arguments:
	//		szURL - URL string.
	// Description:
	//		Browses a URL address.
	int BrowseURL(IFunctionHandler *pH);
	// <title IsDevModeEnable>
	// Syntax: System.IsDevModeEnable()
	// Description:
	//		Checks if game is running in dev mode (cheat mode)
	//		to check if we are allowed to enable certain scripts
	//		function facilities (god mode, fly mode etc.).
	int IsDevModeEnable(IFunctionHandler* pH);
	// <title SaveConfiguration>
	// Syntax: System.SaveConfiguration()
	// Description:
	//		Saves the configuration.
	int SaveConfiguration(IFunctionHandler *pH);
	// <title SetSystemShaderRenderFlags>
	// Syntax: System.SetSystemShaderRenderFlags( const char *pszShaderName, const char *pszFlagName, bool bEnable)
	// Arguments:
	//		pszShaderName	- Shader name.
	//		pszFlagName		- Flag name for the shader.
	//		bEnable			- True to enable the shader flag, false otherwise.
	// Description:
	//		Sets the render flags for the system shader.
	int SetSystemShaderRenderFlags(IFunctionHandler *pH);
	// <title Quit>
	// Syntax: System.Quit()
	// Description:
	//		Quits the program.
	int Quit(IFunctionHandler *pH);
	// <title GetHDRDynamicMultiplier>
	// Syntax: System.GetHDRDynamicMultiplier()
	// Description:
	//		Gets hte HDR dynamic multiplier.
	int GetHDRDynamicMultiplier(IFunctionHandler *pH);
	// <title SetHDRDynamicMultiplier>
	// Syntax: System.SetHDRDynamicMultiplier( float fMul )
	// Arguments:
	//		fMul - Dynamic multiplier value.
	// Description:
	//		Sets the HDR dynamic multiplier.
	int SetHDRDynamicMultiplier(IFunctionHandler *pH);
	// <title GetFrameID>
	// Syntax: System.GetFrameID()
	// Description:
	//		Gets the frame identifier.
	int GetFrameID(IFunctionHandler *pH);

	// <title ClearKeyState>
	// Syntax: System.ClearKeyState()
	// Description:
	//    Clear the key state
	int ClearKeyState(IFunctionHandler *pH);

	// <title SetSunColor>
	// Syntax: System.SetSunColor( Vec3 vColor )
	// Description:
	//    Set color of the sun, only relevant outdoors.
	// Arguments:
	//    vColor - Sun Color as an {x,y,z} vector (x=r,y=g,z=b).
	int SetSunColor(IFunctionHandler *pH,Vec3 vColor );

	// <title GetSunColor>
	// Syntax: Vec3 System.GetSunColor()
	// Description:
	//    Retrieve color of the sun outdoors.
	// Return Value:
	//    Sun Color as an {x,y,z} vector (x=r,y=g,z=b).
	int GetSunColor(IFunctionHandler *pH );

	// <title SetSkyColor>
	// Syntax: System.SetSkyColor( Vec3 vColor )
	// Description:
	//    Set color of the sky (outdoors ambient color).
	// Arguments:
	//    vColor - Sky Color as an {x,y,z} vector (x=r,y=g,z=b).
	int SetSkyColor(IFunctionHandler *pH,Vec3 vColor );

	// <title GetSkyColor>
	// Syntax: Vec3 System.GetSkyColor()
	// Description:
	//    Retrieve color of the sky (outdoor ambient color).
	// Return Value:
	//    Sky Color as an {x,y,z} vector (x=r,y=g,z=b).
	int GetSkyColor(IFunctionHandler *pH );

	// <title SetSkyHighlight>
	// Syntax: System.SetSkyHighlight( Table params )
	// Description:
	//    Set Sky highlighing parameters.
	// See Also: GetSkyHighlight
	// Arguments:
	//    params - Table with Sky highlighing parameters.
	//      <TABLE>
	//          Highligh Params                  Meaning
	//          -------------                    -----------
	//          size                             Sky highlight scale.
	//          color                            Sky highlight color.
	//          direction                        Direction of the sky highlight in world space.
	//          pod                              Position of the sky highlight in world space.
	//       </TABLE>
	int SetSkyHighlight(IFunctionHandler *pH,SmartScriptTable params );

	// <title SetSkyHighlight>
	// Syntax: System.SetSkyHighlight( Table params )
	// Description:
	//    Retrieves Sky highlighing parameters.
	//    see SetSkyHighlight for parameters description.
	// See Also: SetSkyHighlight
	int GetSkyHighlight(IFunctionHandler *pH,SmartScriptTable params );


	// <title LoadLocalizationXml>
	// Syntax: System.LoadLocalizationXml( string filename )
	// Description:
	//    Loads Excel exported xml file with text and dialog localization data.
	int LoadLocalizationXml( IFunctionHandler *pH,const char *filename );

private:
	void MergeTable(IScriptTable *pDest, IScriptTable *pSrc);
	//log a string to console and/or to disk with support for different languages
	void	LogString(IFunctionHandler *pH,bool bToConsoleOnly);
	int DeformTerrainInternal( IFunctionHandler *pH, bool nameIsMaterial );

private:
	ISystem *m_pSystem;
	ILog *m_pLog;
	IRenderer *m_pRenderer;
	IConsole *m_pConsole;
	ITimer *m_pTimer;
	I3DEngine *m_p3DEngine;

	//	ICVar *r_hdrrendering;						// the Cvar is created in cry3dengine, this is just a pointer


	//Vlad is too lazy to add this to 3DEngine - so I have to put it here. It should 
	//not be here, but I have to put it somewhere.....
	float	m_SkyFadeStart;		// when fogEnd less - start to fade sky to fog
	float	m_SkyFadeEnd;			// when fogEnd less - no sky, just fog

	SmartScriptTable m_pScriptTimeTable;
	SmartScriptTable m_pGetEntitiesCacheTable;
};

#endif // __ScriptBind_System_h__
