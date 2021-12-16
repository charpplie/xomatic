/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
RecursiveDimensionalClustering.h

Description: 
- implements RDC on entities
- first pass is for usage by frontline spawning, needs more work to be a more
- general solution

-------------------------------------------------------------------------
History:
-	[27/05/2009] : Created by James Bamford
*************************************************************************/

#ifndef __RECURSIVEDIMENSIONALCLUSTERING_H__
#define __RECURSIVEDIMENSIONALCLUSTERING_H__

#include <vector>	// necessary for using in EntityRDCGroup's entities? - cloudsManager.h CloudParticles usage suggests not
#include "GameCVars.h"

typedef struct EntityRDCElement_s
{
	EntityId entityId;
	Vec3 min,max;

	void Set(EntityId eId, Vec3 &worldPos, float entityRDCRadius);
} EntityRDCElement;

// TODO seperate out the results entityRDCGroup from the recursive entityRDCGroup so that we're not 
// wasting memory whilst recursing with members only really needed for the result groups
typedef struct EntityRDCGroup_s
{
	std::vector<int> entities;	// EntityElementIndex
	bool testedX, testedY, testedZ, entitiesContainsMixedTeams;	// TODO turn into flags

	AABB myAABB;
	Vec3 centre;

	static bool Compare(const EntityRDCGroup_s &a, const EntityRDCGroup_s &b);

	EntityRDCGroup_s()
	{
		myAABB.min.zero();
		myAABB.max.zero();
		centre.zero();
		testedX=testedY=testedZ=entitiesContainsMixedTeams=false;
	}

} EntityRDCGroup;

void PerformRDCOnElements(const std::vector<EntityRDCElement> *entityRDCElements, std::vector<EntityRDCGroup> *resultGroups);
int CalculateBoundsOfGroups(const std::vector<EntityRDCElement> *entityRDCElements, std::vector<EntityRDCGroup> *resultGroups, bool onlyValidGroups);
void TestEntityClustering();


#if DEBUG_NEW_SPAWNING
void VisuallyTestEntityClustering();
#endif

#endif //__RECURSIVEDIMENSIONALCLUSTERING_H__