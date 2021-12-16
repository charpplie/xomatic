#pragma once

class CBrushTriangles : public CRefCountBase
{
public:

	CBrushTriangles() : m_bHasBackFaces(false)
	{
	}

	BUtil::SMeshInfo& GetMesh() { return m_Mesh; }

	void EnableBackFaces( bool bHasBackFaces ) { m_bHasBackFaces = bHasBackFaces; }
	bool HasBackFaces() const { return m_bHasBackFaces; }

private:

	mutable BUtil::SMeshInfo m_Mesh;
	bool m_bHasBackFaces;

};