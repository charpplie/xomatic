////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   ScriptBind_Entity.h
//  Version:     v1.00
//  Created:     28/7/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScriptBind_Entity_h__
#define __ScriptBind_Entity_h__
#pragma once


#include <IScriptSystem.h>

struct IEntity;
struct ISystem;

#include <IPhysics.h>
#include <ParticleParams.h>

#define ENTITYPROP_CASTSHADOWS		0x00000001
#define ENTITYPROP_DONOTCHECKVIS	0x00000002

/* 
	In this class are all entity-related script-functions implemented in order to expose all functionalities provided by an entity.

	IMPLEMENTATIONS NOTES:
	These function will never be called from C-Code. They're script-exclusive.
*/

// <title Entity>
// Syntax: Entity
// Description:
//    Entity table used by all entities to call to the entity system interface.
class CScriptBind_Entity : public CScriptableBase
{
public:
	CScriptBind_Entity( IScriptSystem *pSS,ISystem *pSystem, IEntitySystem *pEntitySystem );
	virtual ~CScriptBind_Entity();

	void DelegateCalls( IScriptTable *pInstanceTable );

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddContainer(m_areaPoints);
		
	}
protected:
	// <title DeleteThis>
	// Syntax: Entity.DeleteThis()
	// Description:
	//    Deletes this entity.
	int DeleteThis( IFunctionHandler *pH );

	// <title LoadObject>
	// Syntax: Entity.LoadObject( int nSlot,const char *sFilename )
	// Description:
	//    Load CGF geometry into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    sFilename  - CGF geometry file name.
	int LoadObject( IFunctionHandler *pH,int nSlot,const char *sFilename );

	// <title LoadObjectWithFlags>
	// Syntax: Entity.LoadObject( int nSlot,const char *sFilename, const int nFlags )
	// Description:
	//    Load CGF geometry into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    sFilename  - CGF geometry file name.
	//    nFlags  - entity load flags
	int LoadObjectWithFlags( IFunctionHandler *pH,int nSlot,const char *sFilename, const int nFlags );

	// <title LoadObject>
	// Syntax: Entity.LoadObjectLattice( int nSlot )
	// Description:
	//    Load lattice into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	int LoadObjectLattice( IFunctionHandler *pH,int nSlot );

	// <title LoadSubObject>
	// Syntax: Entity.LoadSubObject( int nSlot,const char *sFilename,const char *sGeomName )
	// Description:
	//    Load geometry of one CGF node into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    sFilename - CGF geometry file name.
	//    sGeomName - Name of the node inside CGF geometry.
	int LoadSubObject( IFunctionHandler *pH,int nSlot,const char *sFilename,const char *sGeomName );
	
	// <title LoadCharacter>
	// Syntax: Entity.LoadCharacter( int nSlot,const char *sFilename )
	// Description:
	//    Load CGF geometry into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    sFilename  - CGF geometry file name.
	int LoadCharacter( IFunctionHandler *pH,int nSlot,const char *sFilename );
	
	// <title LoadLight>
	// Syntax: Entity.LoadLight( int nSlot,table lightTable )
	// Description:
	//    Load CGF geometry into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    lightTable - Table with all the light information.
	int LoadLight(IFunctionHandler *pH,int nSlot,SmartScriptTable table);
	
	// <title UpdateLightClipBounds>
	// Syntax: Entity.UpdateLightClipBounds( int nSlot )
	// Description:
	//    Update the clip bounds of the light from the linked entities
	// Arguments:
	//    nSlot - Slot identifier.
	int UpdateLightClipBounds(IFunctionHandler *pH,int nSlot);


	// <title SetLightCasterException>
	// Syntax: Entity.SetLightCasterException( int nLightSlot, int nGeometrySlot )
	// Description:
	//    Entity render node will be a caster exception for light loaded in nLightSlot.
	// Arguments:
	//    nLightSlot - Slot where our light is loaded.
	int SetSelfAsLightCasterException(IFunctionHandler *pH, int nLightSlot);

	// <title LoadCloud>
	// Syntax: Entity.LoadCloud( int nSlot,sFilename )
	// Description:
	//    Loads the cloud XML file into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    sFilename - Filename.
	int LoadCloud(IFunctionHandler *pH, int nSlot, const char *sFilename);
	// <title SetCloudMovementProperties>
	// Syntax: Entity.SetCloudMovementProperties( int nSlot, table )
	// Description:
	//    Sets the cloud movement properties.
	// Arguments:
	//    nSlot - Slot identifier.
	//    table - Table property for the cloud movement.
	int SetCloudMovementProperties(IFunctionHandler *pH, int nSlot, SmartScriptTable table);

	// <title LoadFogVolume>
	// Syntax: Entity.LoadFogVolume( int nSlot, table )
	// Description:
	//    Loads the fog volume XML file into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    table - Table with fog volume properties.
	int LoadFogVolume( IFunctionHandler* pH, int nSlot, SmartScriptTable table );
	// <title FadeGlobalDensity>
	// Syntax: Entity.FadeGlobalDensity( int nSlot, float fadeTime, float newGlobalDensity )
	// Description:
	//    Sets the fade global density.
	// Arguments:
	//	  nSlot - Slot identifier.
	//    fadeTime - .
	//    newGlobalDensity - .
	int FadeGlobalDensity(IFunctionHandler *pH, int nSlot, float fadeTime, float newGlobalDensity);

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
	int LoadPrismObject(IFunctionHandler* pH, int nSlot);
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

	// <title LoadVolumeObject>
	// Syntax: Entity.LoadVolumeObject( int nSlot, const char* sFilename )
	// Arguments:
	//		nSlot		- Slot identifier.
	//		sFilename	- File name of the volume object.
	// Description:
	//		Loads volume object.
	int LoadVolumeObject(IFunctionHandler* pH, int nSlot, const char* sFilename);
	// <title SetVolumeObjectMovementProperties>
	// Syntax: Entity.SetVolumeObjectMovementProperties( int nSlot, table )
	// Arguments:
	//		nSlot		- Slot identifier.
	//		table		- Table with volume object properties.
	// Description:
	//		Sets the properties of the volume object movement.
	int SetVolumeObjectMovementProperties(IFunctionHandler *pH, int nSlot, SmartScriptTable table);

	// <title LoadParticleEffect>
	// Syntax: Entity.LoadParticleEffect( int nSlot,const char *sEffectName,float fPulsePeriod,bool bPrime,float fScale )
	// Description:
	//    Loads CGF geometry into the entity slot.
	// Arguments:
	//    nSlot - Slot identifier.
	//    sEffectName - Name of the particle effect (Ex: "explosions/rocket").
	//		Optional table fields:
	//			bPrime - Whether effect starts fully primed to equilibrium state.
	//			fPulsePeriod - Time period between particle effect restarts.
	//			fScale - Size scale to apply to particles
	//			fCountScale - Count multiplier to apply to particles
	//			bScalePerUnit - Scale size by attachment extent
	//			bCountPerUnit - Scale count by attachment extent
	//			sAttachType - string for EGeomType
	//			sAttachForm - string for EGeomForm
	int LoadParticleEffect(IFunctionHandler *pH,int nSlot, const char *sEffectName, SmartScriptTable table );
	// <title PreLoadParticleEffect>
	// Syntax: Entity.PreLoadParticleEffect( const char *sEffectName )
	// Description:
	//    Pre-loads a particle effect.
	// Arguments:
	//    sEffectName - Name of the particle effect (Ex: "explosions/rocket").
	int PreLoadParticleEffect(IFunctionHandler *pH, const char *sEffectName );
	// <title IsSlotParticleEmitter>
	// Syntax: Entity.IsSlotParticleEmitter( int slot )
	// Description:
	//    Checks if the slot is a particle emitter.
	// Arguments:
	//    slot - Slot identifier.
	int IsSlotParticleEmitter(IFunctionHandler *pH, int slot);
	// <title IsSlotLight>
	// Syntax: Entity.IsSlotLight( int slot )
	// Description:
	//    Checks if the slot is a light.
	// Arguments:
	//    slot - Slot identifier.
	int IsSlotLight(IFunctionHandler *pH, int slot);
	// <title IsSlotGeometry>
	// Syntax: Entity.IsSlotGeometry( int slot )
	// Description:
	//    Checks if the slot is a geometry.
	// Arguments:
	//    slot - Slot identifier.
	int IsSlotGeometry(IFunctionHandler *pH, int slot);
	// <title IsSlotCharacter>
	// Syntax: Entity.IsSlotCharacter( int slot )
	// Description:
	//    Checks if the slot is a character.
	// Arguments:
	//    slot - Slot identifier.
	int IsSlotCharacter(IFunctionHandler *pH, int slot);


	//////////////////////////////////////////////////////////////////////////
	// Slots.
	//////////////////////////////////////////////////////////////////////////

	// <title GetSlotCount>
	// Syntax: Entity.GetSlotCount()
	// Description:
	//    Gets the count of the slots.
	int GetSlotCount(IFunctionHandler* pH);
	// <title GetSlotPos>
	// Syntax: Entity.GetSlotPos( int nSlot )
	// Description:
	//    Gets the slot position.
	// Arguments:
	//	  nSlot - Slot identifier.
	int GetSlotPos(IFunctionHandler *pH, int nSlot );
	// <title SetSlotPos>
	// Syntax: Entity.SetSlotPos( int nSlot,Vec3 v )
	// Description:
	//    Sets the slot position.
	// Arguments:
	//	  nSlot - Slot identifier.
	//	  v		- Position to be set.
	int SetSlotPos(IFunctionHandler *pH, int nSlot,Vec3 v );
	// <title SetSlotPosAndDir>
	// Syntax: Entity.SetSlotPosAndDir( int nSlot, Vec3 pos, Vec3 dir )
	// Description:
	//    Sets the slot position and direction.
	// Arguments:
	//	  nSlot - Slot identifier.
	//	  pos	- Position to be set.
	//	  dir	- Direction to be set.
	int SetSlotPosAndDir(IFunctionHandler *pH, int nSlot, Vec3 pos, Vec3 dir);
	// <title GetSlotAngles>
	// Syntax: Entity.GetSlotAngles( int nSlot )
	// Description:
	//    Gets the slot angles.
	// Arguments:
	//	  nSlot - Slot identifier.
	int GetSlotAngles(IFunctionHandler *pH, int nSlot);
	// <title GetSlotAngles>
	// Syntax: Entity.GetSlotAngles( int nSlot,Ang3 v  )
	// Description:
	//    Sets the slot angles.
	// Arguments:
	//	  nSlot - Slot identifier.
	//	  v		- Angle to be set.
	int SetSlotAngles(IFunctionHandler *pH, int nSlot,Ang3 v );
	// <title GetSlotScale>
	// Syntax: Entity.GetSlotScale( int nSlot )
	// Description:
	//    Gets the slot scale amount.
	// Arguments:
	//	  nSlot - Slot identifier.
	int GetSlotScale(IFunctionHandler *pH, int nSlot);
	// <title SetSlotScale>
	// Syntax: Entity.SetSlotScale( int nSlot,float fScale )
	// Description:
	//    Sets the slot scale amount.
	// Arguments:
	//	  nSlot		- Slot identifier.
	//	  fScale	- Scale amount for the slot.
	int SetSlotScale(IFunctionHandler *pH, int nSlot,float fScale );
	// <title IsSlotValid>
	// Syntax: Entity.IsSlotValid( int nSlot )
	// Description:
	//    Checks if the slot is valid.
	// Arguments:
	//	  nSlot		- Slot identifier.
	int IsSlotValid(IFunctionHandler *pH, int nSlot);
	// <title CopySlotTM>
	// Syntax: Entity.CopySlotTM( int destSlot, int srcSlot, bool includeTranslation )
	// Description:
	//    Copies the TM (Transformation Matrix) of the slot.
	// Arguments:
	//	  destSlot				- Destination slot identifier.
	//	  srcSlot				- Source slot identifier.
	//	  includeTranslation	- True to include the translation, false otherwise.
	int CopySlotTM(IFunctionHandler *pH, int destSlot, int srcSlot, bool includeTranslation);
	// <title MultiplyWithSlotTM>
	// Syntax: Entity.MultiplyWithSlotTM( int slot, Vec3 pos )
	// Description:
	//    Multiplies with the TM (Transformation Matrix) of the slot.
	// Arguments:
	//	  slot	- Slot identifier.
	//	  pos	- Position vector.
	int MultiplyWithSlotTM(IFunctionHandler *pH, int slot, Vec3 pos);
	// <title SetSlotWorldTM>
	// Syntax: Entity.SetSlotWorldTM( int nSlot, Vec3 pos, Vec3 dir )
	// Description:
	//    Sets the World TM (Transformation Matrix) of the slot.
	// Arguments:
	//	  nSlot	- Slot identifier.
	//	  pos	- Position vector.
	//	  dir	- Direction vector.
	int SetSlotWorldTM(IFunctionHandler *pH, int nSlot, Vec3 pos, Vec3 dir);
	// <title GetSlotWorldPos>
	// Syntax: Entity.GetSlotWorldPos( int nSlot )
	// Description:
	//    Gets the World position of the slot.
	// Arguments:
	//	  nSlot	- Slot identifier.
	int GetSlotWorldPos(IFunctionHandler *pH, int nSlot);
	// <title GetSlotWorldDir>
	// Syntax: Entity.GetSlotWorldDir( int nSlot )
	// Description:
	//    Gets the World direction of the slot.
	// Arguments:
	//	  nSlot	- Slot identifier.
	int GetSlotWorldDir(IFunctionHandler *pH, int nSlot);



	//////////////////////////////////////////////////////////////////////////

	// <title SetPos>
	// Syntax: Entity.SetPos( Vec3 vPos )
	// Description:
	//    Sets the position of the entity.
	// Arguments:
	//	  vPos - Position vector.
	int SetPos(IFunctionHandler *pH,Vec3 vPos );
	// <title SetPos>
	// Syntax: Entity.GetPos()
	// Description:
	//    Gets the position of the entity.
	int GetPos(IFunctionHandler *pH);
	// <title SetAngles>
	// Syntax: Entity.SetAngles( Ang3 vAngles )
	// Arguments:
	//		vAngles - Angle vector.
	// Description:
	//    Sets the angle of the entity.
	int SetAngles(IFunctionHandler *pH,Ang3 vAngles );
	// <title GetAngles>
	// Syntax: Entity.GetAngles()
	// Description:
	//    Gets the angle of the entity.
	int GetAngles(IFunctionHandler *pH);
	// <title SetScale>
	// Syntax: Entity.SetScale( float fScale )
	// Arguments:
	//		fScale - Scale amount.
	// Description:
	//    Sets the scaling value for the entity.
	int SetScale(IFunctionHandler *pH,float fScale );
	// <title GetsScale>
	// Syntax: Entity.GetScale()
	// Description:
	//    Gets the scaling value for the entity.
	int GetScale(IFunctionHandler *pH);
	// <title GetCenterOfMassPos>
	// Syntax: Entity.GetCenterOfMassPos()
	// Description:
	//    Gets the position of the entity center of mass.
	int GetCenterOfMassPos(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////
	// <title SetLocalPos>
	// Syntax: Entity.SetLocalPos( Vec3 vPos )
	int SetLocalPos(IFunctionHandler *pH,Vec3 vPos );
	// <title GetLocalPos>
	// Syntax: Vec3 Entity.GetLocalPos()
	int GetLocalPos(IFunctionHandler *pH);
	// <title SetLocalAngles>
	// Syntax: Entity.SetLocalAngles( Vec3 vAngles )
	int SetLocalAngles(IFunctionHandler *pH,Ang3 vAngles );
	// <title GetLocalAngles>
	// Syntax: Vec3 Entity.GetLocalAngles( Vec3 vAngles )
	int GetLocalAngles(IFunctionHandler *pH);
	// <title SetLocalScale>
	// Syntax: Entity.SetLocalScale( float fScale )
	int SetLocalScale(IFunctionHandler *pH,float fScale );
	// <title GetLocalScale>
	// Syntax: float Entity.GetLocalScale()
	int GetLocalScale(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////
	// <title SetWorldPos>
	// Syntax: Entity.SetWorldPos( Vec3 vPos )
	int SetWorldPos(IFunctionHandler *pH,Vec3 vPos );
	// <title GetWorldPos>
	// Syntax: Vec3 Entity.GetWorldPos()
	int GetWorldPos(IFunctionHandler *pH);
	// <title SetWorldAngles>
	// Syntax: Entity.SetWorldAngles( Vec3 vAngles )
	int SetWorldAngles(IFunctionHandler *pH,Ang3 vAngles );
	// <title GetWorldAngles>
	// Syntax: Vec3 Entity.GetWorldAngles( Vec3 vAngles )
	int GetWorldAngles(IFunctionHandler *pH);
	// <title SetWorldScale>
	// Syntax: Entity.SetWorldScale( float fScale )
	int SetWorldScale(IFunctionHandler *pH,float fScale );
	// <title GetWorldScale>
	// Syntax: float Entity.GetWorldScale()
	int GetWorldScale(IFunctionHandler *pH);

	// <title GetBoneLocal>
	// Syntax: float Entity.GetBoneLocal( const char *boneName, Vec3 trgDir )
	int GetBoneLocal(IFunctionHandler *pH, const char *boneName, Vec3 trgDir);
	
	// <title CalcWorldAnglesFromRelativeDir>
	// Syntax: Ang3 Entity.CalcWorldAnglesFromRelativeDir( Vec3 dir )
	int CalcWorldAnglesFromRelativeDir(IFunctionHandler *pH, Vec3 dir);
	
	// <title IsEntityInside>
	// Syntax: float Entity.IsEntityInside(entityId)
	int IsEntityInside(IFunctionHandler *pH, ScriptHandle entityId);


	//////////////////////////////////////////////////////////////////////////

	// <title GetDistance>
	// Syntax: float Entity.GetDistance( entityId )
	// Description:
	//    Return the distance from  entity specified with entityId
	int GetDistance(IFunctionHandler *pH );
	
	//////////////////////////////////////////////////////////////////////////
	// <title DrawSlot>
	// Syntax: Entity.DrawSlot( int nSlot )
	// Description:
	//    Enables/Disables drawing of object or character at specified slot of the entity.
	// Arguments:
	//    nSlot - Slot identifier.
	//    nEnable - 1-Enable drawing, 0-Disable drawing.
	int DrawSlot(IFunctionHandler *pH, int nSlot,int nEnable );

	//////////////////////////////////////////////////////////////////////////
	// <title IgnorePhysicsUpdatesOnSlot>
	// Syntax: Entity.IgnorePhysicsUpdatesOnSlot( int nSlot )
	// Description:
	//    Ignore physics when it try to update the position of a slot.
	// Arguments:
	//    nSlot - Slot identifier.
	int IgnorePhysicsUpdatesOnSlot(IFunctionHandler *pH, int nSlot);

	// <title FreeSlot>
	// Syntax: Entity.FreeSlot( int nSlot )
	// Description:
	//    Delete all objects from specified slot.
	// Arguments:
	//    nSlot - Slot identifier.
	int FreeSlot(IFunctionHandler *pH, int nSlot );

	// <title FreeAllSlots>
	// Syntax: Entity.FreeAllSlots()
	// Description:
	//    Delete all objects on every slot part of the entity.
	int FreeAllSlots(IFunctionHandler *pH);

	// <title GetCharacter>
	// Syntax: Entity.GetCharacter( int nSlot )
	// Description:
	//    Gets the character for the specified slot if there is any.
	int GetCharacter(IFunctionHandler *pH, int nSlot );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Physics.
	//////////////////////////////////////////////////////////////////////////

	// <title DestroyPhysics>
	// Syntax: Entity.DestroyPhysics()
	int DestroyPhysics(IFunctionHandler *pH);
	// <title EnablePhysics>
	// Syntax: Entity.EnablePhysics( bool bEnable )
	int EnablePhysics(IFunctionHandler *pH,bool bEnable);
	// <title ResetPhysics>
	// Syntax: Entity.ResetPhysics()
	int ResetPhysics(IFunctionHandler *pH);
	// <title AwakePhysics>
	// Syntax: Entity.AwakePhysics( int nAwake )
	int AwakePhysics(IFunctionHandler *pH,int nAwake);
	// <title AwakeCharacterPhysics>
	// Syntax: Entity.AwakeCharacterPhysics( int nSlot,const char *sRootBoneName,int nAwake )
	int AwakeCharacterPhysics(IFunctionHandler *pH,int nSlot,const char *sRootBoneName,int nAwake);
	
	// <title Physicalize>
	// Syntax: Entity.Physicalize( int nSlot,int nPhysicsType,table physicsParams )
	// Description:
	//    Create physical entity from the specified entity slot.
	//      <TABLE>
	//          Physics Type        Meaning
	//          -------------       -----------
	//          PE_NONE             No physics.
	//          PE_STATIC           Static physical entity.
	//          PE_LIVING           Live physical entity (Players,Monsters).
	//          PE_RIGID            Rigid body physical entity.
	//          PE_WHEELEDVEHICLE   Physical vechicle with wheels.
	//          PE_PARTICLE         Particle physical entity, it only have mass and radius.
	//          PE_ARTICULATED      Ragdolls or other articulated physical enttiies.
	//          PE_ROPE             Physical representation of the rope.
	//          PE_SOFT             Soft body physics, cloth simulation.
	//          PE_AREA             Physical Area (Sphere,Box,Geometry or Shape).
	//       </TABLE>
	//
	//      <TABLE>
	//          Params table keys   Meaning
	//          -----------------   -----------
	//          mass                Object mass, only used if density is not specified or -1.
	//          density             Object density, only used if mass is not specified or -1.
	//          flags               Physical entity flags.
	//          partid              Index of the articulated body part, that this new physical entity will be attached to.
	//          stiffness_scale     Scale of character joints stiffness (Multiplied with stiffness values specified from exported model)
	//          Particle            This table must be set when Physics Type is PE_PARTICLE.
	//          Living              This table must be set when Physics Type is PE_LIVING.
	//          Area                This table must be set when Physics Type is PE_AREA.
	//       </TABLE>
	//
	//      <TABLE>
	//          Particle table      Meaning
	//          -----------------   -----------
	//          mass                Particle mass.
	//          radius              Particle pseudo radius.
	//          thickness           Thickness when lying on a surface (if 0, radius will be used).
	//          velocity            Velocity direction and magnitude vector.
	//          air_resistance      Air resistance coefficient, F = kv.
	//          water_resistance    Water resistance coefficient, F = kv.
	//          gravity             Gravity force vector to the air.
	//          water_gravity       Gravity force vector when in the water.
	//          min_bounce_vel      Minimal velocity at which particle bounces of the surface.
	//          accel_thrust        Acceleration along direction of movement.
	//          accel_lift          Acceleration that lifts particle with the current speed.
	//          constant_orientation (0,1) Keep constance orientation.
	//          single_contact      (0,1) Calculate only one first contact.
	//          no_roll             (0,1) Do not roll particle on terrain.
	//          no_spin             (0,1) Do not spin particle in air.
	//          no_path_alignment   (0,1) Do not align particle orientation to the movement path.
	//       </TABLE>
	//      <TABLE>
	//          Living table        Meaning
	//          -----------------   -----------
	//          height              Vertical offset of collision geometry center.
	//          size                Collision cylinder dimensions vector (x,y,z).
	//          height_eye          Vertical offset of the camera.
	//          height_pivot        Offset from central ground position that is considered entity center.
	//          head_radius         Radius of the head.
	//          height_head         Vertical offset of the head.

	//          inertia             Inertia coefficient, the more it is, the less inertia is; 0 means no inertia
	//          air_resistance      Air control coefficient 0..1, 1 - special value (total control of movement)
	//          gravity             Vertical gravity magnitude.
	//          mass                Mass of the player (in kg).
	//          min_slide_angle     If surface slope is more than this angle, player starts sliding (In radians)
	//          max_climb_angle     Player cannot climb surface which slope is steeper than this angle (In radians)
	//          max_jump_angle      Player is not allowed to jump towards ground if this angle is exceeded (In radians)
	//          min_fall_angle      Player starts falling when slope is steeper than this (In radians)
	//          max_vel_ground      Player cannot stand on surfaces that are moving faster than this (In radians)
	//       </TABLE>
	//
	//      <TABLE>
	//          Area table keys     Meaning
	//          -----------------   -----------
	//          type                Type of the area, valid values are: AREA_SPHERE,AREA_BOX,AREA_GEOMETRY,AREA_SHAPE
	//          radius              Radius of the area sphere, must be specified if type is AREA_SPHERE.
	//          box_min             Min vector of bounding box, must be specified if type is AREA_BOX.
	//          box_max             Max vector of bounding box, must be specified if type is AREA_BOX..
	//          points              Table, indexed collection of vectors in local entity space defining 2D shape of the area, (AREA_SHAPE)
	//          height              Height of the 2D area (AREA_SHAPE), relative to the minimal Z in the points table.
	//          uniform             Same direction in every point or always point to the center.
	//          falloff             ellipsoidal falloff dimensions; 0,0,0 if no falloff.
	//          gravity             Gravity vector inside the physical area.
	//       </TABLE>
	// Arguments:
	//    nSlot - Slot identifier, if -1 use geometries from all slots.
	//    nPhysicsType - Type of physical entity to create.
	//    physicsParams - Table with physicalization parameters.
	int Physicalize(IFunctionHandler *pH,int nSlot,int nPhysicsType,SmartScriptTable physicsParams);

	// <title SetPhysicParams>
	// Syntax: Entity.SetPhysicParams()
	int SetPhysicParams(IFunctionHandler *pH);
	// <title SetCharacterPhysicParams>
	// Syntax: Entity.SetCharacterPhysicParams()
	int SetCharacterPhysicParams(IFunctionHandler *pH);
	// <title ActivatePlayerPhysics>
	// Syntax: Entity.ActivatePlayerPhysics( bool bEnable )
	int ActivatePlayerPhysics(IFunctionHandler *pH,bool bEnable);

	// <title PhysicalizeSlot>
	// Syntax: Entity.PhysicalizeSlot( int slot, SmartScriptTable physicsParams )
	int PhysicalizeSlot(IFunctionHandler *pH, int slot, SmartScriptTable physicsParams);
	// <title UpdateSlotPhysics>
	// Syntax: Entity.UpdateSlotPhysics( int slot )
	int UpdateSlotPhysics(IFunctionHandler *pH, int slot);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////

	// <title SetColliderMode>
	// Syntax: Entity.SetColliderMode( int mode )
	int SetColliderMode(IFunctionHandler *pH, int mode);

	//////////////////////////////////////////////////////////////////////////

	// <title SelectPipe>
	// Syntax: Entity.SelectPipe()
	int SelectPipe(IFunctionHandler *pH);

	// <title IsUsingPipe>
	// Syntax: Entity.IsUsingPipe( string pipename )
	// Description:
	//    Returns true if entity is running the given goalpipe or has it inserted
	// Arguments:
	//    pipename - goalpipe name
	// Return:
	//    true - if entity is running the given goalpipe or has it inserted
	//		false - otherwise
	int IsUsingPipe(IFunctionHandler *pH);

	// <title Activate>
	// Syntax: Entity.Activate( int bActivate )
	// Description:
	//    Activates or deactivates entity.
	//    This calls ignores update policy and forces entity to activate or deactivate
	//    All active entities will be updated every frame, having too many active entities can affect performance.
	// Arguments:
	//    bActivate - if true entity will become active, is false will deactivate and stop being updated every frame.
	//////////////////////////////////////////////////////////////////////////
	int Activate(IFunctionHandler *pH,int bActive);

	// <title IsActive>
	// Syntax: Entity.IsActive( int bActivate )
	// Description:
	//    Retrieve active status of entity.
	// Return:
	//    true - Entity is active.
	//    false - Entity is not active.
	int IsActive(IFunctionHandler *pH);

	// <title SetUpdatePolicy>
	// Syntax: Entity.SetUpdatePolicy( int nUpdatePolicy )
	// Description:
	//    Changes update policy for the entity.
	//    Update policy controls when entity becomes active or inactive (ex. when visible, when in close proximity, etc...).
	//    All active entities will be updated every frame, having too many active entities can affect performance.
	//      <TABLE>
	//          Update Policy                    Meaning
	//          -------------                    -----------
	//          ENTITY_UPDATE_NEVER              Never update this entity.
	//          ENTITY_UPDATE_IN_RANGE           Activate entity when in specified radius.
	//          ENTITY_UPDATE_POT_VISIBLE        Activate entity when potentially visible.
	//          ENTITY_UPDATE_VISIBLE            Activate entity when visible in frustum.
	//          ENTITY_UPDATE_PHYSICS            Activate entity when physics awakes, deactivate when physics go to sleep.
	//          ENTITY_UPDATE_PHYSICS_VISIBLE    Same as ENTITY_UPDATE_PHYSICS, but also activates when visible.
	//          ENTITY_UPDATE_ALWAYS             Entity is always active and updated every frame.
	//       </TABLE>
	// Arguments:
	//    nUpdatePolicy - Update policy type.
	// Remarks:
	//    Use SetUpdateRadius for update policy that require a radius.
	int SetUpdatePolicy(IFunctionHandler *pH,int nUpdatePolicy);
	
	//////////////////////////////////////////////////////////////////////////

	// <title SetLocalBBox>
	// Syntax: Entity.SetLocalBBox( Vec3 vMin,Vec3 vMax )
	int SetLocalBBox(IFunctionHandler *pH,Vec3 vMin,Vec3 vMax);
	// <title GetLocalBBox>
	// Syntax: Entity.GetLocalBBox()
	int GetLocalBBox(IFunctionHandler *pH);
	// <title GetWorldBBox>
	// Syntax: Entity.GetWorldBBox()
	int GetWorldBBox(IFunctionHandler *pH);
	// <title GetProjectedWorldBBox>
	// Syntax: Entity.GetProjectedWorldBBox()
	int GetProjectedWorldBBox(IFunctionHandler *pH);
	// <title SetTriggerBBox>
	// Syntax: Entity.SetTriggerBBox( Vec3 vMin,Vec3 vMax )
	int SetTriggerBBox(IFunctionHandler *pH,Vec3 vMin,Vec3 vMax);
	// <title GetTriggerBBox>
	// Syntax: Entity.GetTriggerBBox()
	int GetTriggerBBox(IFunctionHandler *pH);
	// <title InvalidateTrigger>
	// Syntax: Entity.InvalidateTrigger()
	int InvalidateTrigger(IFunctionHandler *pH);
	// <title ForwardTriggerEventsTo>
	// Syntax: Entity.ForwardTriggerEventsTo( entityId )
	int ForwardTriggerEventsTo(IFunctionHandler *pH, ScriptHandle entityId);

	//////////////////////////////////////////////////////////////////////////

	// <title SetUpdateRadius>
	// Syntax: Entity.SetUpdateRadius()
	int SetUpdateRadius(IFunctionHandler *pH);
	// <title GetUpdateRadius>
	// Syntax: Entity.GetUpdateRadius()
	int GetUpdateRadius(IFunctionHandler *pH);
	// <title TriggerEvent>
	// Syntax: Entity.TriggerEvent()
	int TriggerEvent(IFunctionHandler *pH);
	// <title GetHelperPos>
	// Syntax: Entity.GetHelperPos()
	int GetHelperPos(IFunctionHandler *pH);
	// <title GetHelperDir>
	// Syntax: Entity.GetHelperDir()
	int GetHelperDir(IFunctionHandler *pH);
	// <title GetHelperAngles>
	// Syntax: Entity.GetHelperAngles()
	int GetHelperAngles(IFunctionHandler *pH);
	// <title GetSlotHelperPos>
	// Syntax: Entity.GetSlotHelperPos( int slot, const char *helperName, bool objectSpace )
	int GetSlotHelperPos(IFunctionHandler *pH, int slot, const char *helperName, bool objectSpace);
	// <title GetBonePos>
	// Syntax: Entity.GetBonePos()
	int GetBonePos(IFunctionHandler *pH);
	// <title GetBoneDir>
	// Syntax: Entity.GetBoneDir()
	int GetBoneDir(IFunctionHandler *pH);
	// <title GetBoneVelocity>
	// Syntax: Entity.GetBoneVelocity( int characterSlot, const char *boneName )
	int GetBoneVelocity(IFunctionHandler *pH, int characterSlot, const char *boneName);
	// <title GetBoneAngularVelocity>
	// Syntax: Entity.GetBoneAngularVelocity( int characterSlot, const char *boneName )
	int GetBoneAngularVelocity(IFunctionHandler *pH, int characterSlot, const char *boneName);
	// <title GetBoneNameFromTable>
	// Syntax: Entity.GetBoneNameFromTable()
	int GetBoneNameFromTable(IFunctionHandler *pH);

	// <title SetName>
	// Syntax: Entity.SetName()
	int SetName(IFunctionHandler *pH);
	// <title GetName>
	// Syntax: Entity.GetName()
	int GetName(IFunctionHandler *pH);
	// <title GetRawId>
	// Syntax: Entity.GetRawId()
	// Description:
	//		Returns entityId in raw numeric format.
	int GetRawId(IFunctionHandler *pH); 
	// <title SetAIName>
	// Syntax: Entity.SetAIName()
	int SetAIName(IFunctionHandler *pH);
	// <title GetAIName>
	// Syntax: Entity.GetAIName()
	int GetAIName(IFunctionHandler *pH);
	// <title SetFlags>
	// Syntax: Entity.SetFlags( int flags, int mode )
	// Description:
	//		Mode: 0->or 1->and 2->xor
	int SetFlags(IFunctionHandler *pH, int flags, int mode); 
	// <title GetFlags>
	// Syntax: Entity.GetFlags()
	int GetFlags(IFunctionHandler *pH);
	// <title HasFlags>
	// Syntax: Entity.HasFlags( int flags )
	int HasFlags(IFunctionHandler *pH, int flags);

	// <title GetArchetype>
	// Syntax: Entity.GetArchetype()
	// Description:
	//    Retrieve the archetype of the entity.
	// Return:
	//    name of entity archetype, nil if no archetype.
	int GetArchetype(IFunctionHandler *pH);

	// <title IntersectRay>
	// Syntax: Entity.IntersectRay( int slot, Vec3 rayOrigin, Vec3 rayDir, float maxDistance )
	int IntersectRay(IFunctionHandler *pH, int slot, Vec3 rayOrigin, Vec3 rayDir, float maxDistance);

	//////////////////////////////////////////////////////////////////////////
	// Attachments
	//////////////////////////////////////////////////////////////////////////

	// <title AttachChild>
	// Syntax: Entity.AttachChild( ScriptHandle childEntityId, int flags )
	int AttachChild(IFunctionHandler *pH, ScriptHandle childEntityId, int flags);
	// <title DetachThis>
	// Syntax: Entity.DetachThis()
	int DetachThis(IFunctionHandler *pH);
	// <title DetachAll>
	// Syntax: Entity.DetachAll()
	int DetachAll(IFunctionHandler *pH);
	// <title GetParent>
	// Syntax: Entity.GetParent()
	int GetParent(IFunctionHandler *pH);
	// <title GetChildCount>
	// Syntax: Entity.GetChildCount()
	int GetChildCount(IFunctionHandler *pH);
	// <title GetChild>
	// Syntax: Entity.GetChild( int nIndex )
	int GetChild(IFunctionHandler *pH,int nIndex);
	
	// <title EnableInheritXForm>
	// Syntax: Entity.EnableInheritXForm()
	// Description:
	//    Enables/Disable entity from inheriting transformation from the parent.
	int EnableInheritXForm(IFunctionHandler *pH, bool bEnable);
	//////////////////////////////////////////////////////////////////////////

	// <title NetPresent>
	// Syntax: Entity.NetPresent()
	int NetPresent(IFunctionHandler *pH);
	// <title RenderShadow>
	// Syntax: Entity.RenderShadow()
	int RenderShadow(IFunctionHandler *pH);
	// <title SetRegisterInSectors>
	// Syntax: Entity.SetRegisterInSectors()
	int SetRegisterInSectors(IFunctionHandler *pH);
	
	// <title IsColliding>
	// Syntax: Entity.IsColliding()
	int IsColliding(IFunctionHandler *pH);
	// <title GetDirectionVector>
	// Syntax: Entity.GetDirectionVector()
	int GetDirectionVector(IFunctionHandler *pH);
	// <title SetDirectionVector>
	// Syntax: Entity.SetDirectionVector()
	int SetDirectionVector(IFunctionHandler *pH, Vec3 dir);
	// <title IsAnimationRunning>
	// Syntax: Entity.IsAnimationRunning( int characterSlot, int layer )
	int IsAnimationRunning(IFunctionHandler *pH, int characterSlot, int layer);
	// <title AddImpulse>
	// Syntax: Entity.AddImpulse()
	int AddImpulse(IFunctionHandler *pH);
	// <title AddConstraint>
	// Syntax: Entity.AddConstraint()
	int AddConstraint(IFunctionHandler *pH);

	// <title SetPublicParam>
	// Syntax: Entity.SetPublicParam()
	int SetPublicParam(IFunctionHandler *pH);

	// <title PlaySound>
	// Syntax: Entity.PlaySound( Handle SoundId )
	// Description:
	//    Start playing the specified sound and attach it to the entity.
	//    Sound will move and rotate with the entity.
	// Arguments:
	//    SoundId - Sound Id previously loaded with Sound.LoadSound function.
	// Return:
	//    Return not a nil value if sound started to play successfully.
	//    nil if sound failed to play.
	int PlaySound(IFunctionHandler *pH);

	// <title PlaySoundEvent>
	// Syntax: Entity.PlaySoundEvent( const char *sSoundOrEventName, Vec3 vOffset, Vec3 vDirection, uint32 nSoundFlags, uint32 nSemantic )
	// Description:
	//		First pass impl. and docu.
	int PlaySoundEvent(IFunctionHandler *pH, const char *sSoundOrEventName, Vec3 vOffset, Vec3 vDirection, uint32 nSoundFlags, uint32 nSemantic);

	// <title PlaySoundEx>
	// Syntax: Entity.PlaySoundEx( Handle SoundId )
	// Description:
	//    Extended function for playing sounds.
	//    Start playing the specified sound and attach it to the entity with specified 
	//    offset and direction in local entity space.
	//    Sound will move and rotate with the entity.
	// Arguments:
	//    SoundId - Sound Id previously loaded with Sound.LoadSound function.
	//    vOffset - Sound Offset from the entity position.
	//    vDir    - Sound Direction relative to the default entity direction.
	//    fSoundScale - Sound scale, multiplier for sound volume.
	// Return:
	//    Return not a nil value if sound started to play successfully.
	//    nil if sound failed to play.
	//int PlaySoundEx(IFunctionHandler *pH,ScriptHandle nSoundId,Vec3 vOffset,Vec3 vDir,float fSoundScale);

	// <title PlaySoundEx>
	// Syntax: Entity.PlaySoundEx( const char *sSoundFilename,int nSoundFlags,float fVolume,Vec3 pos,float minRadius,float maxRadius )
	// Description:
	//    Play sound file attached to the entity.
	// Arguments:
	//    sSoundFilename - Name of the sound file to play.
	//    vPos - Position where to play sound.
	//    nSoundFlags - Sound flags, specify how to play sound.
	//    vOffset - Sound Offset from the entity position.
	//    nSemantic - Semantical information what this sound is.
	// Return:
	//    Sound ID, or nil if failed to play sound.
	int PlaySoundEventEx( IFunctionHandler *pH,const char *sSoundOrEventName, uint32 nSoundFlags, float fVolume, Vec3 vOffset, float minRadius, float maxRadius, uint32 nSemantic );

	// <title SetSoundSphereSpec>
	// Syntax: Entity.SetSoundSphereSpec( soundId, float radius )
	int SetSoundSphereSpec( IFunctionHandler *pH, ScriptHandle soundId, float radius);

	// <title SetStaticSound>
	// Syntax: Entity.SetStaticSound( Handle SoundId, true/false )
	// Description:
	//    Marks a sound as static to a certain entity. This sound will not be deleted on done, but can be started a new.
	// Arguments:
	//    SoundId - Sound Id.
	//    bStatic - boolean
	int SetStaticSound(IFunctionHandler *pH, ScriptHandle nSoundId, bool bStatic);

	// <title StopSound>
	// Syntax: Entity.StopSound( Handle SoundId )
	// Description:
	//    Stop sound played on the entity.
	//    Only sound started for this entity will be stopped.
	// Arguments:
	//    SoundId - Sound Id.
	int StopSound(IFunctionHandler *pH);

	// <title StopAllSounds>
	// Syntax: Entity.StopAllSounds()
	// Description:
	//    Stop all sounds played on the entity.
	int StopAllSounds(IFunctionHandler *pH);

	// <title PauseSound>
	// Syntax: Entity.PauseSound( Handle SoundId )
	// Description:
	//    Pauses/Unpauses playing sound.
	// Arguments:
	//    SoundId - Sound Id.
	//    bPause - true to pause sound, false to unpause sound.
	int PauseSound(IFunctionHandler *pH,ScriptHandle nSoundId,bool bPause);

	// <title SetSoundEffectRadius>
	// syntax: Entity.SetSoundEffectRadius( float fEffectRadius)
	// Description:
	//   Sets the effect radius for a sound entity (e.g. volume ambient sound, reverb preset)
	// Arguments:
	//   fEffectRadius - radius of when the effect will be faded in
	int SetSoundEffectRadius(IFunctionHandler *pH,float fEffectRadius);

	// <title StartAnimation>
	// syntax: Entity.StartAnimation()
	int StartAnimation(IFunctionHandler *pH);
	// <title StopAnimation>
	// syntax: Entity.StopAnimation( int characterSlot, int layer )
	int StopAnimation(IFunctionHandler *pH, int characterSlot, int layer);
	// <title ResetAnimation>
	// syntax: Entity.ResetAnimation( int characterSlot, int layer )
	int ResetAnimation(IFunctionHandler *pH, int characterSlot, int layer);
	// <title RedirectAnimationToLayer0>
	// syntax: Entity.RedirectAnimationToLayer0( int characterSlot, bool redirect )
	int RedirectAnimationToLayer0(IFunctionHandler *pH, int characterSlot, bool redirect);
	// <title SetAnimationBlendOut>
	// syntax: Entity.SetAnimationBlendOut( int characterSlot, int layer, float blendOut )
	int SetAnimationBlendOut(IFunctionHandler *pH, int characterSlot, int layer, float blendOut);

	// <title EnableBoneAnimation>
	// syntax: Entity.EnableBoneAnimation( int characterSlot, int layer, const char *boneName, bool status )
	int EnableBoneAnimation(IFunctionHandler *pH, int characterSlot, int layer, const char *boneName, bool status);
	// <title EnableBoneAnimationAll>
	// syntax: Entity.EnableBoneAnimationAll( int characterSlot, int layer, bool status )
	int EnableBoneAnimationAll(IFunctionHandler *pH, int characterSlot, int layer, bool status);

	// <title EnableProceduralFacialAnimation>
	// syntax: Entity.EnableProceduralFacialAnimation( bool enable )
	int EnableProceduralFacialAnimation(IFunctionHandler *pH, bool enable);
	// <title PlayFacialAnimation>
	// syntax: Entity.PlayFacialAnimation( char* name, bool looping )
	int PlayFacialAnimation(IFunctionHandler *pH, char* name, bool looping);

	// <title SetAnimationEvent>
	// syntax: Entity.SetAnimationEvent( int nSlot,const char *sAnimation )
	int SetAnimationEvent(IFunctionHandler *pH,int nSlot,const char *sAnimation);
	//int SetAnimationKeyEvent(IFunctionHandler *pH,int nSlot,const char *sAnimation,int nFrameID,const char *sEvent);
	// <title SetAnimationKeyEvent>
	// syntax: Entity.SetAnimationKeyEvent()
	int SetAnimationKeyEvent(IFunctionHandler *pH);
	// <title DisableAnimationEvent>
	// syntax: Entity.DisableAnimationEvent( int nSlot,const char *sAnimation )
	int DisableAnimationEvent(IFunctionHandler *pH,int nSlot,const char *sAnimation);

	// <title SetAnimationSpeed>
	// syntax: Entity.SetAnimationSpeed( int characterSlot, int layer, float speed )
	int SetAnimationSpeed(IFunctionHandler *pH,int characterSlot, int layer, float speed);
	// <title SetAnimationTime>
	// syntax: Entity.SetAnimationTime( int nSlot,int nLayer,float fTime )
	int SetAnimationTime(IFunctionHandler *pH,int nSlot,int nLayer,float fTime);
	// <title GetAnimationTime>
	// syntax: Entity.GetAnimationTime( int nSlot,int nLayer )
	int GetAnimationTime(IFunctionHandler *pH,int nSlot,int nLayer);

	// <title GetCurAnimation>
	// syntax: Entity.GetCurAnimation()
	int GetCurAnimation(IFunctionHandler *pH);
	// <title GetAnimationLength>
	// syntax: Entity.GetAnimationLength( int characterSlot, const char *animation )
	int GetAnimationLength(IFunctionHandler *pH, int characterSlot, const char *animation);
	// <title SetAnimationFlip>
	// syntax: Entity.SetAnimationFlip( int characterSlot, Vec3 flip )
	int SetAnimationFlip(IFunctionHandler *pH, int characterSlot, Vec3 flip);
	
	// <title SetTimer>
	// syntax: Entity.SetTimer()
	int SetTimer(IFunctionHandler *pH);
	// <title KillTimer>
	// syntax: Entity.KillTimer()
	int KillTimer(IFunctionHandler *pH);
	// <title SetScriptUpdateRate>
	// syntax: Entity.SetScriptUpdateRate( int nMillis )
	int SetScriptUpdateRate(IFunctionHandler *pH,int nMillis);
	
	//////////////////////////////////////////////////////////////////////////
	// State management.
	//////////////////////////////////////////////////////////////////////////

	// <title GotoState>
	// syntax: Entity.GotoState( const char *sState )
	int GotoState(IFunctionHandler *pH,const char *sState);
	// <title IsInState>
	// syntax: Entity.IsInState( const char *sState )
	int IsInState(IFunctionHandler *pH,const char *sState);
	// <title GetState>
	// syntax: Entity.GetState()
	int GetState(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////

	// <title IsHidden>
	// syntax: Entity.IsHidden()
	int	IsHidden(IFunctionHandler *pH);
	// <title GetTouchedSurfaceID>
	// syntax: Entity.GetTouchedSurfaceID()
	int GetTouchedSurfaceID(IFunctionHandler *pH);
	// <title GetTouchedPoint>
	// syntax: Entity.GetTouchedPoint()
	// Description:
	//		Retrieves point of collision for rigid body.
	int GetTouchedPoint(IFunctionHandler *pH);

	// <title CreateBoneAttachment>
	// syntax: Entity.CreateBoneAttachment( int characterSlot, const char *boneName, const char *attachmentName )
	int CreateBoneAttachment(IFunctionHandler *pH, int characterSlot, const char *boneName, const char *attachmentName);
	// <title CreateSkinAttachment>
	// syntax: Entity.CreateSkinAttachment( int characterSlot, const char *attachmentName )
	int CreateSkinAttachment(IFunctionHandler *pH, int characterSlot, const char *attachmentName);
	// <title DestroyAttachment>
	// syntax: Entity.DestroyAttachment( int characterSlot, const char *attachmentName )
	int DestroyAttachment(IFunctionHandler *pH, int characterSlot, const char *attachmentName);
	// <title GetAttachmentBone>
	// syntax: Entity.GetAttachmentBone( int characterSlot, const char *attachmentName )
	int GetAttachmentBone(IFunctionHandler *pH, int characterSlot, const char *attachmentName);
	// <title GetAttachmentCGF>
	// syntax: Entity.GetAttachmentCGF( int characterSlot, const char *attachmentName )
	int GetAttachmentCGF(IFunctionHandler *pH, int characterSlot, const char *attachmentName);
	// <title ResetAttachment>
	// syntax: Entity.ResetAttachment( int characterSlot, const char *attachmentName )
	int ResetAttachment(IFunctionHandler *pH, int characterSlot, const char *attachmentName);
	// <title SetAttachmentEffect>
	// syntax: Entity.SetAttachmentEffect( int characterSlot, const char *attachmentName, const char *effectName, Vec3 offset, Vec3 dir, float scale, int flags )
	int SetAttachmentEffect(IFunctionHandler *pH, int characterSlot, const char *attachmentName, const char *effectName, Vec3 offset, Vec3 dir, float scale, int flags);
	// <title SetAttachmentObject>
	// syntax: Entity.SetAttachmentObject( int characterSlot, const char *attachmentName, ScriptHandle entityId, int slot, int flags )
	int SetAttachmentObject(IFunctionHandler *pH, int characterSlot, const char *attachmentName, ScriptHandle entityId, int slot, int flags);
	// <title SetAttachmentCGF>
	// syntax: Entity.SetAttachmentCGF( int characterSlot, const char *attachmentName, const char* filePath )
	int SetAttachmentCGF(IFunctionHandler *pH, int characterSlot, const char *attachmentName, const char* filePath);
	// <title SetAttachmentLight>
	// syntax: Entity.SetAttachmentLight( int characterSlot, const char *attachmentName, SmartScriptTable lightTable, int flags )
	int SetAttachmentLight(IFunctionHandler *pH, int characterSlot, const char *attachmentName, SmartScriptTable lightTable, int flags);
	// <title SetAttachmentPos>
	// syntax: Entity.SetAttachmentPos( int characterSlot, const char *attachmentName, Vec3 pos )
	int SetAttachmentPos(IFunctionHandler *pH, int characterSlot, const char *attachmentName, Vec3 pos);
	// <title SetAttachmentAngles>
	// syntax: Entity.SetAttachmentAngles( int characterSlot, const char *attachmentName, Vec3 angles )
	int SetAttachmentAngles(IFunctionHandler *pH, int characterSlot, const char *attachmentName, Vec3 angles);
	// <title SetAttachmentDir>
	// syntax: Entity.SetAttachmentDir()
	int SetAttachmentDir(IFunctionHandler *pH, int characterSlot, const char *attachmentName, Vec3 dir, bool worldSpace);
	// <title HideAttachment>
	// syntax: Entity.HideAttachment( int characterSlot, const char *attachmentName, bool hide, bool hideShadow )
	int HideAttachment(IFunctionHandler *pH, int characterSlot, const char *attachmentName, bool hide, bool hideShadow);
	// <title HideAllAttachments>
	// syntax: Entity.HideAllAttachments( int characterSlot, bool hide, bool hideShadow )
	int HideAllAttachments(IFunctionHandler *pH, int characterSlot, bool hide, bool hideShadow);
	// <title HideAttachmentMaster>
	// syntax: Entity.HideAttachmentMaster( int characterSlot, bool hide )
	int HideAttachmentMaster(IFunctionHandler *pH, int characterSlot, bool hide);

	// <title PhysicalizeAttachment>
	// syntax: Entity.PhysicalizeAttachment( int characterSlot, const char* attachmentName, bool physicalize )
	int PhysicalizeAttachment(IFunctionHandler *pH, int characterSlot, const char* attachmentName, bool physicalize);

	// <title Damage>
	// syntax: Entity.Damage()
	int Damage(IFunctionHandler *pH);

	// <title GetEntitiesInContact>
	// syntax: Entity.GetEntitiesInContact()
	int GetEntitiesInContact(IFunctionHandler *pH);
	// <title GetExplosionObstruction>
	// syntax: Entity.GetExplosionObstruction()
	int GetExplosionObstruction(IFunctionHandler *pH);
	// <title GetExplosionImpulse>
	// syntax: Entity.GetExplosionImpulse()
	int GetExplosionImpulse(IFunctionHandler *pH);
	// <title SetMaterial>
	// syntax: Entity.SetMaterial()
	int SetMaterial(IFunctionHandler *pH);
	// <title GetMaterial>
	// syntax: Entity.GetMaterial()
	int GetMaterial(IFunctionHandler *pH);
  
	// <title ReplaceMaterial>
	// syntax: Entity.ReplaceMaterial( int slot, const char *name, const char *replacement )
	int ReplaceMaterial(IFunctionHandler *pH, int slot, const char *name, const char *replacement);
	// <title ResetMaterial>
	// syntax: Entity.ResetMaterial( int slot )
	int ResetMaterial(IFunctionHandler *pH, int slot);
/*	int AddMaterialLayer(IFunctionHandler *pH, int slotId, const char *shader);
	int RemoveMaterialLayer(IFunctionHandler *pH, int slotId, int id);
	int RemoveAllMaterialLayers(IFunctionHandler *pH, int slotId);
	int SetMaterialLayerParamF(IFunctionHandler *pH, int slotId, int layerId, const char *name, float value);
	int SetMaterialLayerParamV(IFunctionHandler *pH, int slotId, int layerId, const char *name, Vec3 vec);
	int SetMaterialLayerParamC(IFunctionHandler *pH, int slotId, int layerId, const char *name,
		float r, float g, float b, float a);
		*/
	// <title EnableMaterialLayer>
	// syntax: Entity.EnableMaterialLayer( bool enable, int layer )
	int EnableMaterialLayer(IFunctionHandler *pH, bool enable, int layer);


	// <title CloneMaterial>
	// Syntax: Entity.CloneMaterial( int nSlotId,string sSubMaterialName )
	// Description:
	//    Replace material on the slot with a cloned version of the material.
	//    Cloned material can be freely changed uniquely for this entity.
	// Arguments:
	//    nSlotId - On which slot to clone material.
	//    sSubMaterialName - if non empty string only this specific sub-material is cloned.
	int CloneMaterial(IFunctionHandler *pH, int slot );
	
	// <title SetMaterialFloat>
	// Syntax: Entity.SetMaterialFloat( int nSlotId,sParamName,fValue )
	// Description:
	//    Change material parameter.
	// Arguments:
	//    nSlot - On which slot to change material.
	//    sParamName - Name of the material parameter.
	//    fValue - New material parameter value.
	int SetMaterialFloat( IFunctionHandler *pH, int slot,int nSubMtlId,const char *sParamName,float fValue );

	// <title GetMaterialFloat>
	// Syntax: Entity.GetMaterialFloat( int nSlotId,sParamName )
	// Description:
	//    Change material parameter.
	// Arguments:
	//    nSlot - On which slot to change material.
	//    sParamName - Name of the material parameter.
	// Returns
	//    Material parameter value.
	int GetMaterialFloat( IFunctionHandler *pH, int slot,int nSubMtlId,const char *sParamName );

	// <title SetMaterialVec3>
	// Syntax: Entity.SetMaterialVec3( int nSlotId,sParamName,vVec3 )
	// See Also:
	//    SetMaterialFloat
	int SetMaterialVec3( IFunctionHandler *pH, int slot,int nSubMtlId,const char *sParamName,Vec3 fValue );

	// <title GetMaterialFloat>
	// Syntax: Entity.GetMaterialVec3( int nSlotId,sParamName )
	// See Also:
	//    GetMaterialFloat
	int GetMaterialVec3( IFunctionHandler *pH, int slot,int nSubMtlId,const char *sParamName );

	// <title MaterialFlashInvoke>
	// Syntax: Entity.MaterialFlashInvoke()
	int MaterialFlashInvoke(IFunctionHandler *pH);

	// <title ToLocal>
	// Syntax: Entity.ToLocal( int slotId, Vec3 point )
	int ToLocal(IFunctionHandler *pH, int slotId, Vec3 point);
	// <title ToGlobal>
	// Syntax: Entity.ToGlobal( int slotId, Vec3 point )
	int ToGlobal(IFunctionHandler *pH, int slotId, Vec3 point);
	// <title VectorToLocal>
	// Syntax: Entity.VectorToLocal( int slotId, Vec3 dir )
	int VectorToLocal(IFunctionHandler *pH, int slotId, Vec3 dir);
	// <title VectorToGlobal>
	// Syntax: Entity.VectorToGlobal( int slotId, Vec3 dir )
	int VectorToGlobal(IFunctionHandler *pH, int slotId, Vec3 dir);

	// <title CheckCollisions>
	// Syntax: Entity.CheckCollisions()
	int CheckCollisions(IFunctionHandler *pH);
	// <title AwakeEnvironment>
	// Syntax: Entity.AwakeEnvironment()
	int AwakeEnvironment(IFunctionHandler *pH);

	// <title GetViewDistRatio>
	// Syntax: Entity.GetViewDistRatio()
	int GetViewDistRatio(IFunctionHandler *pH);
	// <title SetViewDistRatio>
	// Syntax: Entity.SetViewDistRatio()
	int SetViewDistRatio(IFunctionHandler *pH);
	// <title SetViewDistUnlimited>
	// Syntax: Entity.SetViewDistUnlimited()
	int SetViewDistUnlimited(IFunctionHandler *pH);
	// <title SetLodRatio>
	// Syntax: Entity.SetLodRatio()
	int SetLodRatio(IFunctionHandler *pH);
	// <title GetLodRatio>
	// Syntax: Entity.GetLodRatio()
	int GetLodRatio(IFunctionHandler *pH);
	// <title SetStateClientside>
	// Syntax: Entity.SetStateClientside()
	int SetStateClientside(IFunctionHandler *pH);
	// <title InsertSubpipe>
	// Syntax: Entity.InsertSubpipe()
	int InsertSubpipe(IFunctionHandler * pH);
	// <title CancelSubpipe>
	// Syntax: Entity.CancelSubpipe()
	int CancelSubpipe(IFunctionHandler * pH);
	// <title SetDefaultIdleAnimations>
	// Syntax: Entity.SetDefaultIdleAnimations()
	int SetDefaultIdleAnimations(IFunctionHandler *pH);
	// <title GetVelocity>
	// Syntax: Entity.GetVelocity()
	int GetVelocity(IFunctionHandler *pH);
	// <title GetSpeed>
	// Syntax: Entity.GetSpeed()
	int GetSpeed(IFunctionHandler *pH);
	// <title GetMass>
	// Syntax: Entity.GetMass()
	int GetMass(IFunctionHandler *pH);
	// <title GetVolume>
	// Syntax: Entity.GetVolume( int slot)
	int GetVolume(IFunctionHandler *pH, int slot);
	// <title GetGravity>
	// Syntax: Entity.GetGravity()
	int GetGravity(IFunctionHandler *pH);
	// <title GetSubmergedVolume>
	// Syntax: Entity.GetSubmergedVolume( int slot, Vec3 planeNormal, Vec3 planeOrigin )
	int GetSubmergedVolume(IFunctionHandler *pH, int slot, Vec3 planeNormal, Vec3 planeOrigin);

	// <title CreateLink>
	// Syntax: Entity.CreateLink( const char *name )
	int CreateLink(IFunctionHandler *pH, const char *name);
	// <title GetLinkName>
	// Syntax: Entity.GetLinkName( targetId )
	int GetLinkName(IFunctionHandler *pH, ScriptHandle targetId);
	// <title SetLinkTarget>
	// Syntax: Entity.SetLinkTarget( const char *name, targetId)
	int SetLinkTarget(IFunctionHandler *pH, const char *name, ScriptHandle targetId);
	// <title GetLinkTarget>
	// Syntax: Entity.GetLinkTarget( const char *name )
	int GetLinkTarget(IFunctionHandler *pH, const char *name);
	// <title RemoveLink>
	// Syntax: Entity.RemoveLink( const char *name )
	int RemoveLink(IFunctionHandler *pH, const char *name);
	// <title RemoveAllLinks>
	// Syntax: Entity.RemoveAllLinks()
	int RemoveAllLinks(IFunctionHandler *pH);
	// <title GetLink>
	// Syntax: Entity.GetLink()
	int GetLink(IFunctionHandler *pH, int ith);
	// <title CountLinks>
	// Syntax: Entity.CountLinks()
	int	CountLinks(IFunctionHandler *pH);

	// <title RemoveDecals>
	// Syntax: Entity.RemoveDecals()
	int RemoveDecals(IFunctionHandler * pH);
	// <title EnableDecals>
	// Syntax: Entity.EnableDecals( int slot, bool enable )
	int EnableDecals(IFunctionHandler *pH, int slot, bool enable);
	// <title ForceCharacterUpdate>
	// Syntax: Entity.ForceCharacterUpdate( int characterSlot, bool updateAlways )
	int ForceCharacterUpdate(IFunctionHandler * pH, int characterSlot, bool updateAlways);
	// <title CharacterUpdateAlways>
	// Syntax: Entity.CharacterUpdateAlways( int characterSlot, bool updateAlways )
	int CharacterUpdateAlways(IFunctionHandler * pH, int characterSlot, bool updateAlways);
	// <title CharacterUpdateOnRender>
	// Syntax: Entity.CharacterUpdateOnRender( int characterSlot, bool bUpdateOnRender )
	int CharacterUpdateOnRender(IFunctionHandler * pH, int characterSlot, bool bUpdateOnRender);
	// <title RagDollize>
	// Syntax: Entity.RagDollize(int slot)
	int RagDollize(IFunctionHandler* pH, int slot);
	// <title Hide>
	// Syntax: Entity.Hide()
	int Hide(IFunctionHandler * pH);
	// <title NoExplosionCollision>
	// Syntax: Entity.NoExplosionCollision()
	int NoExplosionCollision(IFunctionHandler * pH);
	// <title NoBulletForce>
	// Syntax: Entity.NoBulletForce( bool state )
	int NoBulletForce(IFunctionHandler * pH,bool state);

	// <title UpdateAreas>
	// Syntax: Entity.UpdateAreas()
	int UpdateAreas(IFunctionHandler * pH);
	// <title IsPointInsideArea>
	// Syntax: Entity.IsPointInsideArea( int areaId, Vec3 point )
	int IsPointInsideArea(IFunctionHandler * pH, int areaId, Vec3 point);
	// <title IsEntityInsideArea>
	// Syntax: Entity.IsEntityInsideArea( int areaId, ScriptHandle entityId )
	int IsEntityInsideArea(IFunctionHandler * pH, int areaId, ScriptHandle entityId);

	// <title GetPhysicalStats>
	// Syntax: Entity.GetPhysicalStats()
	// Description:
	//		Some more physics related.
	int GetPhysicalStats(IFunctionHandler *pH);

	// <title SetParentSlot>
	// Syntax: Entity.SetParentSlot( int child, int parent )
	int SetParentSlot(IFunctionHandler * pH, int child, int parent);
	// <title GetParentSlot>
	// Syntax: Entity.GetParentSlot( int child )
	int GetParentSlot(IFunctionHandler* pH, int child);
	
	// <title BreakToPieces>
	// Syntax: Entity.BreakToPieces()
	// Description:
	//    Breaks static geometry in slot 0 into sub objects and spawn them as particles or entities.
	int BreakToPieces(IFunctionHandler* pH, int nSlot, int nPiecesSlot, float fExplodeImp, Vec3 vHitPt, Vec3 vHitImp, float fLifeTime, bool bSurfaceEffects);
	// <title AttachSurfaceEffect>
	// Syntax: Entity.AttachSurfaceEffect( int nSlot, const char *effect, bool countPerUnit, const char *form, const char *typ, float countScale, float sizeScale )
	int AttachSurfaceEffect(IFunctionHandler* pH, int nSlot, const char *effect, bool countPerUnit, const char *form, const char *typ, float countScale, float sizeScale);

	//////////////////////////////////////////////////////////////////////////
	// This method is for engine internal usage.

	// <title ProcessBroadcastEvent>
	// Syntax: Entity.ProcessBroadcastEvent()
	int ProcessBroadcastEvent(IFunctionHandler* pH);
	// <title ActivateOutput>
	// Syntax: Entity.ActivateOutput()
	int ActivateOutput(IFunctionHandler* pH);
	
	// <title CreateCameraProxy>
	// Syntax: Entity.CreateCameraProxy()
	// Description:
	//    Create a proxy camera object for the entity, allows entity to serve as camera source for material assigned on the entity.
	int CreateCameraProxy(IFunctionHandler* pH);

	// <title UnSeenFrames>
	// Syntax: Entity.UnSeenFrames()
	int UnSeenFrames(IFunctionHandler *pH);

	// <title DeleteParticleEmitter>
	// Syntax: Entity.DeleteParticleEmitter(slot)
	// Arguments:
	//	  slot - .
	// Description:
	//	  Deletes particles emitter from 3dengine.
	int DeleteParticleEmitter(IFunctionHandler *pH, int slot);

private: // -------------------------------------------------------------------------------
	
	// Helper function to get IEntity pointer from IFunctionHandler
	IEntity* GetEntity( IFunctionHandler *pH );
	IEntityProxy* GetOrMakeProxy( IEntity *pEntity,EEntityProxy proxyType );
	Vec3 GetGlobalGravity() const { return Vec3(0,0,-9.81f); }
	int SetEntityPhysicParams( IFunctionHandler *pH, IPhysicalEntity *pe,int type,IScriptTable *pTable, ICharacterInstance *pIChar=0);
	EntityId GetEntityID( IScriptTable *pEntityTable );
	ISound * GetSoundPtr( IFunctionHandler *pH, int arg );

	IEntitySystem *m_pEntitySystem;
	ISystem *m_pISystem;
	ISoundSystem *m_pSoundSystem;

	// copy of function from ScriptObjectParticle
	bool ReadParticleTable(IScriptTable *pITable, struct ParticleParams &sParamOut);
	
	bool ParseLightParams(IScriptTable *pLightTable, IEntity *pEntity, CDLight &light, bool* pHasViewDistRatio = NULL, int* pViewDistRatio = NULL);
	bool ParseFogVolumesParams( IScriptTable* pTable, IEntity* pEntity, SFogVolumeProperties& properties );
	bool ParseCloudMovementProperties(IScriptTable* pTable, IEntity* pEntity, SCloudMovementProperties& properties);
	bool ParseVolumeObjectMovementProperties(IScriptTable* pTable, IEntity* pEntity, SVolumeObjectMovementProperties& properties);
	// Parse script table to the entity physical params table.
	bool ParsePhysicsParams( IScriptTable *pTable,SEntityPhysicalizeParams &params );

	typedef struct
	{
		Vec3 position;
		Vec3 v0, v1, v2;
		Vec2 uv0, uv1, uv2;
		Vec3 baricentric;
		float distance;
		bool backface;
		char material[256];

	} SIntersectionResult;

	static int __cdecl cmpIntersectionResult(const void *v1, const void *v2);
	int IStatObjRayIntersect(IStatObj *pStatObj, const Vec3 &rayOrigin, const Vec3 &rayDir, float maxDistance, SIntersectionResult *pOutResult, unsigned int maxResults);

	//////////////////////////////////////////////////////////////////////////
	// Structures used in Physicalize call.
	//////////////////////////////////////////////////////////////////////////
	pe_params_particle   m_particleParams;
	pe_params_buoyancy   m_buoyancyParams;
	pe_player_dimensions m_playerDimensions;
	pe_player_dynamics   m_playerDynamics;
	pe_params_area  m_areaGravityParams;
	SEntityPhysicalizeParams::AreaDefinition m_areaDefinition;
	std::vector<Vec3> m_areaPoints;
	pe_params_car m_carParams;

	//temp table used by GetPhysicalStats
	SmartScriptTable m_pStats;
};

#endif // __ScriptBind_Entity_h__
