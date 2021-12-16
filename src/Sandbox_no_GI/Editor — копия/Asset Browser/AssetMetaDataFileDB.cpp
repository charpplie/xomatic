////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetMetaDataFileDB.cpp
//  Version:	v1.00
//  Created:	18/05/2010 15:43 by Jaewon Jung
//  Description:	Implementation of AssetMetaDataFileDB.h
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "AssetMetaDataFileDB.h"
#include "Util/FileUtil.h"
#include "Util/IndexedFiles.h"
#include "Include/IAssetDisplayDatabase.h"
#include "Include/IAssetDisplay.h"
#include "Asset Browser/AssetBrowserDialog.h"
#include "AssetTags.h"

volatile TIntAtomic CAssetMetaDataFileDB::s_bDbLoadingDone;
volatile bool CAssetMetaDataFileDB::s_bCachingDone = true;
bool CAssetMetaDataFileDB::s_bIsCacheInfoOnThread = true;
CAssetTags CAssetMetaDataFileDB::s_tagManager;

CAssetTags& CAssetMetaDataFileDB::GetTagManager()
{
	return s_tagManager;
}

void CAssetMetaDataFileDB::StartLoadAndUpdate()
{
	assert(s_bDbLoadingDone == 0);
	GetLoadAndUpdateThread().Start(-1, "AssetMetaDataFileDB_LoadAndUpdate");
	GetTagManager().Initialize();
}

void CAssetMetaDataFileDB::StartCachingInfo( bool bOnThread )
{
	if( !s_bCachingDone )
		return;

	s_bCachingDone = false;
	s_bIsCacheInfoOnThread = bOnThread;

	if( bOnThread )
	{
		GetCachingInfoThread().Start(-1, "AssetMetaDataFileDB_CachingInfo");
	}
	else
	{
		CacheAssetsInfo();
	}
}

IAssetDisplay* CAssetMetaDataFileDB::GetCurrentNonThreadedAssetToCacheInfo()
{
	IAssetDisplay* pAsset = GetCachingInfoThread().m_pNonThreadedCache;

	GetCachingInfoThread().m_pNonThreadedCache = NULL;

	return pAsset;
}

void CAssetMetaDataFileDB::AbortLoadAndUpdate()
{
	if( IsLoadAndUpdateDone() )
	{
		return;
	}
	else
	{
		GetLoadAndUpdateThread().Abort();
	}
}

void CAssetMetaDataFileDB::AbortCachingInfo()
{
	if( s_bIsCacheInfoOnThread )
	{
		if( IsCachingInfoDone() )
		{
			return;
		}
		else
		{
			GetCachingInfoThread().Abort();
		}
	}
	else
	{
		if( IsCachingInfoDone() )
		{
			return;
		}
		else
		{
			GetCachingInfoThread().m_bStopped = true;
		}
	}

	s_bCachingDone = true;
}

bool CAssetMetaDataFileDB::LoadAndUpdate(const CString& fileName)
{
	XmlNodeRef root = ReadAndVerifyMainDB(fileName);
	if(root == NULL)
		return false;

	CString fullPath = Path::GetUserSandboxFolder();

	ReadTransactionsAndUpdateDB(fullPath, root);

	///4. Save the updated DB.
	root->saveToFile(fileName);

	// To call the 'Refresh()' method for each database, 
	// the file indexing should be completed first.
	while(CIndexedFiles::HasFileIndexingDone() == false)
		::Sleep(100);

	ClearTransactionsAndApplyMetaData(fullPath, root);

	return true;
}

bool CAssetMetaDataFileDB::CacheAssetsInfo()
{
	// if this function is not called on a thread, then init the caching variables
	if( !s_bIsCacheInfoOnThread )
	{
		s_bCachingDone = false;
		GetCachingInfoThread().m_bStopped = false;
		GetCachingInfoThread().m_currentCount = 0;
		GetCachingInfoThread().m_totalCount = 0;
		GetCachingInfoThread().m_pNonThreadedCache = NULL;
	}

	IAssetDisplayDatabase* pCurrentDatabaseInterface = NULL;
	std::vector<IClassDesc*> assetDatabasePlugins;
	IEditorClassFactory *pClassFactory = GetIEditor()->GetClassFactory();
	pClassFactory->GetClassesByCategory("Asset Display", assetDatabasePlugins);

	// Get the total count of all assets.
	for( size_t i = 0; i < assetDatabasePlugins.size(); ++i )
	{
		if( assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetDisplayDatabase), (void**)&pCurrentDatabaseInterface ) == S_OK )
		{
			GetCachingInfoThread().m_totalCount += pCurrentDatabaseInterface->GetAssets().size();
		}
	}

	// Cache the meta data from all assets with a progress display.
	for( size_t i = 0; i < assetDatabasePlugins.size(); ++i )
	{
		if( assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetDisplayDatabase), (void**)&pCurrentDatabaseInterface ) == S_OK )
		{
			IAssetDisplayDatabase::TFilenameAssetMap& assets = pCurrentDatabaseInterface->GetAssets();

			for( IAssetDisplayDatabase::TFilenameAssetMap::iterator iter = assets.begin(), iterEnd = assets.end(); iter != iterEnd; ++iter )
			{
				if( !iter->second->IsFlagSet( IAssetDisplay::eAssetFlags_CachedFieldsInfo ) )
				{
					CAssetBrowserDialog* pAB = (CAssetBrowserDialog*)CAssetBrowserDialog::GetCurrentInstance();

					if( pAB )
					{
						if( iter->second->IsFlagSet( IAssetDisplay::eAssetFlags_ThreadFieldsInfoCachingSupported ) || !s_bIsCacheInfoOnThread )
						{
							// Actual caching
							pAB->GetAssetViewer().LockAssetItemsArray();
							iter->second->CacheFieldsInfo();
							pAB->GetAssetViewer().UnlockAssetItemsArray();
						}
						else
						{
							GetCachingInfoThread().m_pNonThreadedCache = iter->second;

							while( GetCachingInfoThread().m_pNonThreadedCache && !GetCachingInfoThread().m_bStopped )
							{
								Sleep( 1 );
							}
						}
					}
				}

				// Progress update, the current count of processed assets from the total count
				++GetCachingInfoThread().m_currentCount;

				// if the caching of asset info is not done on a thread, we should update the engine and other UI systems
				// and also process incoming window messages
				if( !s_bIsCacheInfoOnThread )
				{
					// We should call this periodically to clear up garbages.
					// Otherwise, this caching will take the whole memory up and
					// spit out the memory error.
					if( GetCachingInfoThread().m_currentCount % 10 == 0 )
					{
						GetIEditor()->Get3DEngine()->Update();
						((CCryEditApp*)AfxGetApp())->ForceNextIdleProcessing();
						((CCryEditApp*)AfxGetApp())->OnIdle(0);
					}
				
					// Handle window messages if we're not on a separate thread
					MSG msg;
					
					if( FALSE != PeekMessage( &msg, 0, 0, 0, 0 ) && !GetCachingInfoThread().m_bStopped )
					{ 
						if( msg.message == WM_QUIT )
						{
							GetCachingInfoThread().m_bStopped = true;
							break;
						}

						// pump the messages from the app's queue
						AfxGetApp()->PumpMessage();
					}
				}

				// if the asset browser window is still opened and the caching of info was cancelled, then stop this caching function
				if( CAssetBrowserDialog::GetCurrentInstance() &&
						CAssetBrowserDialog::GetCurrentInstance()->GetAssetViewer().IsCachingCancelled() )
				{
					GetCachingInfoThread().m_bStopped = true;
				}

				// if there is not asset browser visible, stop
				if( !CAssetBrowserDialog::GetCurrentInstance() )
				{
					GetCachingInfoThread().m_bStopped = true;
				}

				// if the stop boolean is set to true, we stop and return
				if( GetCachingInfoThread().m_bStopped )
				{
					s_bCachingDone = true;
					return true;
				}

				// if this function is on a thread, then lets give some CPU time to other threads
				if( s_bIsCacheInfoOnThread )
				{
					Sleep( 1 );
				}
				else
				// if the asset browser is opened, we update the UI each 100 assets processed
				if( CAssetBrowserDialog::GetCurrentInstance() && GetCachingInfoThread().m_currentCount % 100 == 0 )
				{
					CAssetBrowserDialog::GetCurrentInstance()->GetAssetViewer().SetCachingProgress( GetCachingInfoCurrentCount(), GetCachingInfoTotalCount() );
					CAssetBrowserDialog::GetCurrentInstance()->GetAssetViewer().Invalidate();
				}
			}
		}
	}

	// stop all
	s_bCachingDone = true;
	GetCachingInfoThread().m_bStopped = true;

	// if the asset browser is opened and this is not called on a thread, signal the stop of caching to the browser (and asset thumb control)
	if( CAssetBrowserDialog::GetCurrentInstance() && !s_bIsCacheInfoOnThread )
	{
		CAssetBrowserDialog::GetCurrentInstance()->StopCacheAssetInfo();
	}

	return true;
}

static CryMutex transactionUpdateMutex;
bool CAssetMetaDataFileDB::OnNewTransaction(const IAssetDisplay *pAssetItem)
{
	assert(pAssetItem);
	if(pAssetItem == NULL)
		return false;

	IAssetDisplayDatabase* pDatabaseInterface 
		= pAssetItem->GetOwnerDisplayDatabase();

	CryAutoLock<CryMutex> scopedLock(transactionUpdateMutex);

	CString fullPath = Path::GetUserSandboxFolder();
	fullPath += pDatabaseInterface->GetTransactionFilename();

	//1. Get  the transaction data.
	XmlNodeRef newTransactionNode = XmlHelpers::CreateXmlNode("Transaction");
	pAssetItem->ToXML(newTransactionNode);
	CFileUtil::FileDesc fd;
	if(CFileUtil::FileExists(newTransactionNode->getAttr("fileName"), &fd))
	{
		// Add the time stamp info.
		CString buf;
		buf.Format("%I64d", fd.time_write);
		newTransactionNode->setAttr("timeStamp", buf);

		//2. Append the new transaction to the file.
		FILE *transactionFile = fopen(fullPath, "r+t");
		if(transactionFile == NULL)
			return false;
		const char endTag[] = "</Transactions>";
		int result = fseek(transactionFile, -( (long)sizeof( endTag ) ) + 1, SEEK_END);	// To delete the end tag of "</Transactions>" 
		if(result != 0)	// It means this is the first transaction saved.
		{
			fseek(transactionFile, 0, SEEK_SET);
			fprintf(transactionFile, "<Transactions>\n");
		}
		fprintf(transactionFile, "%s%s", newTransactionNode->getXML().c_str(), endTag);
		fclose(transactionFile);
	}

	return true;
}

static void CopyNode(XmlNodeRef dst, XmlNodeRef src)
{
	dst->copyAttributes(src);
	dst->removeAllChilds();
	for(int i=0; i<src->getChildCount(); ++i)
		dst->addChild(src->getChild(i));
	dst->setContent(src->getContent());
}

static void FastErase(XmlNodeRef node, int i)
{
	XmlNodeRef child = node->getChild(i);
	XmlNodeRef last = node->getChild(node->getChildCount()-1);

	// Copy the last child to the index i.
	CopyNode(child, last);

	// And pop back the last.
	node->deleteChildAt(node->getChildCount()-1);
}

XmlNodeRef CAssetMetaDataFileDB::ReadAndVerifyMainDB( const CString& fileName )
{
	XmlNodeRef root;
	///1. Read the main DB first.
	if(CFileUtil::FileExists(fileName) == false)
	{
		root = GetISystem()->CreateXmlNode("AssetMetaDataFileDB");
	}
	else
	{
		root = GetISystem()->LoadXmlFromFile(fileName);
		if(root == NULL)
			return NULL;
	}

	///2. Check if each file still exists/has the same timestamp. 
	for(int i=0; i<root->getChildCount(); ++i)
	{
		XmlNodeRef db = root->getChild(i);
		for(int k=0; k<db->getChildCount(); ++k)
		{
			XmlNodeRef entry = db->getChild(k);
			CFileUtil::FileDesc fd;
			if(CFileUtil::FileExists(entry->getAttr("fileName"), &fd) == false)
			// The file no longer exists, remove the entry from the DB.
			{
				FastErase(db, k);
			}
			else
			{
				CString actualTimeStamp;
				actualTimeStamp.Format("%I64d", fd.time_write);
				CString savedTimeStamp = entry->getAttr("timeStamp");
				if(actualTimeStamp != savedTimeStamp)
				// The file changed, remove the entry from the DB.
				{
					FastErase(db, k);
				}
			}
		}
	}

	return root;
}

void CAssetMetaDataFileDB::ReadTransactionsAndUpdateDB( const CString& fullPath, XmlNodeRef& root )
{
	///3. Now read transaction files for each DB and process each.
	IAssetDisplayDatabase* pCurrentDatabaseInterface = NULL;
	std::vector<IClassDesc*> assetDatabasePlugins;
	IEditorClassFactory *pClassFactory = GetIEditor()->GetClassFactory();
	pClassFactory->GetClassesByCategory("Asset Display", assetDatabasePlugins);

	for(size_t i=0; i<assetDatabasePlugins.size(); ++i)
	{
		if(assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetDisplayDatabase), (void**)&pCurrentDatabaseInterface ) == S_OK)
		{
			// Register the listener for recording transactions during this run of the editor.
			pCurrentDatabaseInterface->AddMetaDataChangeListener(functor(CAssetMetaDataFileDB::OnNewTransaction));
			CString filePath = fullPath + pCurrentDatabaseInterface->GetTransactionFilename();
			if(CFileUtil::FileExists(filePath) == false)
				continue;
			XmlNodeRef rootTransaction = GetISystem()->LoadXmlFromFile(filePath);
			if(rootTransaction == NULL)
				continue;

			// Update with transactions. 
			XmlNodeRef dbNode = root->findChild(pCurrentDatabaseInterface->GetDatabaseName());
			if(dbNode == NULL)
				dbNode = root->newChild(pCurrentDatabaseInterface->GetDatabaseName());
			for(int k=0; k<rootTransaction->getChildCount(); ++k)
			{
				XmlNodeRef transaction = rootTransaction->getChild(k);
				bool found = false;
				for(int m=0; m<dbNode->getChildCount(); ++m)
				{
					XmlNodeRef entry = dbNode->getChild(m);
					if(strcmp(entry->getAttr("fileName"), transaction->getAttr("fileName")) == 0)
					{
						CopyNode(entry, transaction);		// Update the entry if it already exists.
						found = true;
						break;
					}
				}
				if(found == false)
					dbNode->addChild(transaction);		// Or add a new one.
			}
		}
	}
}

void CAssetMetaDataFileDB::ClearTransactionsAndApplyMetaData( const CString& fullPath, XmlNodeRef& root )
{
	IAssetDisplayDatabase* pCurrentDatabaseInterface = NULL;
	std::vector<IClassDesc*> assetDatabasePlugins;
	IEditorClassFactory *pClassFactory = GetIEditor()->GetClassFactory();
	pClassFactory->GetClassesByCategory("Asset Display", assetDatabasePlugins);

	for(size_t i=0; i<assetDatabasePlugins.size(); ++i)
	{
		///5. Clear transaction files.
		if(assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetDisplayDatabase), (void**)&pCurrentDatabaseInterface ) == S_OK)
		{
			CString filePath = fullPath + pCurrentDatabaseInterface->GetTransactionFilename();
			FILE *file = fopen(filePath, "wt");
			if(file == NULL)
				continue;
			fputs ("<Transactions>\n</Transactions>", file);
			fclose(file);
		}

		///6. Initialize each asset browser database. 
		pCurrentDatabaseInterface->Refresh();

		///7. Precache each asset browser database with the info DB.
		XmlNodeRef dbNode = root->findChild(pCurrentDatabaseInterface->GetDatabaseName());
		if(dbNode)
			pCurrentDatabaseInterface->PrecacheFieldsInfoFromFileDB(dbNode);
	}
}

void CAssetMetaDataFileDB::CLoadAndUpdateThread::Run()
{
	// Load the asset info file DB mainly for the asset browser.
	CString fullPath = Path::GetUserSandboxFolder();
	fullPath += "assetInfoDB.xml";
	if (CAssetMetaDataFileDB::LoadAndUpdate(fullPath) == false)
	{
		CString errorMsg;
		errorMsg.Format("Cannot load Asset Info File DB!\nIf the issue continues, try to delete the '%s' file.", fullPath);
		AfxMessageBox(errorMsg, MB_ICONWARNING|MB_OK);
	}

	CryInterlockedAdd(CAssetMetaDataFileDB::s_bDbLoadingDone.Addr(), 1);
}

void CAssetMetaDataFileDB::CCachingInfoThread::Run()
{
	if( !m_bStopped )
		return;

	m_bStopped = false;
	s_bCachingDone = false;
	m_currentCount = m_totalCount = 0;
	CAssetMetaDataFileDB::CacheAssetsInfo();
	s_bCachingDone = true;
	m_bStopped = true;
}
