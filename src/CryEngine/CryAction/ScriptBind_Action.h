#ifndef __SCRIPTBIND_ACTION__
#define __SCRIPTBIND_ACTION__

#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>
#include <IViewSystem.h>

// FIXME: Cell SDK GCC bug workaround.
#ifndef __IGAMEOBJECTSYSTEM_H__
#include "IGameObjectSystem.h"
#endif

class CCryAction;
class CScriptBind_Action :
	public CScriptableBase
{
public:
	CScriptBind_Action(CCryAction *pCryAction);
	virtual ~CScriptBind_Action();

	void Release() { delete this; };

	// <title LoadXML>
	// Syntax: Action.LoadXML(const char * definitionFile, const char * dataFile )
	// Arguments:
	//		destinationFile - .
	//		dataFile		- XML-lua data file name.
	int LoadXML( IFunctionHandler *pH, const char * definitionFile, const char * dataFile );
	// <title SaveXML>
	// Syntax: Action.SaveXML(const char * definitionFile, const char * dataFile, SmartScriptTable dataTable)
	// Arguments:
	//		destinationFile - .
	//		dataFile		- XML-lua data file name.
	//		dataTable		- .
	int SaveXML( IFunctionHandler *pH, const char * definitionFile, const char * dataFile, SmartScriptTable dataTable );
	// <title IsServer>
	// Syntax: Action.IsServer()
	// Description:
	//		Checks if the current script runs on a server.
	int IsServer( IFunctionHandler *pH );
	// <title IsClient>
	// Syntax: Action.IsClient()
	// Description:
	//		Checks if the current script runs on a client.
	int IsClient( IFunctionHandler *pH );
	// <title IsGameStarted>
	// Syntax: Action.IsGameStarted()
	// Description:
	//		Checks if the game is started.
	int IsGameStarted( IFunctionHandler *pH );
	// <title IsRMIServer>
	// Syntax: Action.IsRMIServer()
	// Description:
	//		Checks if the current script runs on a RMI (Remote Method Invocation)server.
	int IsRMIServer( IFunctionHandler *pH );
	// <title GetPlayerList>
	// Syntax: Action.GetPlayerList()
	// Description:
	//		Checks the current players list.
	int GetPlayerList( IFunctionHandler *pH );
	// <title IsGameObjectProbablyVisible>
	// Syntax: Action.IsGameObjectProbablyVisible( ScriptHandle gameObject )
	// Arguments:
	//		gameObject - Object that we want to check.
	// Description:
	//		Checks if an object is probably visible.
	int IsGameObjectProbablyVisible( IFunctionHandler *pH, ScriptHandle gameObject );
	// <title ActivateEffect>
	// Syntax: Action.ActivateEffect( const char * name )
	// Arguments:
	//		name - Name of the effect.
	// Description:
	//		Activates the specified effect.
	int ActivateEffect(IFunctionHandler *pH, const char * name );
	// <title GetWaterInfo>
	// Syntax: Action.GetWaterInfo( Vec3 pos )
	// Arguments:
	//		pos - Position to be checked.
	// Description:
	//		Gets information about the water at the position pos.
	int GetWaterInfo(IFunctionHandler *pH, Vec3 pos );
	// <title SetViewCamera>
	// Syntax: Action.SetViewCamera()
	// Description:
	//		Saves the previous valid view and override it with the current camera settings.
	int SetViewCamera(IFunctionHandler *pH);
	// <title ResetToNormalCamera>
	// Syntax: Action.ResetToNormalCamera()
	// Description:
	//		Resets the camera to the last valid view stored.
	int ResetToNormalCamera(IFunctionHandler *pH);
	// <title GetServer>
	// Syntax: Action.GetServer( int number )
	// Description:
	//		Gets the server.
	int GetServer( IFunctionHandler *pFH, int number );
	// <title RefreshPings>
	// Syntax: Action.RefreshPings()
	// Description:
	//		Refreshes pings for all the server liste.d
	int RefreshPings( IFunctionHandler *pFH );
	// <title ConnectToServer>
	// Syntax: Action.ConnectToServer( char* server )
	// Arguments:
	//		server - String value that specifies the server to be used for the connection.
	// Description:
	//		Connects to the specified server.
	int ConnectToServer( IFunctionHandler *pFH, char* server );
	// <title GetServerTime>
	// Syntax: Action.GetServerTime()
	// Description:
	//		Gets the current time on the server.
	int GetServerTime( IFunctionHandler *pFH );
	// <title PauseGame>
	// Syntax: Action.PauseGame( bool pause )
	// Arguments:
	//		pause - True to set the game into the pause mode, false to restart the game.
	// Description:
	//		Puts the game into pause mode.
	int PauseGame( IFunctionHandler *pFH, bool pause );
	// <title IsImmersivenessEnabled>
	// Syntax: Action.IsImmersivenessEnabled()
	// Description:
	//		Gets if the immersive multiplayer is enabled.
	int IsImmersivenessEnabled( IFunctionHandler * pH );
	// <title IsChannelSpecial>
	// Syntax: Action.IsChannelSpecial()
	// Description:
	//		Gets if the channel is a special one.
	int IsChannelSpecial( IFunctionHandler * pH );

	// <title ForceGameObjectUpdate>
	// Syntax: Action.ForceGameObjectUpdate( ScriptHandle entityId, bool force )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		force		- True to force the update, false otherwise.
	// Description:
	//		Forces the game object to be updated.
	int ForceGameObjectUpdate( IFunctionHandler *pH, ScriptHandle entityId, bool force );
	// <title CreateGameObjectForEntity>
	// Syntax: Action.CreateGameObjectForEntity( ScriptHandle entityId )
	// Arguments:
	//		entityId	- Identifier for the entity.
	// Description:
	//		Creates a game object for the specified entity.
	int CreateGameObjectForEntity( IFunctionHandler *pH, ScriptHandle entityId );
	// <title BindGameObjectToNetwork>
	// Syntax: Action.BindGameObjectToNetwork( ScriptHandle entityId )
	// Arguments:
	//		entityId	- Identifier for the entity.
	// Description:
	//		Binds game object to the network.
	int BindGameObjectToNetwork(  IFunctionHandler *pH, ScriptHandle entityId );
	// <title ActivateExtensionForGameObject>
	// Syntax: Action.ActivateExtensionForGameObject( ScriptHandle entityId, const char *extension, bool activate )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		extension	- Extension name.
	//		activate	- True to activate the extension, false to deactivate it.
	// Description:
	//		Activates a specified extension for a game object.
	int ActivateExtensionForGameObject( IFunctionHandler *pH, ScriptHandle entityId, const char *extension, bool activate );
	// <title SetNetworkParent>
	// Syntax: Action.SetNetworkParent( ScriptHandle entityId, ScriptHandle parentId )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		parentID	- Identifier for the parent network.
	//	Description:
	//		Sets the network parent.
	int SetNetworkParent( IFunctionHandler *pH, ScriptHandle entityId, ScriptHandle parentId );
	// <title IsChannelOnHold>
	// Syntax: Action.IsChannelOnHold( int channelId )
	// Arguments:
	//		channelId	- Identifier for the channel.
	// Description:
	//		Checks if the specified channel is on hold.
	int IsChannelOnHold( IFunctionHandler *pH, int channelId );
	// <title BanPlayer>
	// Syntax: Action.BanPlayer( ScriptHandle entityId, const char* message )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		message		- Message for the ban.
	// Description:
	//		Bans a specified player.
	int BanPlayer( IFunctionHandler *pH, ScriptHandle entityId, const char* message );

	// <title PersistantSphere>
	// Syntax: Action.PersistantSphere( Vec3 pos, float radius, Vec3 color, const char* name, float timeout )
	// Arguments:
	//		pos		- Position of the sphere.
	//		radius	- Radius of the sphere.
	//		color	- Color of the sphere.
	//		name	- Name assigned to the sphere.
	//		timeout - Timeout for the sphere.
	// Description:
	//		Adds a persistent sphere to the world.
	int PersistantSphere(IFunctionHandler* pH, Vec3 pos, float radius, Vec3 color, const char* name, float timeout);
	// <title PersistantLine>
	// Syntax: Action.PersistantLine( Vec3 start, Vec3 end, Vec3 color, const char* name, float timeout )	
	// Arguments:
	//		start	- Starting position of the line.
	//		end		- Ending position of the line.
	//		color	- Color of the line.
	//		name	- Name assigned to the line.
	//		timeout - Timeout for the line.
	// Description:
	//		Adds a persistent line to the world.
	int PersistantLine(IFunctionHandler* pH, Vec3 start, Vec3 end, Vec3 color, const char* name, float timeout);
	// <title PersistantArrow>
	// Syntax: Action.PersistantArrow( Vec3 pos, float radius, Vec3 dir, Vec3 color, const char* name, float timeout )
	// Arguments:
	//		pos		- Position of the arrow.
	//		radius	- Radius of the arrow.
	//		dir		- Direction of the arrow.
	//		color	- Color of the arrow.
	//		name	- Name assigned to the arrow.
	//		timeout - Timeout for the arrow.
	// Description:
	//		Adds a persistent arrow to the world.
	int PersistantArrow(IFunctionHandler* pH, Vec3 pos, float radius, Vec3 dir, Vec3 color, const char* name, float timeout);
	// <title Persistant2DText>
	// Syntax: Action.Persistant2DText( const char* text, float size, Vec3 color, const char* name, float timeout )
	// Arguments:
	//		text	- Text that has to be displayed.
	//		size	- Size of the 2D text.
	//		color	- Color of the 2D text.
	//		name	- Name assigned to the 2D text.
	//		timeout - Timeout for the 2D text.
	// Description:
	//		Adds a persistent 2D text.
	int Persistant2DText(IFunctionHandler* pH, const char* text, float size, Vec3 color, const char* name, float timeout);
	// <title PersistantEntityTag>
	// Syntax: Action.PersistantEntityTag( ScriptHandle entityId, const char *text )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		text		- Text for the entity tag.
	// Description:
	//		Adds a persistent entity tag.
	int PersistantEntityTag(IFunctionHandler* pH, ScriptHandle entityId, const char *text);
	// <title ClearEntityTags>
	// Syntax: Action.ClearEntityTags( ScriptHandle entityId )
	// Arguments:
	//		entityId	- Identifier for the entity.
	// Description:
	//		Clears the tag for the specified entity.
	int ClearEntityTags(IFunctionHandler* pH, ScriptHandle entityId);
	// <title ClearStaticTag>
	// Syntax: Action.ClearStaticTag( ScriptHandle entityId, const char *staticId )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		staticId	- Identifier for the static tag.
	// Description:
	//		Clears the specified static tag for the specified entity.
	int ClearStaticTag(IFunctionHandler* pH, ScriptHandle entityId, const char *staticId);

	// <title SendGameplayEvent>
	// Syntax: Action.SendGameplayEvent( ScriptHandle entityId, int event )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		event		- Integer for the event.
	// Description:
	//		Sends an event for the gameplay.
	int SendGameplayEvent(IFunctionHandler* pH, ScriptHandle entityId, int event);

	// <title CacheItemSound>
	// Syntax: Action.CacheItemSound( const char *itemName )
	// Arguments:
	//		itemName - Item name string.
	// Description:
	//		Caches an item sound.
	int CacheItemSound(IFunctionHandler* pH, const char *itemName);
	// <title CacheItemGeometry>
	// Syntax: Action.CacheItemGeometry( const char *itemName )
	// Arguments:
	//		itemName - Item name string.
	// Description:
	//		Caches an item geometry.
	int CacheItemGeometry(IFunctionHandler* pH, const char *itemName);

	// <title DontSyncPhysics>
	// Syntax: Action.DontSyncPhysics( ScriptHandle entityId )
	// Arguments:
	//		entityId - Identifier for the entity.
	// Description:
	//		Doesn't sync physics for the specified entity.
	int DontSyncPhysics(IFunctionHandler* pH, ScriptHandle entityId);

	// <title EnableSignalTimer>
	// Syntax: Action.EnableSignalTimer( ScriptHandle entityId, const char *sText )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		sText		- Text for the signal.
	// Description:
	//		Enables the signal timer.
	int EnableSignalTimer(IFunctionHandler* pH, ScriptHandle entityId, const char *sText);
	// <title DisableSignalTimer>
	// Syntax: Action.DisableSignalTimer( ScriptHandle entityId, const char *sText )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		sText		- Text for the signal.
	// Description:
	//		Disables the signal timer.
	int DisableSignalTimer(IFunctionHandler* pH, ScriptHandle entityId, const char *sText);
	// <title SetSignalTimerRate>
	// Syntax: Action.SetSignalTimerRate( ScriptHandle entityId, const char *sText, float fRateMin, float fRateMax )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		sText		- Text for the signal.
	//		fRateMin	- Minimum rate for the signal timer.
	//		fRateMax	- Maximum rate for the signal timer.
	// Description:
	//		Sets the rate for the signal timer.
	int SetSignalTimerRate(IFunctionHandler* pH, ScriptHandle entityId, const char *sText, float fRateMin, float fRateMax);
	// <title ResetSignalTimer>
	// Syntax: Action.ResetSignalTimer( ScriptHandle entityId, const char *sText )
	// Arguments:
	//		entityId	- Identifier for the entity.
	//		sText		- Text for the signal.
	// Description:
	//		Resets the rate for the signal timer.
	int ResetSignalTimer(IFunctionHandler* pH, ScriptHandle entityId, const char *sText);

	int EnableRangeSignaling(IFunctionHandler* pH, ScriptHandle entityId, bool bEnable);
	int DestroyRangeSignaling(IFunctionHandler* pH, ScriptHandle entityId);
	int ResetRangeSignaling(IFunctionHandler* pH, ScriptHandle entityId);

	// <title AddRangeSignal>
	// Syntax: Action.AddRangeSignal( ScriptHandle entityId, float fRadius, float fFlexibleBoundary, const char *sSignal )
	// Arguments:
	//		entityId			- Identifier for the entity.
	//		fRadius				- Radius of the range area.
	//		fFlexibleBoundary	- Flexible boundary size.
	//		sSignal				- String for signal.
	// Description:
	//		Adds a range for the signal.
	int AddRangeSignal(IFunctionHandler* pH, ScriptHandle entityId, float fRadius, float fFlexibleBoundary, const char *sSignal);

	int AddTargetRangeSignal(IFunctionHandler* pH, ScriptHandle entityId, ScriptHandle targetId, float fRadius, float fFlexibleBoundary, const char *sSignal);

	// <title AddRangeSignal>
	// Syntax: Action.AddRangeSignal( ScriptHandle entityId, float fAngle, float fFlexibleBoundary, const char *sSignal )
	// Arguments:
	//		entityId			- Identifier for the entity.
	//		fAngle				- Angle value.
	//		fFlexibleBoundary	- Flexible boundary size.
	//		sSignal				- String for signal.
	// Description:
	//		Adds an angle for the signal.
	int AddAngleSignal(IFunctionHandler* pH, ScriptHandle entityId, float fAngle, float fFlexibleBoundary, const char *sSignal);

	// <title RegisterWithAI>
	// Syntax: 
	//		Action.RegisterWithAI()
	// Description: 
	//		Registers the entity to AI System, creating an AI object associated to it
	int RegisterWithAI(IFunctionHandler *pH); 

private:
	void RegisterGlobals();
	void RegisterMethods();
	
	CCryAction *m_pCryAction;
	IView				*m_pPreviousView;
};

#endif
