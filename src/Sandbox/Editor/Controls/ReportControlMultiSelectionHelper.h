// (c) 2001-2012 Crytek GmbH
#pragma once


struct IMultiSelectionHelperListener
{
	virtual void OnMultiSelectionHelperSelectionChanged() = 0;
};


class CReportControlMultiSelectionHelper
{
	enum SelectMode
	{
		SelectMode_Add,
		SelectMode_Remove,
	};

	enum ChildSelectMode
	{
		ChildSelectMode_Ignore,
		ChildSelectMode_Recursive,
	};

public:
	void OnCreate( CXTPReportControl* pControl ) const;

	void OnLButtonDblClk( CXTPReportControl* pControl, UINT nFlags, CPoint ptDblClick ) const;
	void OnLButtonUp( CXTPReportControl* pControl, UINT nFlags, CPoint point ) const;
	void OnLButtonDown( CXTPReportControl* pControl, UINT nFlags, CPoint point ) const;

	template< typename TRecord >
	void GetSelectedRecords( const CXTPReportControl* pControl, std::vector< TRecord* >& recordsOut ) const
	{
		recordsOut.clear();

		CXTPReportSelectedRows* pSelectedRows = pControl->GetSelectedRows();
		assert( pSelectedRows != NULL );

		const int selectedRowsCount = pSelectedRows->GetCount();
		for ( int i = 0; i < selectedRowsCount; ++i )
		{
			CXTPReportRow* pSelectedRow = pSelectedRows->GetAt( i );
			CXTPReportRecord* pSelectedRecordGeneric = pSelectedRow->GetRecord();
			TRecord* pSelectedRecord = DYNAMIC_DOWNCAST( TRecord, pSelectedRecordGeneric );

			recordsOut.push_back( pSelectedRecord );

			if ( ! pSelectedRecord->IsExpanded() )
			{
				AddChildRecordsToListRec< TRecord >( pSelectedRecord, recordsOut );
			}
		}
	}

private:
	void ClearSelection( CXTPReportControl* pControl ) const;
	bool SelectFromMouseClick( CXTPReportControl* pControl, UINT nFlags, CPoint point, ChildSelectMode childSelectMode ) const;
	void SelectFromRecord( CXTPReportControl* pControl, CXTPReportRecord* pRecord, SelectMode selectMode, ChildSelectMode childSelectMode ) const;
	void SelectFromRow( CXTPReportControl* pControl, CXTPReportRow* pRow, SelectMode selectMode ) const;

	template< typename TRecord >
	void AddChildRecordsToListRec( TRecord* pRecord, std::vector< TRecord* >& recordsOut ) const
	{
		assert( pRecord != NULL );

		CXTPReportRecords* pChildRecords= pRecord->GetChilds();
		if ( pChildRecords == NULL )
		{
			return;
		}

		const int childRecordsCount = pChildRecords->GetCount();
		for ( int i = 0; i < childRecordsCount; ++i )
		{
			CXTPReportRecord* pChildRecordGeneric = pChildRecords->GetAt( i );
			TRecord* pChildRecord = DYNAMIC_DOWNCAST( TRecord, pChildRecordGeneric );
			assert( pChildRecord != NULL );

			recordsOut.push_back( pChildRecord );

			AddChildRecordsToListRec( pChildRecord, recordsOut );
		}
	}
};

