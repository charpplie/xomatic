#pragma once

#include "Util/BoostPythonHelpers.h"
#include "Core/BrushDesignerElementManager.h"

class CDesignerBrushObject;
class CBrushDesigner;
class CBaseBrush;
class CBrushRegion;

namespace BPython
{
	BUtil::SMainContext GetContext();
	void UpdateBrush(BUtil::SMainContext& mc, bool bForce = false);
	BrushVec3 FromSVecToBrushVec3( const SPyWrappedProperty::SVec& sVec );
	void UpdateSelection( BUtil::SMainContext& mc );

	typedef __int64 ElementID;
	class CBrushDesignerPythonContext
	{
	public:
		CBrushDesignerPythonContext()
		{
			Init();
		}

		~CBrushDesignerPythonContext()
		{
			ClearElementVariables();
		}

		CBrushDesignerPythonContext( const CBrushDesignerPythonContext& context )
		{
			operator = (context);
		}

		CBrushDesignerPythonContext& operator = ( const CBrushDesignerPythonContext& context )
		{
			bMoveTogether = context.bMoveTogether;
			bAutomaticUpdateMesh = context.bAutomaticUpdateMesh;
			
			ClearElementVariables();

			std::set<CBrushDesignerElementManager*>::iterator ii = context.elementVariables.begin();
			for( ;ii != context.elementVariables.end(); ++ii )
			{
				(*ii)->AddRef();
				elementVariables.insert(*ii);
			}

			return *this;
		}

		void Init()
		{
			elementVariables.clear();
			bMoveTogether = true;
			bAutomaticUpdateMesh = true;
		}
		
		bool bMoveTogether;
		bool bAutomaticUpdateMesh;
		
		ElementID RegisterElements( DesignerElementsPtr pElements );
		DesignerElementsPtr FindElements( ElementID id );
		void ClearElementVariables();

	private:
		std::set<CBrushDesignerElementManager*> elementVariables;
	};

	extern CBrushDesignerPythonContext s_bdpc;
	extern CBrushDesignerPythonContext s_bdpc_before_init;

	void OutputPolygonPythonCreationCode( CBrushRegion* pRegion );
};