/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 4:9:2009: Created by Filipe Amim

*************************************************************************/

#pragma once

#ifndef COVER_AND_LEAN_H
#define COVER_AND_LEAN_H


class CPlayer;
class CWeapon;
struct IItemParamsNode;
class CCoverAndLeanLogic;


class CCoverAndLean
{
public:

	struct SQuadPolygon
	{
		SQuadPolygon();

		Vec3 m_vertices[4];
		Plane m_plane;
	};


	struct SBox
	{
		SBox();

		SQuadPolygon m_worldPolygons[6];
		Vec3 m_localMin;
		Vec3 m_localMax;
		Matrix34 m_local;
	};


	enum ELeanMode
	{
		ELeanMode_NONE,
		ELeanMode_LEAN_AROUND_LEFT,
		ELeanMode_LEAN_AROUND_RIGHT,
		ELeanMode_LEAN_OVER,
	};

	struct SEdgeReference
	{
		SEdgeReference();

		Vec3 m_point;
		Vec3 m_normal;
		Vec3 m_tangent;
		Vec3 m_direction;
		ELeanMode m_leanMode;

		Vec3 m_minVector;
		Vec3 m_maxVector;
		float m_minAngle;
		float m_maxAngle;
		float m_depth;

		Plane m_leftLimit;
		Plane m_rightLimit;
	};

public:
	CCoverAndLean();
	~CCoverAndLean();

	void Update(CPlayer* pPlayer);

	void Reset(const IItemParamsNode* pRootNode);

	bool IsLeaning() const {return m_leaning;}
	float GetLeanAmount() const;
	float GetRaiseAmount() const;

	void AdjustMovement(Vec3* pMoveVector) const;

private:
	void UpdateWeaponRaise(bool raiseWeapon, bool noSwitchAnimation);
	void SmoothLeaning();

	bool UpdateLeanLogic();
	void UpdateReadibility();
	void DetectLeanAroundReferences();
	void DetectLeanOverReferences();
	bool CanStartLean() const;
	bool CanLean() const;

	Vec3 GetViewPosition() const;

	void StopingLeaning();


	SBox m_coverBox;
	SEdgeReference m_edgeReference;
	Vec3 m_intersectionPoint;
	Vec3 m_viewLeanVec;
	Vec3 m_viewFrontVec;

	CPlayer* m_pPlayer;
	CWeapon* m_pWeapon;

	float m_raiseAmountBias;
	float m_leanAmountBias;
	float m_maxRaise;
	float m_maxLean;
	float m_leanDetectionApparture;
	float m_maxDistanceToEdge;

	float m_leanAmount;
	float m_leanBlend;

	int m_coverSurfaceIdx;

	bool m_lastZoomed;
	bool m_leaning;
	bool m_canLean;

	static uint32 m_PhysicsCallbackRegistered;
};



#endif
