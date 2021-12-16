////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   ScriptBind_Particle.h
//  Version:     v1.00
//  Created:     8/7/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScriptBind_Particle_h__
#define __ScriptBind_Particle_h__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <IScriptSystem.h>

struct ISystem;
struct I3DEngine;
struct ParticleParams;
struct IParticleEffect;
struct CryEngineDecalInfo;

/*! 
	 <title Particle>
	 Syntax: Particle

	This class implements script-functions for particles and decals.
	REMARKS:
	After initialization of the script-object it will be globally accessable through scripts using the namespace "Particle".
	
	Example:
		Particle.CreateDecal(pos, normal, scale, lifetime, decal.texture, decal.object, rotation);

	IMPLEMENTATIONS NOTES:
	These function will never be called from C-Code. They're script-exclusive.
*/

class CScriptBind_Particle : public CScriptableBase
{
public:
	CScriptBind_Particle(IScriptSystem *pScriptSystem, ISystem *pSystem);
	virtual ~CScriptBind_Particle();
	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	// <title CreateEffect>
	// Syntax: Particle.CreateEffect(  const char *name, SmartScriptTable params )
	// Arguments:
	//		name	- Particle effect name.
	//		params	- Effect parameters.
	// Description:
	//		Creates a new particle effect.
	int CreateEffect(IFunctionHandler *pH, const char *name, SmartScriptTable params);
	// <title DeleteEffect>
	// Syntax: Particle.DeleteEffect( const char *name )
	// Arguments:
	//		name - Particle effect name.
	// Description:
	//		Deletes the specified particle effect.
	int DeleteEffect(IFunctionHandler *pH, const char *name);
	// <title IsEffectAvailable>
	// Syntax: Particle.IsEffectAvailable( const char *name )
	// Arguments:
	//		name - Particle effect name.
	// Description:
	//		Checks if the specified particle effect is available.
	int IsEffectAvailable(IFunctionHandler *pH, const char *name);
	// <title SpawnEffect>
	// Syntax: Particle.SpawnEffect( const char *effectName, Vec3 pos, Vec3 dir )
	// Arguments:
	//		effectName	- Effect name.
	//		pos			- Position vector.
	//		dir			- Direction vector.
	// Description:
	//		Spawns an effect.
	int SpawnEffect(IFunctionHandler *pH, const char *effectName, Vec3 pos, Vec3 dir);
	// <title SpawnEffectLine>
	// Syntax: Particle.SpawnEffectLine(  const char *effectName, Vec3 startPos, Vec3 endPos, Vec3 dir, float scale, int slices )
	// Arguments:
	//		effectName	- Effect name.
	//		startPos	- Start position.
	//		endPos		- End position.
	//		dir			- Direction of the effect.
	//		scale		- Scale value for the effect.
	//		slices		- Number of slices.
	// Description:
	//		Spawns an effect line.
	int SpawnEffectLine(IFunctionHandler *pH, const char *effectName, Vec3 startPos, Vec3 endPos, Vec3 dir, float scale, int slices);
	// <title SpawnParticles>
	// Syntax: Particle.SpawnParticles(  SmartScriptTable params, Vec3 pos, Vec3 dir )
	// Arguments:
	//		params	- Effect parameters.
	//		pos		- Effect position.
	//		dir		- Effect direction.
	// Description:
	//		Spawns a particle effect.
	int SpawnParticles(IFunctionHandler *pH, SmartScriptTable params, Vec3 pos, Vec3 dir);
	// <title CreateDecal>
	// Syntax: Particle.CreateDecal(  Vec3 pos, Vec3 normal, float size, float lifeTime, const char *textureName )
	// Arguments:
	//		pos			- Decal position.
	//		normal		- Decal normal vector.
	//		size		- Decal size.
	//		lifeTime	- Decal life time.
	//		textureName - Name of the texture.
	// Description:
	//		Creates a decal with the specified parameters.
	int CreateDecal(IFunctionHandler *pH, Vec3 pos, Vec3 normal, float size, float lifeTime, const char *textureName);
	// <title CreateMatDecal>
	// Syntax: Particle.CreateMatDecal(  Vec3 pos, Vec3 normal, float size, float lifeTime, const char *materialName )
	// Arguments:
	//		pos			 - Decal position.
	//		normal		 - Decal normal vector.
	//		size		 - Decal size.
	//		lifeTime	 - Decal life time.
	//		materialName - Name of the Material.
	// Description:
	//		Creates a material decal.
	int CreateMatDecal(IFunctionHandler *pH, Vec3 pos, Vec3 normal, float size, float lifeTime, const char *materialName);
	// <title Attach>
	// Syntax: Particle.Attach()
	// Description:
	//		Attaches an effect.
	int Attach(IFunctionHandler * pH);
	// <title Detach>
	// Syntax: Particle.Detach()
	// Description:
	//		Detaches an effect.
	int Detach(IFunctionHandler * pH);
  
private:
	void ReadParams(SmartScriptTable &table, ParticleParams *params, IParticleEffect *pEffect);
	void CreateDecalInternal( IFunctionHandler *pH, const Vec3& pos, const Vec3& normal, float size, float lifeTime, const char *name, bool nameIsMaterial );

	ISystem			*m_pSystem;
	I3DEngine		*m_p3DEngine;
};

#endif // __ScriptBind_Particle_h__
