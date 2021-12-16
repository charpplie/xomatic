//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "Database.h"

#include "GameBind.h"
#include "EditorLog.h"
#include <Descriptor/ITypeLibrary.h>
#include <Descriptor/ObjectTypeInfo.h>

#include <Serialization/IArchive.h>
#include <Serialization/IArchiveHost.h>

#include <ISourceControl.h>
#include <ICryPak.h>

#include "InputDialog.h"

string ObjectTypeInfoEntry::GetGroupName() const
{
	return string( stack_string(pTypeInfo->name.c_str()).MakeLower().c_str() );
}

void DatabaseEntry::SetDatabaseRecord(Descriptor::DatabaseRecordPtr pDatabaseRecord)
{
	m_pDatabaseRecord = pDatabaseRecord;

	BackupOriginal();
}

bool DatabaseEntry::HasBeenModified() const
{
	if ((m_pDatabaseRecord != NULL) && (m_pOriginalDatabaseRecord != NULL))
	{
		return !Serialization::CompareBinary(*m_pDatabaseRecord->pObject, *m_pOriginalDatabaseRecord->pObject);
	}

	return false;
}

void DatabaseEntry::Revert()
{
	if ((m_pDatabaseRecord != NULL) && (m_pOriginalDatabaseRecord != NULL))
	{
		Serialization::CloneBinary(*m_pDatabaseRecord->pObject, *m_pOriginalDatabaseRecord->pObject);
	}
}

void DatabaseEntry::BackupOriginal()
{
	if (m_pDatabaseRecord != NULL)
	{
		m_pOriginalDatabaseRecord.reset( new Descriptor::DatabaseRecord(m_pDatabaseRecord->typeInfo, m_pDatabaseRecord->typeInfo.createFunction()) );

		Serialization::CloneBinary(*m_pOriginalDatabaseRecord->pObject, *m_pDatabaseRecord->pObject);
	}
	else
	{
		m_pOriginalDatabaseRecord.reset();
	}

}

//////////////////////////////////////////////////////////////////////////

Database::TypesVisitor::TypesVisitor()
{
	m_types.reserve(64);
}

void Database::TypesVisitor::Enumerate(const Descriptor::ObjectTypeInfo* pTypeInfo)
{
	ObjectTypeInfoEntry newEntry(pTypeInfo);
	m_types.push_back(newEntry);
}

void Database::TypesVisitor::SwapDescriptorsTo( ObjectTypeEntriesList& destinationList )
{
	std::swap( m_types, destinationList );
}

//////////////////////////////////////////////////////////////////////////

Database::RecordsVisitor::RecordsVisitor( const IdGenerator& generator )
	: m_idGenerator(generator)
{
	CRY_ASSERT(m_idGenerator != NULL);
}

void Database::RecordsVisitor::Enumerate( const char* recordName, Descriptor::DatabaseRecordPtr pRecord )
{
	stack_string fullRecordName = recordName;
	const stack_string::size_type foundPosition = fullRecordName.rfind( '/' );
	
	DatabaseEntry* pNewEntry = new DatabaseEntry();
	pNewEntry->path = fullRecordName.c_str();
	pNewEntry->name = (foundPosition != stack_string::npos) ? fullRecordName.substr( foundPosition + 1 ).c_str() : fullRecordName.c_str();
	pNewEntry->id = m_idGenerator();
	pNewEntry->SetDatabaseRecord(pRecord);

	m_records.insert(DatabaseEntries::value_type(pNewEntry->id, pNewEntry));
}

void Database::RecordsVisitor::SwapRecordsTo( DatabaseEntries& destination )
{
	std::swap( m_records, destination );
}

//////////////////////////////////////////////////////////////////////////

Database::Database()
	: m_entryIdGenerator(0)
{
	Initialize();

	GetDatabase().RegisterListener(this, "Editor plugin");
}

Database::~Database()
{
	CleanupAll();

	GetDatabase().UnregisterListener(this);
}

DatabaseEntryNode* Database::GetRoot()
{
	return m_pRoot.get();
}

const DatabaseEntry* Database::GetDatabaseEntryById( const DatabaseEntryId entryId ) const
{
	DatabaseEntries::const_iterator it = m_databaseEntries.find(entryId);

	if (it != m_databaseEntries.end())
	{
		return it->second;
	}

	return NULL;
}

size_t Database::GetTypesCount() const
{
	return m_descriptorTypes.size();
}

const ObjectTypeInfoEntry& Database::GetTypeAt( const size_t index ) const
{
	CRY_ASSERT(index < m_descriptorTypes.size());

	return m_descriptorTypes[index];
}

void Database::AddNewRecord( const char* descriptorTypeName, const char* recordPath )
{
	const ObjectTypeInfoEntry* pTypeEntry = GetTypeInfoByName(descriptorTypeName);
	if (pTypeEntry == NULL)
		return;

	Descriptor::DatabaseRecordPtr pNewRecord;
	if (GetDatabase().CreateRecord(*pTypeEntry->pTypeInfo, recordPath, pNewRecord) == Descriptor::IDatabaseEdit::eOR_Succeed)
	{
		AddNewRecord( recordPath, pNewRecord, true );
	}
}

void Database::CloneRecord( const char* sourceRecord, const char* clonedRecordPath )
{
	DatabaseEntryNodesByPath::const_iterator it = m_leafNodes.byPath.find( CONST_TEMP_STRING(sourceRecord) );
	if (it == m_leafNodes.byPath.end())
		return;

	const DatabaseEntry* pSourceEntry = GetDatabaseEntryById(it->second->databaseEntryId);
	if (pSourceEntry == NULL)
		return;

	Descriptor::DatabaseRecordPtr pClonedRecord;
	if (GetDatabase().CreateRecord(pSourceEntry->Get()->typeInfo, clonedRecordPath, pClonedRecord) == Descriptor::IDatabaseEdit::eOR_Succeed)
	{
		Descriptor::DatabaseRecordPtr pSourceRecord = pSourceEntry->Get();

		Serialization::CloneBinary(*pClonedRecord->pObject, *pSourceRecord->pObject);

		AddNewRecord( clonedRecordPath, pClonedRecord, true );
	}
}

void Database::DeleteRecord( const char* recordPath )
{
	DatabaseEntryNodesByPath::iterator it = m_leafNodes.byPath.find( CONST_TEMP_STRING(recordPath) );
	if (it == m_leafNodes.byPath.end())
		return;

	DatabaseEntryNodePtr pEntryNode(it->second);

	RemoveEntryNode(pEntryNode);

	const string filePath = GetFilePathForRecord(recordPath);
	m_sourceControl.DeleteFile(filePath.c_str()); 
}

void Database::SaveRecord(const char* recordPath)
{
	DatabaseEntryNodesByPath::iterator it = m_leafNodes.byPath.find( CONST_TEMP_STRING(recordPath) );
	if (it == m_leafNodes.byPath.end())
		return;

	DatabaseEntryNodePtr pEntryNode(it->second);
	SaveEntryNode(pEntryNode);
}

bool Database::RenameRecord( DatabaseEntryNodePtr pEntryNode, const char* recordName )
{
	CRY_ASSERT(pEntryNode);
	CRY_ASSERT(pEntryNode->pParent);

	if (pEntryNode->type == DatabaseEntryNode::Leaf)
	{
		return RenameLeafNode(pEntryNode, recordName);
	}

	return false;
}

void Database::RevertRecordChanges(const char* recordPath)
{
	DatabaseEntryNodesByPath::iterator nodeIt = m_leafNodes.byPath.find( CONST_TEMP_STRING(recordPath) );
	if (nodeIt == m_leafNodes.byPath.end())
		return;

	DatabaseEntryNode* pEntryNode = nodeIt->second;
	DatabaseEntries::iterator it = m_databaseEntries.find(pEntryNode->databaseEntryId);
	if (it == m_databaseEntries.end())
		return;

	it->second->Revert();

	pEntryNode->flags.ClearFlags(DatabaseEntryNode::FlagModified);

	SignalEntryNodeReverted(pEntryNode);

	EDITOR_LOG_DATABASE("Reverted changes for record '%s'", recordPath); 
}

void Database::ReloadRecord( const char* recordPath )
{
	DatabaseEntryNodesByPath::iterator nodeIt = m_leafNodes.byPath.find( CONST_TEMP_STRING(recordPath) );
	if (nodeIt == m_leafNodes.byPath.end())
		return;

	DatabaseEntryNodePtr pEntryNode = nodeIt->second;
	DatabaseEntries::iterator it = m_databaseEntries.find(pEntryNode->databaseEntryId);
	if (it == m_databaseEntries.end())
		return;

	it->second->BackupOriginal();

	pEntryNode->flags.ClearFlags(DatabaseEntryNode::FlagModified);
	UpdateRecordEntryPakState(pEntryNode);

	SignalEntryNodeReverted(pEntryNode);
}

void Database::RefreshRecordModified( DatabaseEntryNodePtr pEntryNode )
{
	if (pEntryNode == NULL)
		return;

	const DatabaseEntry* pDatabaseEntry = GetDatabaseEntryById(pEntryNode->databaseEntryId);
	if (pDatabaseEntry == NULL)
		return;

	pEntryNode->flags.SetFlags(DatabaseEntryNode::FlagModified, pDatabaseEntry->HasBeenModified());

	SignalEntryNodeDataChanged(pEntryNode.get());
}

void Database::ShowRecordInExplorer(  const char* recordPath )
{
	const string  filePath = GetFilePathForRecord( recordPath ); 

	CFileUtil::ShowInExplorer( CString(filePath.c_str()) );
}

void Database::ExtractRecordFromPak( const char* recordPath )
{
	const string  filePath = GetFilePathForRecord( recordPath ); 

	if (CFileUtil::ExtractFile( CString(filePath.c_str()), false ))
	{
		EDITOR_LOG_DATABASE("Extracted '%s' from pak");
	}
	else
	{
		EDITOR_LOG_DATABASE("Failed to extract '%s' from pak");
	}
}

bool Database::HasModifiedRecords() const
{
	for(DatabaseEntryNodesList::const_iterator it = m_leafNodes.all.begin(), end = m_leafNodes.all.end(); it != end; ++it)
	{
		const DatabaseEntryNodePtr pEntryNode = *it;
		if (pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagModified))
			return true;
	}

	return false;
}

void Database::SaveAllModifiedRecords()
{
	EDITOR_LOG_DATABASE("Saving all modified records");

	for(DatabaseEntryNodesList::iterator it = m_leafNodes.all.begin(), end = m_leafNodes.all.end(); it != end; ++it)
	{
		DatabaseEntryNodePtr pEntryNode = *it;
		if (!pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagModified))
			continue;

		SaveEntryNode(pEntryNode);
	}
}

void Database::RevertAllModifiedRecords()
{
	EDITOR_LOG_DATABASE("Reverting all modified records");

	for(DatabaseEntryNodesList::iterator nodeIt = m_leafNodes.all.begin(), endNodeIt = m_leafNodes.all.end(); nodeIt != endNodeIt; ++nodeIt)
	{
		DatabaseEntryNodePtr pEntryNode = *nodeIt;
		if (!pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagModified))
			continue;

		DatabaseEntries::iterator entryIt = m_databaseEntries.find(pEntryNode->databaseEntryId);
		{
			entryIt->second->Revert();
		}
	}
}

void Database::Reload()
{
	EDITOR_LOG_DATABASE("Reloading database...");

	while(!m_pRoot->children.empty())
	{
		DatabaseEntryNodePtr pChildEntryNode = m_pRoot->children[0];
		RemoveEntryNode(pChildEntryNode);
	}

	CleanupAll();

	GetDatabase().Reload();

	Initialize();
}

/* static */ string Database::GenerateRecordFolderNameFor( const char* folderPath, const char* folderName )
{
	stack_string fullRecordName;
	
	fullRecordName = folderPath;
	if (fullRecordName.empty())
	{
		fullRecordName.append( stack_string().Format("%s/", folderName).c_str() );
	}
	else
	{
		fullRecordName.append( stack_string().Format("/%s/", folderName).c_str() );
	}

	return string(fullRecordName.c_str());
}

/* static */ string Database::GenerateRecordNameFor( const char* recordFolderPath, const char* recordName )
{
	stack_string fullRecordName;

	fullRecordName= recordFolderPath;
	if (fullRecordName.empty())
	{
		fullRecordName.append( recordName );
	}
	else
	{
		fullRecordName.append("/");
		fullRecordName.append( recordName );
	}

	return string(fullRecordName.c_str());
}

string Database::GetFilePathForRecord( const char* recordPath ) const
{
	return string(GetDatabase().GetFilePathForRecord( recordPath ));
}

void Database::OnRecordChanged( const char* recordName, const Descriptor::IDatabaseEdit::IListener::EChangeType changeType, Descriptor::DatabaseRecordPtr pRecord )
{
	switch(changeType)
	{
	case Descriptor::IDatabaseEdit::IListener::eCT_Modified:
		{
			ReloadRecord(recordName);
		}
		break;
	case Descriptor::IDatabaseEdit::IListener::eCT_Created:
		{
			AddNewRecord(recordName, pRecord, false);
		}
		break;
	case Descriptor::IDatabaseEdit::IListener::eCT_Deleted:
		{
			DeleteRecord(recordName);
		}
		break;
	}
}

void Database::Initialize()
{
	EDITOR_LOG_DATABASE("Initializing...");
	
	LoadDescriptorTypes();
	LoadRecords();
	InitializeRecordsTree();
}

void Database::CleanupAll()
{
	// Remove types
	stl::free_container(m_descriptorTypes);
	
	// Remove actual records
	for (DatabaseEntries::iterator it = m_databaseEntries.begin(); it != m_databaseEntries.end(); ++it)
	{
		SAFE_DELETE(it->second);
	}
	stl::free_container(m_databaseEntries);

	// Remove record nodes (leafs and groups)
	stl::free_container(m_groupNodes.byPath);
	for (DatabaseEntryNodesList::iterator it = m_groupNodes.all.begin(); it != m_groupNodes.all.end(); ++it)
	{
		it->reset();
	}
	stl::free_container(m_groupNodes.all);

	stl::free_container(m_leafNodes.byPath);
	for (DatabaseEntryNodesList::iterator it = m_leafNodes.all.begin(); it != m_leafNodes.all.end(); ++it)
	{
		it->reset();
	}
	stl::free_container(m_leafNodes.all);

	EDITOR_LOG_DATABASE("All records cleaned up");
}

void Database::LoadDescriptorTypes()
{
	Descriptor::ITypeLibrary& typeLibrary = GameBind::Get().GetDescriptorLibrary();

	TypesVisitor descriptorVisitor;
	Descriptor::ITypeLibrary::EnumerateCallback visitorCallback =  functor(descriptorVisitor, &TypesVisitor::Enumerate);

	typeLibrary.EnumerateAllTypes(visitorCallback);
	descriptorVisitor.SwapDescriptorsTo( m_descriptorTypes );

	EDITOR_LOG_DATABASE("Loaded '%d' descriptor types", m_descriptorTypes.size());
}

void Database::LoadRecords()
{
	Descriptor::IDatabaseEdit& descriptorDatabase = GetDatabase();

	RecordsVisitor recordVisitor( functor(*this, &Database::GenerateNextId) );
	Descriptor::DatabaseEnumerateCallback visitorCallback = functor(recordVisitor, &RecordsVisitor::Enumerate);

	descriptorDatabase.EnumerateAllRecords(visitorCallback);
	recordVisitor.SwapRecordsTo(m_databaseEntries);

	DatabaseEntries::const_iterator end = m_databaseEntries.end();
	for (DatabaseEntries::const_iterator it = m_databaseEntries.begin(); it != end; ++it)
	{
		const DatabaseEntry* pDatabaseEntry = it->second;
		DatabaseEntryNodePtr pNewEntryNode = new DatabaseEntryNode( DatabaseEntryNode::Leaf, pDatabaseEntry->name, pDatabaseEntry->Get()->typeInfo.name.c_str() );
		pNewEntryNode->path = pDatabaseEntry->path;
		pNewEntryNode->databaseEntryId = pDatabaseEntry->id;
		UpdateRecordEntryPakState(pNewEntryNode);

		m_leafNodes.all.push_back( pNewEntryNode );
		m_leafNodes.byPath[pDatabaseEntry->path] = pNewEntryNode.get();
	}

	EDITOR_LOG_DATABASE("Loaded '%d' descriptor records", m_leafNodes.all.size());
}

void Database::InitializeRecordsTree()
{
	m_pRoot.reset( new DatabaseEntryNode( DatabaseEntryNode::Group, "GameDatabase", "" ) );
	
	// Add first the folders (just in case no elements of a certain type haven been created yet)
	for (ObjectTypeEntriesList::const_iterator it = m_descriptorTypes.begin(); it != m_descriptorTypes.end(); ++it)
	{
		CreateGroupEntriesForPath(m_pRoot.get(), string().Format("%s/", it->GetGroupName().c_str()).c_str());
	}

	// Assign leaf elements to tree nodes
	for (DatabaseEntryNodesList::const_iterator it = m_leafNodes.all.begin(); it != m_leafNodes.all.end(); ++it)
	{
		DatabaseEntryNode* pLeafNode = *it;
		DatabaseEntryNode* pGroupNode = CreateGroupEntriesForPath( m_pRoot.get(), pLeafNode->path.c_str() );
		if (pGroupNode != NULL)
		{
			LinkEntryNodeToParent(pGroupNode, pLeafNode);
		}
		else
		{
			assert(false);
		}
	}

	EDITOR_LOG_DATABASE("Constructed records tree for explorer"); 
}

DatabaseEntryNode* Database::CreateGroupEntriesForPath( DatabaseEntryNode* pRootNode, const char* path )
{
	const char* currentPath = path;
	DatabaseEntryNode* pCurrentGroup = pRootNode;
	while (true)
	{
		string name;
		string groupPath;

		const char* folderName = currentPath;
		const char* folderNameEnd = strchr(currentPath, '/');
		if (folderNameEnd != 0)
		{
			currentPath = folderNameEnd + 1;

			name.assign(folderName, folderNameEnd);
			groupPath.assign(path, folderNameEnd);
		}
		else
		{
			break;			
		}

		DatabaseEntryNodesByPath::iterator it = m_groupNodes.byPath.find(groupPath);
		if (it == m_groupNodes.byPath.end())
		{
			DatabaseEntryNode* pNewGroup = new DatabaseEntryNode( DatabaseEntryNode::Group, name, "" );
			pNewGroup->path = groupPath;

			m_groupNodes.all.push_back( DatabaseEntryNodePtr(pNewGroup) );
			m_groupNodes.byPath[groupPath] = pNewGroup;

			LinkEntryNodeToParent(pCurrentGroup, pNewGroup);

			pCurrentGroup = pNewGroup;
		}
		else
		{
			pCurrentGroup = it->second;
		}
	}

	return pCurrentGroup;
}

void Database::LinkEntryNodeToParent( DatabaseEntryNode* pParentNode, DatabaseEntryNode* pChildNode )
{
	CRY_ASSERT(pChildNode->pParent == NULL);

	pChildNode->pParent = pParentNode;

	SignalBeginAddEntryNode( pChildNode );
	
	const bool uniqueInserted = stl::push_back_unique(pParentNode->children, pChildNode);
	
	CRY_ASSERT(uniqueInserted);

	SignalEndAddEntryNode( pChildNode );
}

void Database::RemoveEntryNode( DatabaseEntryNodePtr pEntryNode )
{
	if (pEntryNode->type == DatabaseEntryNode::Group)
	{
		RemoveEntryNodeGroup(pEntryNode);
	}
	else if (pEntryNode->type == DatabaseEntryNode::Leaf)
	{
		RemoveEntryNodeLeaf(pEntryNode);
	}
	else
	{
		CRY_ASSERT(0);
	}
}

void Database::RemoveEntryNodeGroup( DatabaseEntryNodePtr pEntryNode )
{
	RemoveEntryNodeChildren(pEntryNode);
	UnlinkEntryNodeFromParent(pEntryNode);

	m_groupNodes.byPath.erase( CONST_TEMP_STRING(pEntryNode->path) );
	stl::find_and_erase(m_groupNodes.all, pEntryNode);
}

void Database::RemoveEntryNodeLeaf( DatabaseEntryNodePtr pEntryNode )
{
	CRY_ASSERT(pEntryNode->type == DatabaseEntryNode::Leaf);

	UnlinkEntryNodeFromParent(pEntryNode);

	const DatabaseEntryId recordId = pEntryNode->databaseEntryId;

	const DatabaseEntry* pDatabaseEntry = GetDatabaseEntryById(recordId);
	CRY_ASSERT(pDatabaseEntry);

	GetDatabase().DeleteRecord(pDatabaseEntry->path.c_str());

	m_leafNodes.byPath.erase(pEntryNode->path);
	
	stl::find_and_erase(m_leafNodes.all, pEntryNode);
	m_databaseEntries.erase(recordId);
}

void Database::RemoveEntryNodeChildren( DatabaseEntryNodePtr pEntryNode )
{
	CRY_ASSERT(pEntryNode->type == DatabaseEntryNode::Group);

	while(!pEntryNode->children.empty())
	{
		DatabaseEntryNode* pChildNode = pEntryNode->children[0];
		RemoveEntryNode(pChildNode);
	}
}

void Database::UnlinkEntryNodeFromParent( DatabaseEntryNodePtr pEntryNode )
{
	CRY_ASSERT(pEntryNode->pParent != NULL);

	DatabaseEntryNode* pParent = pEntryNode->pParent;
	for (DatabaseEntryNode::ChildNodes::iterator it = pParent->children.begin(); it != pParent->children.end();)
	{
		DatabaseEntryNode* pCurrentChildNode = *it;
		if (pCurrentChildNode != pEntryNode)
		{
			++it;
		}
		else
		{
			SignalBeginDeleteEntryNode(pEntryNode);
			DatabaseEntryNode::ChildNodes::iterator nextIt = pEntryNode->pParent->children.erase(it);
			pEntryNode->pParent = NULL;
			SignalEndDeleteEntryNode();

			it = nextIt;
		}
	}
}

void Database::AddNewRecord( const char* recordPath, Descriptor::DatabaseRecordPtr& pRecord, bool saveToDisk )
{
	stack_string fullRecordName = recordPath;
	const stack_string::size_type foundPosition = fullRecordName.rfind( '/' );

	DatabaseEntry* pDatabaseEntry = new DatabaseEntry();
	pDatabaseEntry->path = fullRecordName.c_str();
	pDatabaseEntry->name = (foundPosition != stack_string::npos) ? fullRecordName.substr( foundPosition + 1 ).c_str() : fullRecordName.c_str();
	pDatabaseEntry->id = GenerateNextId();
	pDatabaseEntry->SetDatabaseRecord(pRecord);

	m_databaseEntries.insert(DatabaseEntries::value_type(pDatabaseEntry->id, pDatabaseEntry));

	DatabaseEntryNodePtr pNewEntryNode = new DatabaseEntryNode( DatabaseEntryNode::Leaf, pDatabaseEntry->name, pDatabaseEntry->Get()->typeInfo.name.c_str() );
	pNewEntryNode->path = pDatabaseEntry->path;
	pNewEntryNode->databaseEntryId = pDatabaseEntry->id;

	m_leafNodes.all.push_back( pNewEntryNode );
	m_leafNodes.byPath[pDatabaseEntry->path] = pNewEntryNode.get();

	DatabaseEntryNode* pGroupNode = CreateGroupEntriesForPath(m_pRoot.get(), pDatabaseEntry->path.c_str());
	if (pGroupNode)
	{
		LinkEntryNodeToParent(pGroupNode, pNewEntryNode);
		if (saveToDisk)
		{
			SaveEntryNode( pNewEntryNode );
		}
		UpdateRecordEntryPakState(pNewEntryNode);
	}
	else
	{
		CRY_ASSERT(false);
	}
}

void Database::SaveEntryNode( DatabaseEntryNodePtr pEntryNode )
{
	CRY_ASSERT(pEntryNode->type == DatabaseEntryNode::Leaf);

	EDITOR_LOG_DATABASE("Saving record '%s'...", pEntryNode->path.c_str());

	INDENT_LOG_DURING_SCOPE();

	const string filePath = GetFilePathForRecord(pEntryNode->path.c_str());

	const bool checkedOut = m_sourceControl.CheckoutFile(filePath.c_str());
	
	if(GetDatabase().SaveRecord( pEntryNode->path.c_str() ) == Descriptor::IDatabaseEdit::eOR_Succeed)
	{
		// Attempt again, it could have failed because file did not existed yet and needs to be added to SC
		if (!checkedOut)
		{
			m_sourceControl.CheckoutFile(filePath.c_str());
		}
	}
	else
	{
		stack_string message;
		message.Format("Error: record '%s' could not be saved", pEntryNode->path.c_str());
		ConfirmationDialog failToSaveDialog("Failed to save", message.c_str(), ConfirmationDialog::OPTIONS_OK);
		failToSaveDialog.exec();
		return;
	}

	EDITOR_LOG_DATABASE("Record '%s' saved in file '%s'", pEntryNode->path.c_str(), filePath);

	pEntryNode->flags.ClearFlags(DatabaseEntryNode::FlagModified);

	DatabaseEntries::iterator it = m_databaseEntries.find(pEntryNode->databaseEntryId);
	if (it == m_databaseEntries.end())
		return;

	it->second->BackupOriginal();

	SignalEntryNodeDataChanged(pEntryNode.get());
}

bool Database::RenameLeafNode( DatabaseEntryNodePtr pEntryNode, const char* recordName )
{
	const string newRecordPath = GenerateRecordNameFor(pEntryNode->pParent->path.c_str(), recordName);
	// Check if duplicated
	if (m_leafNodes.byPath.find(newRecordPath) != m_leafNodes.byPath.end())
		return false;

	// Revert original file if checked out
	const string sourceFilePath = GetFilePathForRecord(pEntryNode->path.c_str());
	m_sourceControl.RevertFile( sourceFilePath.c_str() );

	// Clone original and assign new name
	CloneRecord(pEntryNode->path.c_str(), newRecordPath.c_str());
	
	// Delete original one
	RemoveEntryNodeLeaf(pEntryNode);
	m_sourceControl.DeleteFile( sourceFilePath.c_str() );

	return true;
}

void Database::UpdateRecordEntryPakState( DatabaseEntryNodePtr pEntryNode )
{
	CRY_ASSERT(pEntryNode != NULL);
	
	const string filePath = GetFilePathForRecord(pEntryNode->path.c_str());

	pEntryNode->flags.SetFlags(DatabaseEntryNode::FlagInFolder, gEnv->pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_OnDisk));
	pEntryNode->flags.SetFlags(DatabaseEntryNode::FlagInPak, gEnv->pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_InPak));
}

const ObjectTypeInfoEntry* Database::GetTypeInfoByName( const char* name ) const
{
	for (size_t i = 0, typesCount = m_descriptorTypes.size(); i < typesCount; ++i)
	{
		const ObjectTypeInfoEntry& entry = m_descriptorTypes[i];
		if (strcmp(entry.pTypeInfo->name.c_str(), name) == 0)
			return &entry;
	}

	return NULL;
}

uint32 Database::GenerateNextId()
{
	return ++m_entryIdGenerator;
}

Descriptor::IDatabaseEdit& Database::GetDatabase()
{
	return GameBind::Get().GetDatabase();
}

const Descriptor::IDatabaseEdit& Database::GetDatabase() const
{
	return GameBind::Get().GetDatabase();
}

#include <moc_Database.cpp>

