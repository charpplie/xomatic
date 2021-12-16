// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "CryString.h"
#include "AudioControl.h"

namespace AudioControls
{
	class CATLControl;
	class CAudioLibrary
	{
	public:

		CAudioLibrary(const string& name);

		string GetName() const;
		bool IsModified() const;
		void SetModified(bool modified);
		void AddControl(CATLControl* pControl);
		void RemoveControl(CATLControl* pControl);
		CATLControl* GetControl(int index);
		int GetControlCount();
		bool operator ==(const CAudioLibrary& other)
		{
			return m_name == other.m_name;
		}

	private:
		string m_name;
		bool m_bModified;
		std::vector<CATLControl*> m_controls;
	};
}