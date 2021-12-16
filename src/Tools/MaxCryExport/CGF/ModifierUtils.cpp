#include "stdafx.h"

#include <iskin.h>
#include <iiksys.h>
#include "ModifierUtils.h"
#include "MorpherUtils.h"
#include "PhysiqueModifierSkinningInfoSource.h"
#include "SkinModifierSkinningInfoSource.h"
#include "ISkeleton.h"
#include "IBoneArray.h"
#include "IBone.h"
#include "NameList.h"

const double gPi = 3.1415926535897932384626433832795;


Matrix3 Uniform_Matrix(Matrix3 orig_cur_mat)
{          
  AffineParts   parts;  
  Matrix3       mat;   

///Remove  scaling  from orig_cur_mat
//1) Decompose original and get decomposition info
  decomp_affine(orig_cur_mat, &parts); 

//2) construct 3x3 rotation from quaternion parts.q
  parts.q.MakeMatrix(mat);

//3) construct position row from translation parts.t  
  mat.SetRow(3,  parts.t);

  return(mat);
}

MorphR3* FindMorpherModifier (INode* pNode)
{
	if (!pNode)
		return NULL;
	Object* pObjectRef = pNode->GetObjectRef();
	if (!pObjectRef)
	{
		DebugPrint ("Node %s doesn't have object reference\n", pNode->GetName());
		return NULL;
	}

	if (pObjectRef->SuperClassID() != GEN_DERIVOB_CLASS_ID)
	{
		DebugPrint("not a derived object.\n");
		return NULL;
	}

	DebugPrint("\nModifier stack for \"%s\"", (const char*)pNode->GetName());

	do {
		IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObjectRef);
		int numModifiers = pDerivedObject->NumModifiers();
		DebugPrint(" %d modifiers:\n", numModifiers);
		for (int nModifier = 0; nModifier < numModifiers; ++nModifier)
		{
			Modifier* pModifier = pDerivedObject->GetModifier(nModifier);
			Class_ID classId = pModifier->ClassID();
			DebugPrint ("%3d. \"%s\" ClassID(%08x,%08x)\n", nModifier, pModifier->GetName(), classId.PartA(), classId.PartB());
			if (classId == MR3_CLASS_ID)
			{
				MorphR3* pMorpher = static_cast<MorphR3*>(pModifier);
				return pMorpher;
			}
		}
		pObjectRef = pDerivedObject->GetObjRef();
	} while(pObjectRef && pObjectRef->SuperClassID() == GEN_DERIVOB_CLASS_ID);
	
	return NULL;
}


void PrintDebugModifiers (INode* pNode)
{
	if (!pNode)
		return;
	Object* pObjectRef = pNode->GetObjectRef();
	if (!pObjectRef)
	{
		DebugPrint ("Node %s doesn't have object reference\n", pNode->GetName());
		return;
	}

	DebugPrint("Modifier info for node %s:\n", pNode->GetName());

	if (pObjectRef->SuperClassID() != GEN_DERIVOB_CLASS_ID)
	{
		DebugPrint("not a derived object.\n");
		return;
	}

	IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObjectRef);
	int numModifiers = pDerivedObject->NumModifiers();
	for (int nModifier = 0; nModifier < numModifiers; ++nModifier)
	{
		Modifier* pModifier = pDerivedObject->GetModifier(nModifier);
		Class_ID classId = pModifier->ClassID();
		DebugPrint ("%d. \"%s\" ClassID(%08x,%08x)", nModifier, pModifier->GetName(), classId.PartA(), classId.PartB());
		if (classId == MR3_CLASS_ID)
		{
			DebugPrint(" MR3_CLASS_ID\n");

			MorphR3* pMorpher = static_cast<MorphR3*>(pModifier);
			for (int nChannel = 0; nChannel < g_nMaxMorphChannels; ++nChannel)
			{
				morphChannel& rChannel = pMorpher->chanBank[nChannel];
				if (!rChannel.mActive || rChannel.mInvalid || !rChannel.mActiveOverride)
					continue;
				DebugPrint("%4d. \"%s\", node ", nChannel, rChannel.mName);
				if (rChannel.mConnection)
					DebugPrint("\"%s\"", rChannel.mConnection->GetName());
				else
					DebugPrint("NULL");
				DebugPrint("\n");
			}
		}
		DebugPrint("\n");

	}
}

// returns the reference to the (sometimes base) object before the physique, skin or morph modifier(s)
Object* GetObjectBeforeSkin (INode* pNode)
{
	if (!pNode)
		return NULL;
	
	Object* pObjectRef = pNode->GetObjectRef();
	while (pObjectRef && pObjectRef->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObjectRef);
		int numModifiers = pDerivedObject->NumModifiers();
		DebugPrint(" %d modifiers:\n", numModifiers);
		bool bSkinModifierFound = false;
		for (int nModifier = 0; nModifier < numModifiers; ++nModifier)
		{
			Modifier* pModifier = pDerivedObject->GetModifier(nModifier);
			Class_ID classId = pModifier->ClassID();
			DebugPrint ("%3d. \"%s\" ClassID(%08x,%08x)\n", nModifier, pModifier->GetName(), classId.PartA(), classId.PartB());
			if (classId == MR3_CLASS_ID
				||classId == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B)
				||classId == SKIN_CLASSID)
			{
				bSkinModifierFound = true;
				break;
			}
		}
		if (!bSkinModifierFound)
			break; // this is the base object we were looking for
		pObjectRef = pDerivedObject->GetObjRef(); // prepare to scan the next object
	};

	return pObjectRef;
}

// returns the enabled skin and morpher modifiers
void GetSkinModifiers (INode* pNode, Tab<Modifier*>& arrModifiers)
{
	arrModifiers.Resize(0);

	if (!pNode)
		return;
	
	Object* pObjectRef = pNode->GetObjectRef();
	while (pObjectRef && pObjectRef->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObjectRef);
		int numModifiers = pDerivedObject->NumModifiers();
		//DebugPrint(" %d modifiers:\n", numModifiers);
		for (int nModifier = 0; nModifier < numModifiers; ++nModifier)
		{
			Modifier* pModifier = pDerivedObject->GetModifier(nModifier);
			Class_ID classId = pModifier->ClassID();
			//DebugPrint ("%3d. \"%s\" ClassID(%08x,%08x)\n", nModifier, pModifier->GetName(), classId.PartA(), classId.PartB());
			if (classId == MR3_CLASS_ID
				||classId == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B)
				||classId == SKIN_CLASSID)
			{
				arrModifiers.Append(1, &pModifier, 1);
			}
		}
		pObjectRef = pDerivedObject->GetObjRef(); // prepare to scan the next object
	};
}

// enables/disables the modifiers
void EnableModifiers (Tab<Modifier*>& arrModifiers, bool bEnable )
{
	for (int i = 0; i < arrModifiers.Count(); ++i)
	{
		if (bEnable)
			arrModifiers[i]->EnableMod();
		else
			arrModifiers[i]->DisableMod();
	}
}

Modifier* FindPhysiqueModifier(Object* ObjectPtr)
{
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		int numModifiers = DerivedObjectPtr->NumModifiers();
		// Iterate over all entries of the modifier stack.
		for (int ModStackIndex = 0; ModStackIndex < numModifiers; ++ModStackIndex)
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
			Class_ID classId = ModifierPtr->ClassID();

			// Is this Physique ?
			if (classId == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
			{
				// Yes -> Exit.
				return ModifierPtr;
			}
		}
	}

	// Not found.
	return NULL;
}


Modifier *FindSkinModifier(Object* ObjectPtr)
{
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this SKIN ? 
			if (ModifierPtr->ClassID() == SKIN_CLASSID)
			{
				// Yes -> Exit.
				return ModifierPtr;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
	}

	return NULL;
}


typedef std::map<INode*, Matrix3> INodeMatrixMap;

const Matrix3& GetBoneInitPos (IPhysiqueExport *pPhyExport, INode*pNode, INodeMatrixMap& mapNodeInitTM)
{
	INodeMatrixMap::iterator it = mapNodeInitTM.find (pNode);
	if (it == mapNodeInitTM.end())
	{
		Matrix3 tmInit;
		int nError = pPhyExport->GetInitNodeTM (pNode, tmInit);
		if (MATRIX_RETURNED != nError)
		{
			static Matrix3 tmIdentity (TRUE);
			assert (0);
			return tmIdentity;
		}

		it = mapNodeInitTM.insert (INodeMatrixMap::value_type (pNode, tmInit)).first;
	}
	return it->second;
}

void DebugOutput (const Matrix3& matNode)
{
	AffineParts   parts;  
	decomp_affine(matNode, &parts); 

	Point3 vX = matNode.GetRow(0);
	Point3 vY = matNode.GetRow(1);
	Point3 vZ = matNode.GetRow(2);
	// max of cosine between two vectors
	double fMaxCos = 0;
	float fXLength = vX.Length(), fYLength = vY.Length(), fZLength = vZ.Length();

	if (fXLength < 1e-3 || fYLength < 1e-3 || fZLength < 1e-3)
		DebugPrint (" Matrix Degraded");
	else
	{
		fMaxCos = max (fMaxCos, double(fabs(DotProd(vX,vY))/(fXLength*fYLength)));
		fMaxCos = max (fMaxCos, double(fabs(DotProd(vX,vZ))/(fXLength*fZLength)));
		fMaxCos = max (fMaxCos, double(fabs(DotProd(vY,vZ))/(fYLength*fZLength)));

		double fErrorDeg = 90 - fabs(acos(fMaxCos)*180/gPi);
		if (fErrorDeg > 0.5)
			DebugPrint (" NOT Orthogonal (error=%4.1f°)", fErrorDeg);

		if (DotProd(CrossProd(vX,vY),vZ) < 0)
			DebugPrint (" Left-handed %2d%%",(int)(100-fErrorDeg*100/90));
	}


	DebugPrint (":\n");

	for (int i = 0; i < 4; ++i)
	{
		Point3 ptAxis = matNode.GetRow(i);
		DebugPrint ("   %10.3f  %10.3f  %10.3f\n", ptAxis.x, ptAxis.y, ptAxis.z);
	}
}

void DebugOutput (INodeMatrixMap& mapNodes)
{
	for (INodeMatrixMap::iterator it = mapNodes.begin(); it != mapNodes.end(); ++it)
	{
		DebugPrint ("Node \"%s\" (initial pos):", it->first->GetName());

		DebugOutput(it->second);
	}
}

//////////////////////////////////////////////////////////////////////////
// Computes the initial position of the given vertex of the given physique
// PARAMETERS:
Point3 GetPhyVertexInitPos (IPhysiqueExport *pPhyExport, IPhyVertexExport* pPhyVertex, INodeMatrixMap& mapNodeInitTM)
{
	switch (pPhyVertex->GetVertexType())
	{
	case RIGID_NON_BLENDED_TYPE:
		{
			IPhyRigidVertex *pPhyRigidVertex = (IPhyRigidVertex *)pPhyVertex;
			INode* pBone = pPhyRigidVertex->GetNode();
			const Matrix3& tmBone = GetBoneInitPos(pPhyExport, pBone, mapNodeInitTM);
			Point3 ptOffset = pPhyRigidVertex->GetOffsetVector();
			return tmBone * ptOffset;
		}
		break;

	case RIGID_BLENDED_TYPE:
		{
			IPhyBlendedRigidVertex *pPhyBlendedVertex = (IPhyBlendedRigidVertex *)pPhyVertex;
			int numNodes = pPhyBlendedVertex->GetNumberNodes();
#ifdef _DEBUG
			// this will be the 
			Tab<Point3> arrParts;
			arrParts.SetCount (numNodes);
#endif
			Point3 ptSum(0,0,0);
			int nNode;
			for (nNode = 0; nNode < numNodes; ++nNode)
			{
				INode* pBone = pPhyBlendedVertex->GetNode(nNode);
				Point3 ptOffset = pPhyBlendedVertex->GetOffsetVector(nNode);
				const Matrix3& tmBone = GetBoneInitPos(pPhyExport, pBone, mapNodeInitTM);
				Point3 ptPart = tmBone*ptOffset;
#ifdef _DEBUG
				arrParts[nNode] = ptPart;
				//assert (nNode == 0 || (arrParts[nNode]-arrParts[nNode-1]).Length() < max (1e-2,arrParts[nNode].Length()/100));
#endif
				float fWeight = pPhyBlendedVertex->GetWeight(nNode);
				ptSum += ptPart * fWeight;
			}
			return ptSum;
		}
		break;

	default:
		assert (0);
		return Point3(0,0,0);
	}
}



//////////////////////////////////////////////////////////////////////////
// Used to determine the physique modified object initial pose
// PARAMETERS:
//  pNode - the node to extract the information from (must have physique modifier assigned)
//  pVertices [OUT] - array with enough space to take the vertices; may be NULL
// RETURNS:
//  if pVertices == NULL : number of vertices to pass; 0 if no modifier found, or the geometry is empty
//  otherwise: the initial pose vertices in the array pointed to by pVertices; returns the number of vertices; 0 in case of an error
unsigned CalculatePhysiqueInitialPose (INode* pNode, Point3* pVertices)
{
	Modifier *pPhysique = FindPhysiqueModifier(pNode->GetObjectRef());
	if (!pPhysique)
		return 0;
	Matrix3 tmNode = pNode->GetObjTMAfterWSM(0);

	IPhysiqueExport *pPhyExport = (IPhysiqueExport *)( pPhysique->GetInterface(I_PHYINTERFACE));
	if(!pPhyExport) return 0;

	IPhyContextExport *pPhyContext = pPhyExport->GetContextInterface(pNode);
	if(!pPhyContext) return 0;

	pPhyContext->ConvertToRigid(true);
	pPhyContext->AllowBlending(TRUE);

	unsigned numVertices = pPhyContext->GetNumberVertices();
	if (!pVertices)
		return numVertices;

	// CORE: calculates each vertex actual initial position

	// a bit of caching: memorizes each node's initial matrix; this map grows as new
	// nodes are used by subsequent vertices
	INodeMatrixMap mapNodeInitTM;

	for (unsigned nVertex = 0; nVertex < numVertices; ++nVertex)
	{
		IPhyVertexExport *pPhyVertex = pPhyContext->GetVertexInterface(nVertex);
		pVertices[nVertex] = GetPhyVertexInitPos (pPhyExport, pPhyVertex, mapNodeInitTM);
		pPhyContext->ReleaseVertexInterface(pPhyVertex);
	}

	pPhyExport->ReleaseContextInterface(pPhyContext);
	pPhysique->ReleaseInterface(I_PHYINTERFACE, pPhyExport);

#ifdef _DEBUG
	DebugOutput (mapNodeInitTM);
#endif

	return numVertices;
}

ISkinningInfoSource* FindSkinningInfoSource(INode* pNode, NameList& boneList, bool bAllowBlending)
{
	if (FindPhysiqueModifier(pNode->GetObjectRef()))
		return new PhysiqueModifierSkinningInfoSource(pNode, boneList, bAllowBlending);
	else if (FindSkinModifier(pNode->GetObjectRef()))
		return new SkinModifierSkinningInfoSource(pNode, boneList);
	else
		return 0;
}
