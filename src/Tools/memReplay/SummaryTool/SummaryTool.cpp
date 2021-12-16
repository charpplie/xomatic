////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   SummaryTool.cpp
//  Version:     v1.00
//  Created:     07/10/2009 by Steve Barnett.
//  Description: This the implementation of the memReplay summary tool
// -------------------------------------------------------------------------
//  History:
//
//	07/10/2009: Use ReplaySDK to generate summaries of memory logs in
//							spreadsheet form
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ReplaySDK/IReplayDatabaseConnection.h"
#include "ReplaySDK/IInterfaceHooks.h"
#include "ReplaySDK/CommonCSVExporters.h"
#include "ReplaySDK/ExcelExport.h"
#include "ReplaySDK/CSVCollator.h"

// This class is responsible for providing the symbol filenames

class CSummaryHooks : public IInterfaceHooks
{
public:
	virtual void InvokeOnMainImpl( InvokeHandler handler, void* user )
	{
		// Just invoke the function
		(handler)( user );
	}
	
	virtual void ShowMessageImpl( const TCHAR* msg, const TCHAR* title )
	{
		printf( "[Message:%s] %s\n", title, msg );
	}
	
	virtual std::string OpenFileImpl( const char* filter, const TCHAR* title )
	{
		// Should only be called in the command line version of the tool to get the symbol file
		(void) filter;
		if (_tcscmp(title, _T("Find PS3 self")) == 0)
			return m_symbolFile;
		return std::string();
	}
	
	virtual std::string OpenFolderImpl( const TCHAR* title )
	{
		// Should only be called in the command line version of the tool to get the symbol file
		fprintf( stdout, __FUNCTION__ " %s\n", title );
		return std::string();
	}
	
	virtual void RunProgressTaskImpl(IProgressTask& task)
	{
		task.Start();
	}

	virtual void SetSymbolFile( std::string symbolFile )
	{
		m_symbolFile = symbolFile;
	}

protected:
	std::string m_symbolFile;
};

// This is a nasty workaround to get a Total field into the overview sheet

static const char* s_overviewExportColumns[] = {
	"Item"
};

class OverviewExporter : public ICSVExporter
{
public:
	OverviewExporter( int depthBegin, int depthEnd, const char** columns, bool inclCount, ptrdiff_t sizeTotal, ptrdiff_t sizeTotalRSX )
	: m_depthBegin( depthBegin )
	, m_depthEnd( depthEnd )
	, m_columns( columns )
	, m_inclCount( inclCount )
	, m_sizeTotal( sizeTotal )
	, m_sizeTotalRSX( sizeTotalRSX )
	{
	}

	virtual void Export(ICSVCollator& collator, const ContextTreeNode* node) const
	{
		int range = (m_depthEnd - m_depthBegin);
		std::vector<const char*> row(range + 3 + (m_inclCount ? 1 : 0));
		
		for (int i = 0; i < range; ++ i)
			row[i] = m_columns[i];

		row[range] = "Size (Main)";
		row[range + 1] = "Size (RSX)";
		row[range + 2] = "Size (Total)";
		if (m_inclCount)
			row[range + 3] = "Count";
	
		collator.AddRow(&row[0], row.size());
	
		struct item
		{
			item(const ContextTreeNode* node, int depth)
				: node(node), depth(depth) {}
			const ContextTreeNode* node;
			int depth;
		};
	
		std::vector<item> nextList;
		nextList.reserve(64);
	
		nextList.push_back(item(node, 0));
	
		while (!nextList.empty())
		{
			item top = nextList.back();
			nextList.pop_back();
	
			if (top.depth == m_depthEnd - 1)
			{
				if (top.node->GetSize().GetTotal().consumed > 0)
				{
					const ContextTreeNode* wn = top.node;
	
					for (int i = m_depthEnd - 1; i >= m_depthBegin; -- i, wn = wn->GetParent())
					{
						row[i - m_depthBegin] = wn->GetName();
					}
	
					const SizeInfoGroups& sz = top.node->GetSize();
	
					char szMain[32], szRSX[32], szTotal[32];
					sprintf_s(szMain, sizeof(szMain), "%i", sz.GetGroup(MemGroups::Main).consumed);
					sprintf_s(szRSX, sizeof(szRSX), "%i", sz.GetGroup(MemGroups::RSX).consumed);
					sprintf_s(szTotal, sizeof(szTotal), "%i", sz.GetTotal().consumed);
	
					row[range] = szMain;
					row[range + 1] = szRSX;
					row[range + 2] = szTotal;
	
					if (m_inclCount)
					{
						char cnt[32];
						sprintf_s(cnt, sizeof(cnt), "%i", top.node->GetInstanceCount());
						row[range + 3] = cnt;
					}
	
					collator.AddRow(&row[0], row.size());
				}
			}
			else
			{
				for (const ContextTreeNode* child = top.node->GetChildren(); child; child = child->GetNextSibling())
					nextList.push_back(item(child, top.depth + 1));
			}
		}

		// Now add the 'Total' row
		{
			char szMain[32], szRSX[32], szTotal[32];
			sprintf_s(szMain, sizeof(szMain), "%i", m_sizeTotal);
			sprintf_s(szRSX, sizeof(szRSX), "%i", m_sizeTotalRSX);
			sprintf_s(szTotal, sizeof(szTotal), "%i", m_sizeTotal + m_sizeTotalRSX);
			row[range - 1] = "Total";
			row[range] = szMain;
			row[range + 1] = szRSX;
			row[range + 2] = szTotal;
			if (m_inclCount)
				row[range + 3] = "1";

			collator.AddRow(&row[0], row.size());
		}
	}

private:
	int m_depthBegin;
	int m_depthEnd;
	const char** m_columns;
	bool m_inclCount;
	ptrdiff_t m_sizeTotal;
	ptrdiff_t m_sizeTotalRSX;
};

// Main SummaryTool implementation

void Usage( void )
{
	printf( "SummaryTool: Generate a summary Excel spreadsheet from a Crysis memory log\n" );
	printf( "Usage:\n" );
	printf( "\tSummaryTool.exe <log file> <symbol file>\n" );
	printf( "\n" );
}

void ExportWorksheet(ExcelExport& xls, const SharedPtr<ContextTree>& contextTree, const ICSVExporter* exporter )
{
	if ( contextTree->GetRoot() )
	{
		ExcelExportWorksheet ws(xls, contextTree->GetRoot()->GetName());
		if (exporter)
		{
			WorksheetCSVCollator collator(ws);
			exporter->Export(collator, contextTree->GetRoot());
		}
	}
}

bool ExportSpreadsheet( const char* const pFilename, SharedPtr<IReplayDatabaseConnection> pReplay )
{
	printf( "[Log] Opening file for export '%s'\n", pFilename );

	ExcelExport xls;
	if ( xls.Open( pFilename ) )
	{
		int style = xls.BeginStyle();
		xls.EndStyle();

		// Export data
		printf( "[Log] Physics export\n" );
		if ( pReplay->GetCgfPhysicsTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetCgfPhysicsTree(), &CommonCSVExporters::CgfPhysExporter );

		printf( "[Log] Texture export\n" );
		if ( pReplay->GetTextureTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetTextureTree(), &CommonCSVExporters::TextureExporter );

		printf( "[Log] Render mesh export\n" );
		if ( pReplay->GetRenderMeshTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetRenderMeshTree(), &CommonCSVExporters::RenderMeshExporter );

		printf( "[Log] Terrain export\n" );
		if ( pReplay->GetTerrainTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetTerrainTree(), &CommonCSVExporters::OverviewExporter );

		printf( "[Log] Animation export\n" );
		if ( pReplay->GetAnimationTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetAnimationTree(), &CommonCSVExporters::AnimationExporter );

		printf( "[Log] Nav export\n" );
		if ( pReplay->GetNavTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetNavTree(), &CommonCSVExporters::NavExporter );

		printf( "[Log] Entity export\n" );
		if ( pReplay->GetEntityTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetEntityTree(), &CommonCSVExporters::EntityExporter );

		// Calculate total usage for overview sheet
		SharedPtr<FrameUsageTracker> pUsage = pReplay->GetFrameUsageTracker();
		if ( !pUsage.IsValid() ) return 1;
		FrameUsageTracker::ConstIterator lastFrameMain = pUsage->FrameInfoEnd( MemGroups::Main ) - 1;
		FrameUsageTracker::ConstIterator lastFrameRSX = pUsage->FrameInfoEnd( MemGroups::RSX ) - 1;
		SizeInfoGroups usage;
		SizeInfo frameMain = *lastFrameMain;
		SizeInfo frameRSX = *lastFrameRSX;

		printf( "[Log] Overview export\n" );
		OverviewExporter overviewExporter( 1, 2, s_overviewExportColumns, false, frameMain.global, frameRSX.consumed );
		if ( pReplay->GetOverviewTree().IsValid() )
			ExportWorksheet( xls, pReplay->GetOverviewTree(), &overviewExporter );

		xls.Close();

		printf( "[Log] Finished exporting\n" );

		return true;
	}
	else
	{
		fprintf( stderr, "[Error] Failed to open output file '%s'\n", pFilename );
		return false;
	}
}

int main( int argc, char* argv[] )
{
	if ( argc != 3 )
	{
		Usage();
		return 1;
	}

	// Get the input filename
	const char* const pInput = argv[1];
	const char* const pSymbols = argv[2];

	// Generate output filename
	const int outLen = strlen( pInput ) + 4;
	char* const pOutput = new char[outLen + 1];
	if ( !pOutput ) return 1;

	sprintf_s( pOutput, outLen + 1, "%s.xml", pInput );

	printf( "[Log] Writing stats for %s to %s (symbols from %s)\n", pInput, pOutput, pSymbols );
	
	// Use the correct hooks to respond to requests for symbol file
	CSummaryHooks hooks;
	hooks.SetSymbolFile( std::string( pSymbols ) );
	IInterfaceHooks::SetHooks( &hooks );

	// Load and process the file
	SharedPtr<IReplayDatabaseConnection> pReplay = IReplayDatabaseConnection::Open( pInput );
	if ( !pReplay.IsValid() ) return 1;

	// Write out the data
	if ( !ExportSpreadsheet( pOutput, pReplay ) ) return 1;

	return 0;
}
