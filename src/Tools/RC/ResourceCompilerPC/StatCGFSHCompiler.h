#error Do not use it

#ifndef STAT_CGF_SH_COMPILER
#define STAT_CGF_SH_COMPILER

#include <map>

//forward declarations
class CMesh;
class CContentCGF;
struct CMaterialCGF;

class CSimpleIndexedMesh;

namespace NSH
{
	namespace NTransfer
	{
		struct STransferParameters;
	} 
	namespace NFramework
	{
		struct SMeshCompressedCoefficients;
	}
}

// Description: 
//    calculates sh coefficients for a certain mesh using PRT.lib
class CStatCFGSHCompiler
{
public:
	//constructor with all necessary elements(meant to be used in place)
	CStatCFGSHCompiler(CMesh *pMesh, CContentCGF *pCGF, const CMaterialCGF* cpMat) 
		: m_pMesh(pMesh)
		, m_pCGF(pCGF)
		, m_cpMat(cpMat)
	{
	}

	const bool Calculate();	//calculates the coefficients

private:	
	CMesh* m_pMesh;							 										//cached mesh pointer
	CContentCGF* m_pCGF;														//cached cgf pointer
	const CMaterialCGF* m_cpMat;										//cached cgf material pointer
	std::map<uint32, uint32> m_MatToSHIDindexMap;		//cgf-mat-id to prt-shader mapping

	const bool SetTransferParameters(NSH::NTransfer::STransferParameters& rParam);	//helper function, sets the parameter set for the sh process
	const bool CreateSHMaterials(CSimpleIndexedMesh& rSHMesh, NSH::NTransfer::STransferParameters& rTransferParams);			//helper function, creates the sh materials corresponding to cgf mats(exported via 3d Max MaterialNames)
	const bool CreateSHMesh(CSimpleIndexedMesh& rSHMesh);					//helper function, creates the sh mesh from pMesh
	const bool ExportSHValues(CSimpleIndexedMesh&, const NSH::NFramework::SMeshCompressedCoefficients&, const NSH::NTransfer::STransferParameters&);//helper function, exports the sh values from m_SHMesh into pMesh
	static const CMaterialCGF* GetMaterialFromID(const CMaterialCGF *cpParentMat, CContentCGF *cpCGF, const uint32 cMatID);
};

#endif