#ifndef __SKINNINGHELPERS_H__
#define __SKINNINGHELPERS_H__

#include <xsi_ref.h>

namespace SkinningHelpers
{
	std::set<XSI::CRef> GetSkeletonRoots(XSI::CRef model);
	std::map<int, std::map<int, double> > GetVertexWeights(XSI::CRef model, const std::map<string, int>& boneNameIDMap);
}

#endif //__SKINNINGHELPERS_H__
