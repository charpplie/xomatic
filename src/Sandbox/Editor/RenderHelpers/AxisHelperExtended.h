// (c) 2001-2012 Crytek GmbH
#pragma once

struct DisplayContext;
struct HitContext;
struct IPhysicalEntity;

class CAxisHelperExtended
{
public:
	CAxisHelperExtended();
	void DrawAxes(DisplayContext &dc, const Matrix34& matrix, bool bUsePhysicalProxy);

private:
	void DrawAxis(DisplayContext &dc, const Vec3& vDirAxis, const Vec3& vUpAxis, const Vec3& col, bool bUsePhysicalProxy);

private:
	Matrix34 m_matrix;
	Vec3 m_vPos;
	std::vector<IPhysicalEntity*> m_skipEntities;
	std::vector<CBaseObjectPtr> m_objects;
	CBaseObjectsArray m_objectsForPicker;
	CBaseObject* m_pCurObject;
	DWORD m_dwLastUpdateTime;

	float m_fMaxDist;
};
