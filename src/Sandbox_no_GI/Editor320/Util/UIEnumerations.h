/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: UIEnumerations.h  ,v 1.1 2009/01/05 17:48:02 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares the container for the assotiaon of 
enumeration name to enumeration values.
-------------------------------------------------------------------------
History:
- 05:01:2009   17:48 : Created by Paulo Zaffari

*************************************************************************/
#ifndef UiEnumerations_h__
#define UiEnumerations_h__

#pragma once

class CUIEnumerations
{
	public:
		// For XML standard values.
		typedef std::vector<CString>					TDValues;
		typedef std::map<CString,TDValues>		TDValuesContainer;

		// For animation selection.
		typedef std::vector<CString>					TDSelectedAnimations;
	protected:
	private:

	public:
		static CUIEnumerations& GetUIEnumerationsInstance();

		TDSelectedAnimations&	GetSelectedAnimations();

		TDValuesContainer&		GetStandardNameContainer();
	protected:
	private:
};


#endif // UiEnumerations_h__
