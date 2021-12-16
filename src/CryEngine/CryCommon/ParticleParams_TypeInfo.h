////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleParamsTypeInfo.h
// -------------------------------------------------------------------------
// Implements TypeInfo for ParticleParams. 
// Include only once per executable.
//
////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_TYPE_INFO_NAMES
	#if !ENABLE_TYPE_INFO_NAMES
		#error ENABLE_TYPE_INFO_NAMES previously defined to 0
	#endif
#else
	#define ENABLE_TYPE_INFO_NAMES	1
#endif

#include "TypeInfo_impl.h"
#include "IShader_info.h"
#include "I3DEngine_info.h"
#include "Cry_Geo_info.h"
#include "ParticleParams_info.h"
#include "Name_TypeInfo.h"
#include "CryTypeInfo.h"

// Manually implement Color3B

	STRUCT_INFO_BEGIN(Color3B)
		VAR_INFO(r)
		VAR_INFO(g)
		VAR_INFO(b)
	STRUCT_INFO_END(Color3B)

// Implementation of TCurveSpline<T> functions.

	// Helper class for serialization.
	template<class T>
	struct SplineElem
	{
		float fTime;
		T			Value;
		int		nFlags;
		STRUCT_INFO
	};

	// Manually define type info.
	STRUCT_INFO_T_BEGIN(SplineElem, class, T)
		VAR_INFO(fTime)
		VAR_INFO(Value)
		VAR_INFO(nFlags)
	STRUCT_INFO_T_END(SplineElem, class, T)


	template<class S>
	string TCurveSpline<S>::ToString( int flags ) const
	{
		string str;
		for (int i = 0; i < this->num_keys(); i++)
		{
			if (i > 0)
				str += ";";
			SplineElem<S> elem = { this->key(i).time, this->key(i).value, this->key(i).flags };
			str += TypeInfo(&elem).ToString(&elem, flags);
		}
		return str;
	}

	template<class S>
	void TCurveSpline<S>::FromString( const char* str, int flags )
	{
		char strElem[4096];
		this->resize(0);
		int nStrlen = strlen(str);
		if (nStrlen >= sizeof(strElem)-1)
			nStrlen = sizeof(strElem)-1;
		int nCur = 0;
		while (nCur < nStrlen)
		{
			// Extract element string.
			int nEnd = nStrlen;
			const char *strend = strchr(str+nCur,';');
			if (strend)
				nEnd = (int)(strend - str);
			
			strncpy( strElem,str+nCur,nEnd-nCur );
			strElem[nEnd-nCur] = 0;

			nCur = nEnd+1;
			while (nCur < nStrlen && str[nCur] == ' ')
				nCur++;

			// Parse element.
			SplineElem<T> elem = { 0.f, T(0.f), 0 };
			TypeInfo(&elem).FromString(&elem, strElem);

			// Fix any values somehow erroneously serialised.
			Limit(elem.fTime, 0.f, 1.f);
			if (max(elem.Value, T(0.f)) != elem.Value
			|| min(elem.Value, T(1.f)) != elem.Value)
				elem.Value = T(0.f);
			if (elem.nFlags & SPLINE_KEY_TANGENT_UNIFY_MASK)
				// Obsolete convention, now slope type settable per side.
				elem.nFlags |= (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT) 
										| (SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_OUT_SHIFT);
			elem.nFlags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);

			int nKey = insert_key(elem.fTime, elem.Value);
			this->key(nKey).flags = elem.nFlags;
		};
		this->update();
		if (flags & CTypeInfo::READ_FINALIZE)
			this->finalize();
	}

	template<class S>
	void TCurveSpline<S>::SerializeSpline( XmlNodeRef &node, bool bLoading )
	{
		if (bLoading)
			FromString( node->getAttr( "Keys" ) );
		else
			node->setAttr( "Keys", ToString(CTypeInfo::WRITE_SKIP_DEFAULT | CTypeInfo::WRITE_TRUNCATE_SUB) );
	}

