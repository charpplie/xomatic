#ifndef __MATERIALHELPERS_H__
#define __MATERIALHELPERS_H__

namespace XSI {class Material;}

namespace MaterialHelpers
{
	struct MaterialInfo
	{
		MaterialInfo()
			: id(-1)
		{
		}

		string name;
		string physicalize;
		int id;
	};

	MaterialInfo GetInfo(XSI::Material& material);
}

#endif //__MATERIALHELPERS_H__
