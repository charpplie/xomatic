#include "StdAfx.h"
#include "SelectionTreeErrorReport.h"

//////////////////////////////////////////////////////////////////////////

CSelectionTreeErrorReport::CSelectionTreeErrorReport()
{
}

//////////////////////////////////////////////////////////////////////////
const CSelectionTreeErrorRecord& CSelectionTreeErrorReport::GetError( int i ) const
{
	TErrors::const_iterator it = m_errors.begin();
	for (; it != m_errors.end() && i > 0; ++it, --i);
	assert(it != m_errors.end());
	return it->second.first;
};

//////////////////////////////////////////////////////////////////////////
CSelectionTreeErrorReport::TSelectionTreeErrorId CSelectionTreeErrorReport::AddError( const CSelectionTreeErrorRecord &err )
{
	TSelectionTreeErrorId id;
	if (IsNotInList(err, id))
	{
		id = GetNextFreeId();
		m_errors[id] = std::make_pair(err, 1);
		m_bNeedReload = true;
	}
	else
	{
		m_errors[id].second++;
	}
	return id;
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorReport::RemoveError(TSelectionTreeErrorId id)
{
	TErrors::iterator it = m_errors.find(id);
	if (it != m_errors.end())
	{
		int& count = it->second.second;
		count--;
		if (count == 0)
		{
			m_errors.erase(it);
			m_bNeedReload = true;
		}
		return;
	}
	assert( false );
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorReport::Clear()
{
	m_errors.clear();
	m_bNeedReload = true;
}

//////////////////////////////////////////////////////////////////////////
bool CSelectionTreeErrorReport::IsNotInList( const CSelectionTreeErrorRecord &err, TSelectionTreeErrorId& id ) const
{
	for (TErrors::const_iterator it = m_errors.begin(); it != m_errors.end(); ++it)
	{
		const CSelectionTreeErrorRecord& existingErr = it->second.first;
		if ( existingErr.module == err.module
			&& existingErr.type == err.type
			&& existingErr.group == err.group
			&& existingErr.error == err.error )
		{
			id = it->first;
			return false;
		}
	}
	return true;
}

CSelectionTreeErrorReport::TSelectionTreeErrorId CSelectionTreeErrorReport::GetNextFreeId() const
{
	TSelectionTreeErrorId res = 0;
	for (TErrors::const_iterator it = m_errors.begin(); it != m_errors.end(); ++it)
	{
		if (it->first != res)
			break;
		res++;
	}
	return res;
}

