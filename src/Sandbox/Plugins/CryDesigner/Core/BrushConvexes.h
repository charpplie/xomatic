#pragma once

namespace BUtil
{
	typedef std::vector<BrushVec3> Convex;
}

class CBrushConvexes : public CRefCountBase
{
public:

	void AddConvex( const BUtil::Convex& convex ) { m_Convexes.push_back(convex); }
	int GetConvexCount() const { return m_Convexes.size(); }
	std::vector<BrushVec3>& GetConvex(int nIndex ) { return m_Convexes[nIndex]; }

private:

	mutable std::vector<BUtil::Convex> m_Convexes;

};