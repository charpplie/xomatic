/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 5:11:2009: Created by Filipe Amim

*************************************************************************/
#include "StdAfx.h"
#include "CoverAndLean.h"
#include "CoverAndLeanDraw.h"
#include "Player.h"
#include "GameCVars.h"




static const ColorB g_lightRed = ColorB(255, 16, 16);
static const ColorB g_darkRed = ColorB(16, 0, 0);
static const ColorB g_lightBlue = ColorB(16, 16, 255);
static const ColorB g_darkBlue = ColorB(0, 0, 16);
static const ColorB g_cyan = ColorB(0, 128, 255);
static const ColorB g_darkCyan = ColorB(0, 8, 16);
static const Vec3 g_up = Vec3(0, 0, 1);
static const float g_lineLength = 20.0f;
static const float g_lineWidth = 0.0125f;
static const float g_thinLineWidth = g_lineWidth * 0.5f;



static void DrawSolidLine(const Vec3& start, const Vec3& end, float width, const ColorB& color)
{
	const Vec3 center = (start + end) * 0.5f;
	const Vec3 dir = (end - start).GetNormalized();
	const float height = start.GetDistance(end);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawCylinder(center, dir, width, height, color);
}


static void DrawSolidVector(const Vec3& point, const Vec3& vector, float width, const ColorB& color)
{
	DrawSolidLine(point, point + vector, width, color);
}



static void DebugDrawLeanAroundEdge(const CCoverAndLean::SEdgeReference& edge, bool canLean)
{
	const ColorB edgeColor = canLean ? g_lightRed : g_darkRed;
	const ColorB normalColor = canLean ? g_lightBlue : g_darkBlue;

	DrawSolidLine(
		edge.m_point + edge.m_direction * g_lineLength * 0.5f,
		edge.m_point - edge.m_direction * g_lineLength * 0.5f,
		g_lineWidth, edgeColor);
	DrawSolidVector(
		edge.m_point,
		edge.m_normal,
		g_thinLineWidth, normalColor);
	DrawSolidVector(
		edge.m_point + edge.m_normal,
		edge.m_tangent,
		g_thinLineWidth, normalColor);
}



static void DebugDrawLeanVectors(bool canLean, const CCoverAndLean::SEdgeReference& edgeReference, const Vec3& viewPosition, const Vec3& viewFrontVec, const Vec3& viewDirection, const Vec3& viewLeanVec, float leanAmount)
{
	const ColorB vectorColor = canLean ? g_cyan : g_darkCyan;
	const ColorB leanColor = canLean ? g_lightRed : g_darkRed;

	DrawSolidVector(
		viewPosition,
		edgeReference.m_minVector * g_lineLength,
		g_thinLineWidth, vectorColor);
	DrawSolidVector(
		viewPosition,
		edgeReference.m_maxVector * g_lineLength,
		g_thinLineWidth, vectorColor);

	DrawSolidVector(
		viewPosition,
		edgeReference.m_maxVector * g_lineLength,
		g_thinLineWidth, vectorColor);

	DrawSolidVector(
		viewPosition + viewLeanVec*leanAmount,
		viewDirection * g_lineLength,
		g_lineWidth, vectorColor);
	DrawSolidVector(
		viewPosition,
		viewFrontVec * g_lineLength,
		g_thinLineWidth, vectorColor);
}



static void DebugDrawLeanOverEdge(const CCoverAndLean::SEdgeReference& edge, bool canLean)
{
	const ColorB edgeColor = canLean ? g_lightRed : g_darkRed;
	const ColorB normalColor = canLean ? g_lightBlue : g_darkBlue;

	DrawSolidLine(
		edge.m_point + edge.m_direction * g_lineLength * 0.5f,
		edge.m_point - edge.m_direction * g_lineLength * 0.5f,
		g_lineWidth, edgeColor);
	DrawSolidVector(
		edge.m_point,
		edge.m_normal,
		g_thinLineWidth, normalColor);
}



void DebugDrawEdgeReference(CPlayer* pPlayer, const CCoverAndLean::SEdgeReference& edgeReference, bool canLean, const Vec3& viewPosition, const Vec3& viewFrontVec, const Vec3& viewLeanVec, float leanAmount)
{
	if (g_pGameCVars->pl_cover_and_lean.debug_draw == 0)
		return;

	const Vec3 viewDirection = pPlayer->GetViewQuat().GetColumn1();

	if (pPlayer->GetStance() == STANCE_STAND)
		DebugDrawLeanAroundEdge(edgeReference, canLean);
	else if (pPlayer->GetStance() == STANCE_CROUCH)
		DebugDrawLeanOverEdge(edgeReference, canLean);
	DebugDrawLeanVectors(canLean, edgeReference, viewPosition, viewFrontVec, viewDirection, viewLeanVec, leanAmount);
}



void DebugDrawCurrentBox(const CCoverAndLean::SBox& currentBox)
{
	if (g_pGameCVars->pl_cover_and_lean.debug_draw == 0)
		return;

	AABB box;
	box.min = currentBox.m_localMin;
	box.max = currentBox.m_localMax;
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(box, currentBox.m_local, true, ColorB(0, 128, 255), eBBD_Faceted);

	for (int f = 0; f < 6; ++f)
	{
		for (int v = 0; v < 4; ++v)
		{
			const Vec3& point1 = currentBox.m_worldPolygons[f].m_vertices[v];
			const Vec3& point2 = currentBox.m_worldPolygons[f].m_vertices[(v+1)%4];
			DrawSolidLine(point1, point2, 0.015f, ColorB(255, 255, 255));
		}
	}
}
