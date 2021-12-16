#ifndef _CRY_MAX_EXPORTER_MODIFIER_UTILS_HDR_
#define _CRY_MAX_EXPORTER_MODIFIER_UTILS_HDR_

class ISkeleton;
class ISkinningInfoSource;
class NameList;
class INode;
class Object;

void PrintDebugModifiers (INode* pNode);
class MorphR3* FindMorpherModifier (INode* pNode);

// returns the reference to the (sometimes base) object before the physique, skin or morph modifier(s)
Object* GetObjectBeforeSkin (INode* pNode);

// returns the enabled skin and morpher modifiers
void GetSkinModifiers (INode* pNode, Tab<Modifier*>& arrModifiers);

// enables/disables the modifiers
void EnableModifiers (Tab<Modifier*>& arrModifiers, bool bEnable = true);

enum {g_nMaxMorphChannels = 100};

// Used to determine the physique modified object initial pose
// PARAMETERS:
//  pNode - the node to extract the information from (must have physique modifier assigned)
//  pVertices [OUT] - array with enough space to take the vertices; may be NULL
// RETURNS:
//  if pVertices == NULL : number of vertices to pass; 0 if no modifier found, or the geometry is empty
//  otherwise: the initial pose vertices in the array pointed to by pVertices; returns the number of vertices; 0 in case of an error
extern unsigned CalculatePhysiqueInitialPose (INode* pNode, Point3* pVertices);

// Calculates the initial position of each bone from arrBones, in context of
// physique modifier in pNode
// Returns the number of bone transformations got from physique, or 0 if not successful
// PARAMETERS:
//  pSkinNode    - the node that contains the skin
//  pBoneInitPos - [IN] Must be the buffer of at least arrBones.Count() elements
//                [OUT] the initial position of the bone
//
class ISkinningInfo;

extern Modifier* FindPhysiqueModifier(Object* ObjectPtr);
extern Modifier *FindSkinModifier(Object* ObjectPtr);

extern ISkinningInfoSource* FindSkinningInfoSource(INode* pNode, NameList& boneList, bool bAllowBlending);

inline void MaxToCryMatrix(Matrix34& cryMatrix, const Matrix3& maxMatrix)
{
	cryMatrix.m00 = maxMatrix[0][0];
	cryMatrix.m10 = maxMatrix[0][1];
	cryMatrix.m20 = maxMatrix[0][2];

	cryMatrix.m01 = maxMatrix[1][0];
	cryMatrix.m11 = maxMatrix[1][1];
	cryMatrix.m21 = maxMatrix[1][2];

	cryMatrix.m02 = maxMatrix[2][0];
	cryMatrix.m12 = maxMatrix[2][1];
	cryMatrix.m22 = maxMatrix[2][2];

	cryMatrix.m03 = maxMatrix[3][0];
	cryMatrix.m13 = maxMatrix[3][1];
	cryMatrix.m23 = maxMatrix[3][2];
}

inline void CryToMaxMatrix(Matrix3& maxMatrix, const Matrix34& cryMatrix)
{
	MRow *maxRow=maxMatrix.GetAddr();

	maxRow[0][0] = cryMatrix.m00;
	maxRow[0][1] = cryMatrix.m10;
	maxRow[0][2] = cryMatrix.m20;

	maxRow[1][0] = cryMatrix.m01;
	maxRow[1][1] = cryMatrix.m11;
	maxRow[1][2] = cryMatrix.m21;

	maxRow[2][0] = cryMatrix.m02;
	maxRow[2][1] = cryMatrix.m12;
	maxRow[2][2] = cryMatrix.m22;

	maxRow[3][0] = cryMatrix.m03;
	maxRow[3][1] = cryMatrix.m13;
	maxRow[3][2] = cryMatrix.m23;
}

#endif