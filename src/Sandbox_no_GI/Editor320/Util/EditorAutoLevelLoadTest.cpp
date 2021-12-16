#include "StdAfx.h"
#include "EditorAutoLevelLoadTest.h"

CEditorAutoLevelLoadTest &CEditorAutoLevelLoadTest::Instance()
{
	static CEditorAutoLevelLoadTest levelLoadTest;
	return levelLoadTest;
}

CEditorAutoLevelLoadTest::CEditorAutoLevelLoadTest()
{
	GetIEditor()->RegisterNotifyListener( this );
}

CEditorAutoLevelLoadTest::~CEditorAutoLevelLoadTest()
{
	GetIEditor()->UnregisterNotifyListener( this );
}

void CEditorAutoLevelLoadTest::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch ( event )
	{
		case eNotify_OnEndSceneOpen :	CLogFile::WriteLine( "[LevelLoadFinished]" );
										exit(0);
			break;
	}
}