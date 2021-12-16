// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "AudioLibrary.h"

namespace AudioControls
{
	CAudioLibrary::CAudioLibrary(const string& name)
		: m_name(name)
		, m_bModified(false)
	{
	}

	string CAudioLibrary::GetName() const
	{
		return m_name;
	}

	bool CAudioLibrary::IsModified() const
	{
		return m_bModified;
	}

	void CAudioLibrary::SetModified(bool modified)
	{
		m_bModified = modified;
	}

	void CAudioLibrary::AddControl(CATLControl* pControl)
	{
		m_bModified = true;
		m_controls.push_back(pControl);
	}

	void CAudioLibrary::RemoveControl(CATLControl* pControl)
	{
		m_bModified = true;
		m_controls.erase(std::remove(m_controls.begin(), m_controls.end(), pControl), m_controls.end());
	}

	CATLControl* CAudioLibrary::GetControl(int index)
	{
		if (index < m_controls.size())
		{
			return m_controls[index];
		}
		return nullptr;
	}

	int CAudioLibrary::GetControlCount()
	{
		return m_controls.size();
	}
}