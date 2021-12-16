#include "stdafx.h"
#include "ReplayLogDefs.h"

namespace MemStatContextTypes
{
	const char* ToString(Type type)
	{
		switch(type)
		{
		case MSC_MAX: return "MAX";
		case MSC_CGF: return "CGF";
		case MSC_MTL: return "MTL";
		case MSC_DBA: return "DBA";
		case MSC_CHR: return "CHR";
		case MSC_LMG: return "LMG";
		case MSC_AG: return "AG";
		case MSC_Texture: return "Texture";
		case MSC_ParticleLibrary: return "ParticleLibrary";

		case MSC_Physics: return "Physics";
		case MSC_Terrain: return "Terrain";
		case MSC_Shader: return "Shader";
		case MSC_Other: return "Other";
		case MSC_RenderMesh: return "RenderMesh";
		case MSC_Entity: return "Entity";
		case MSC_Navigation: return "Navigation";
		case MSC_ScriptCall: return "ScriptCall";

		case MSC_CDF: return "CDF";
		case MSC_RenderMeshType: return "RenderMeshType";
		
		case MSC_ANM: return "ANM";
		case MSC_CGA: return "CGA";
		case MSC_CAF: return "CAF";

		case MSC_ArchetypeLib: return "ArchetypeLib";

		case MSC_SoundProject: return "SoundProject";

		case MSC_LUA: return "LUA";
		case MSC_D3D: return "D3D";

		case MSC_ParticleEffect: return "ParticleEffect";
		case MSC_SoundBuffer: return "SoundBuffer";
		case MSC_FSB: return "FMOD FSB";
		default: return "Unknown";
		}
	}
}
