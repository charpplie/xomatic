/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
RecursiveDimensionalClustering.cpp

Description: 
- implements RDC on entities
- first pass is for usage by frontline spawning, needs more work to be a more
- general solution

-------------------------------------------------------------------------
History:
-	[27/05/2009] : Created by James Bamford

*************************************************************************/

#include "StdAfx.h"
#include "RecursiveDimensionalClustering.h"
#include "GameCVars.h"
#include "IEntitySystem.h"
#include "IRenderAuxGeom.h"
#include "IActorSystem.h"
#include <algorithm>

#define RDC_VERBOSE_DEBUGGING		0

void EntityRDCElement::Set(EntityId eId, Vec3 &worldPos, float entityRDCRadius)
{
	entityId = eId;
	min = worldPos;
	min.x -= entityRDCRadius;
	min.y -= entityRDCRadius;
	min.z -= entityRDCRadius;

	max = worldPos;
	max.x += entityRDCRadius;
	max.y += entityRDCRadius;
	max.z += entityRDCRadius;

#if RDC_VERBOSE_DEBUGGING
	CryLog("EntityRDCElement::Set() eId=%d; worldPos=(%f, %f, %f); radius=%f", eId, worldPos.x, worldPos.y, worldPos.z, entityRDCRadius);
	CryLog("EntityRDCElement::Set() calculated min=(%f, %f, %f); max=(%f, %f, %f)", min.x, min.y, min.z, max.x, max.y, max.z);
#endif
}


typedef struct EntityRDCAxisElement_s
{
	int EntityElementIndex;
	float pos;
	bool isOpen;

	static bool Compare(const EntityRDCAxisElement_s &a, const EntityRDCAxisElement_s &b); 
} EntityRDCAxisElement;

bool EntityRDCAxisElement::Compare(const EntityRDCAxisElement &a, const EntityRDCAxisElement &b)
{
	if (a.pos < b.pos)
		return true;
	else
		return false;
}

bool EntityRDCGroup::Compare(const EntityRDCGroup &a, const EntityRDCGroup &b)
{
	if (a.entities.size() < b.entities.size())
		return true;
	else
		return false;
}

void PerformRDCOnGroup(const EntityRDCGroup *rdcGroup, const std::vector<EntityRDCElement> *entityRDCElements, std::vector<EntityRDCGroup> *resultGroups)
{
	std::vector<int>::size_type sz = rdcGroup->entities.size();

#if RDC_VERBOSE_DEBUGGING
	CryLog("PerformRDCOnGroup() rdcGroup.entities.size()=%d; testedX=%d; testedY=%d; testedZ=%d", rdcGroup->entities.size(), rdcGroup->testedX, rdcGroup->testedY, rdcGroup->testedZ);
#endif

	if (sz == 1)
	{
#if RDC_VERBOSE_DEBUGGING
		CryLog("PerformRDCOnGroup() passed in group has only one entity - pushing this group into results and returning");
#endif
		resultGroups->push_back(*rdcGroup);
		return;
	}

	int axisIndex=-1;
	if (!rdcGroup->testedX)
	{
		axisIndex=0;
	}
	else if (!rdcGroup->testedY)
	{
		axisIndex=1;
	}
	else if (!rdcGroup->testedZ)
	{
		axisIndex=2;
	}
	else
	{
#if RDC_VERBOSE_DEBUGGING
		CryLog("PerformRDCOnGroup() failed to find an axis that needs testing - pushing this group into results and returning");
#endif
		resultGroups->push_back(*rdcGroup);
		return;
	}

	assert(axisIndex>=0);

	std::vector<EntityRDCAxisElement> axisElements;

	// Generate axis elements 
	for (int i=0; i<sz; i++)
	{
		int elementIndex = rdcGroup->entities[i];
		const EntityRDCElement &rdcElement = entityRDCElements->at(elementIndex);

		EntityRDCAxisElement openEle;
		EntityRDCAxisElement closeEle;

		openEle.EntityElementIndex = elementIndex;
		openEle.pos = rdcElement.min[axisIndex];
		openEle.isOpen = true;
		axisElements.push_back(openEle);

		closeEle.EntityElementIndex = elementIndex;
		closeEle.pos = rdcElement.max[axisIndex];
		closeEle.isOpen = false;
		axisElements.push_back(closeEle);
	}

	// Sort axis elements by pos
	std::sort(axisElements.begin(), axisElements.end(), EntityRDCAxisElement::Compare);

	std::vector<EntityRDCAxisElement>::size_type axisSz = axisElements.size();

#if RDC_VERBOSE_DEBUGGING
	CryLog("PerformRDCOnGroup() sorted axisElements for axis=%d", axisIndex);

	for (int i=0; i<axisSz; i++)
	{
		CryLog("PerformRDCOnGroup() axisElement[%d] pos=%f; entityIndex=%d; isOpen=%d", i, axisElements[i].pos, axisElements[i].EntityElementIndex, axisElements[i].isOpen);
	}
#endif

	// Look for new groups
	std::vector<EntityRDCGroup> newGroups;
	EntityRDCGroup newGroup;
	newGroup.testedX = axisIndex == 0 ? true : false;
	newGroup.testedY = axisIndex == 1 ? true : false;
	newGroup.testedZ = axisIndex == 2 ? true : false;

	int count=0;
	for (int i=0; i<axisSz; i++)
	{
		if (axisElements[i].isOpen)
		{
			count++;
			newGroup.entities.push_back(axisElements[i].EntityElementIndex);
#if RDC_VERBOSE_DEBUGGING
			CryLog("PerformRDCOnGroup() found open axis element; axisElement[%d] pos=%f; elementIndex=%d; adding to new group", i, axisElements[i].pos, axisElements[i].EntityElementIndex);
#endif
		}
		else	// is close
		{
			count--;
			if (count == 0)
			{
				// entire group needs testing
				if (newGroup.entities.size() == rdcGroup->entities.size())
				{
#if RDC_VERBOSE_DEBUGGING
					CryLog("PerformRDCOnGroup() has tested axis %d and has kept the same group size. Passing through the input group's axis tested ontop of the currently tested axis instead of reset group axis", axisIndex);
#endif

					if (axisIndex != 0)
						newGroup.testedX = rdcGroup->testedX;
					if (axisIndex != 1)
						newGroup.testedY = rdcGroup->testedY;
					if (axisIndex != 2)
						newGroup.testedZ = rdcGroup->testedZ;
				}
				else
				{
#if RDC_VERBOSE_DEBUGGING
					CryLog("PerformRDCOnGroup() has tested axis %d and has made a new group different size to current group. Passing through, needs retesting in all other dimensions", axisIndex);
#endif
				}

				PerformRDCOnGroup(&newGroup, entityRDCElements, resultGroups);

				newGroup.testedX = axisIndex == 0 ? true : false;
				newGroup.testedY = axisIndex == 1 ? true : false;
				newGroup.testedZ = axisIndex == 2 ? true : false;
				newGroup.entities.clear();
			}
		}
	}
}


void PerformRDCOnElements(const std::vector<EntityRDCElement> *entityRDCElements, std::vector<EntityRDCGroup> *resultGroups)
{
	// generate group for all elements
	EntityRDCGroup allElementsGroup;
	allElementsGroup.testedX = false;
	allElementsGroup.testedY = false;
	allElementsGroup.testedZ = false;
	
	std::vector<EntityRDCElement>::size_type sz = entityRDCElements->size();
	for (int i=0; i<sz; i++)
	{
		EntityRDCElement rdcElement = entityRDCElements->at(i);
		allElementsGroup.entities.push_back(i);	// add 
	}

	PerformRDCOnGroup(&allElementsGroup, entityRDCElements, resultGroups);
}


// if testing onlyValidGroups then takes result groups sorted in ascending order of number of entities
// stops when reaching groups with single entities inside
int CalculateBoundsOfGroups(const std::vector<EntityRDCElement> *entityRDCElements, std::vector<EntityRDCGroup> *resultGroups, bool onlyValidGroups)
{
	int numValidGroups=0;
	std::vector<EntityRDCGroup>::size_type sz = resultGroups->size();
	for (int i=sz-1; i>=0; i--)
	{
		EntityRDCGroup &group = resultGroups->at(i);
		if ((onlyValidGroups && group.entities.size() > 1) || !onlyValidGroups)
		{
			Vec3 groupMin, groupMax;
			float floatMax=std::numeric_limits<float>::max();

			// TODO fix maximum ranges
			groupMin.Set(floatMax, floatMax, floatMax);
			groupMax.Set(-floatMax, -floatMax, -floatMax);

			for (int j=0; j<group.entities.size(); j++)
			{
				int resultEntityIndex = group.entities[j];
				const EntityRDCElement &element = entityRDCElements->at(resultEntityIndex);

				if (element.min.x < groupMin.x)
					groupMin.x = element.min.x;
				if (element.min.y < groupMin.y)
					groupMin.y = element.min.y;		
				if (element.min.z < groupMin.z)
					groupMin.z = element.min.z;

				if (element.max.x > groupMax.x)
					groupMax.x = element.max.x;
				if (element.max.y > groupMax.y)
					groupMax.y = element.max.y;		
				if (element.max.z > groupMax.z)
					groupMax.z = element.max.z;
			}

			Vec3 groupMinMaxDiff;
			Vec3 groupCentre;
			groupMinMaxDiff = groupMax-groupMin;
			groupMinMaxDiff *= 0.5f; //groupMinMaxDiff * 0.5f;
			
			group.centre = groupMin + groupMinMaxDiff;
			group.myAABB.min = groupMin;
			group.myAABB.max = groupMax;

			numValidGroups++;
		}
		else
		{
			break;
		}
	}

	return numValidGroups;
}

// TODO extend this into a unit test that checks on the result
void TestEntityClustering()
{
	float entityRDCRadius=10.0f;

	CryLog("TestCalculateConflicts()");

	std::vector<EntityRDCElement> entityRDCElements;
	
	// Populate entityRDCElements
	EntityRDCElement ele;
	Vec3 worldPos(0, 0, 0);
	
	ele.Set(0, worldPos, entityRDCRadius);
	entityRDCElements.push_back(ele);

	worldPos.Set(20, 20, 20);
	ele.Set(1, worldPos, entityRDCRadius);
	entityRDCElements.push_back(ele);

	worldPos.Set(-20, -20, -20);
	ele.Set(2, worldPos, entityRDCRadius);
	entityRDCElements.push_back(ele);

	//worldPos.Set(28, 20, 20);
	//worldPos.Set(30, 20, 20);
	worldPos.Set(30, 20, 20.1f);
	ele.Set(3, worldPos, entityRDCRadius);
	entityRDCElements.push_back(ele);

	std::vector<EntityRDCGroup> resultGroups;
	PerformRDCOnElements(&entityRDCElements, &resultGroups);
	
	std::vector<EntityRDCGroup>::size_type resultSz = resultGroups.size();

	for (int i=0; i<resultSz; i++)
	{
		EntityRDCGroup &group = resultGroups[i];

		CryLog("ResultGroup %d has %d entities", i, group.entities.size());

		for (int j=0; j<group.entities.size(); j++)
		{
			CryLog("Group: %d Entity %d has entityIndex=%d; which has actual entityID=%d", i, j, group.entities[j], entityRDCElements.at(group.entities[j]).entityId);
		}
	}

	// use for debug gfx gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB()
}

#if DEBUG_NEW_SPAWNING
// TODO FIXME - use iterator code when you can instead of for loops
void VisuallyTestEntityClustering()
{
	float entityRDCRadius = g_pGameCVars->g_debug_entity_clustering_radius;
	std::vector<EntityRDCElement> entityRDCElements;

	switch (g_pGameCVars->g_debug_entity_clustering)
	{
		case 0:
			CryLog("VisuallyTestEntityClustering() ERROR - debug_entity_clustering not set");
			break;
		case 1:		// consider entities
		{
			IEntityItPtr entIt = gEnv->pEntitySystem->GetEntityIterator();
				
			if (entIt)
			{
				entIt->MoveFirst();
				while (!entIt->IsEnd())
				{
					IEntity* ent = entIt->Next();
					if (ent)
					{
						EntityRDCElement ele;
						Vec3 worldPos = ent->GetWorldPos();
						ele.Set(ent->GetId(), worldPos, entityRDCRadius);
						entityRDCElements.push_back(ele);
					}
				}
			}
			break;
		}
		case 2:		// consider actors
		{
			IActorIteratorPtr actorIt = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
			IActor *actor;
			
			while (actor=actorIt->Next())
			{
				EntityRDCElement ele;
				Vec3 worldPos = actor->GetEntity()->GetWorldPos();
//				CryLog("actor name=%s; classname=%s", actor->GetEntity()->GetName(), actor->GetEntity()->GetClass()->GetName());
				ele.Set(actor->GetEntityId(), worldPos, entityRDCRadius);
				entityRDCElements.push_back(ele);
			}
			break;
		}
		default:
			CryLog("VisuallyTestEntityClustering() ERROR - unknown debug_entity_clustering value=%d", g_pGameCVars->g_debug_entity_clustering);
			break;
	}

	std::vector<EntityRDCGroup> resultGroups;

	PerformRDCOnElements(&entityRDCElements, &resultGroups);

	CalculateBoundsOfGroups(&entityRDCElements, &resultGroups, false);

	std::vector<EntityRDCGroup>::size_type resultsSz = resultGroups.size();
	for (int i=0; i<resultsSz; i++)
	{
		const EntityRDCGroup &group = resultGroups.at(i);

		ColorB color = ColorB(128, 64, 0);
		// TODO change colour if there is more than one entity in a group
		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(m_initial_pos, color, m_targetPos, color, 6.0f);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(group.myAABB, false, color, eBBD_Faceted);
	}
}
#endif // DEBUG_NEW_SPAWNING