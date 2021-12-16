////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   Database.h
//  Description: Stores a tree like structure of all records in the database and a list of descriptor types
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _DATABASE_H_
#define _DATABASE_H_

#pragma once

#include <QtCore/QObject>
#include <CryFlags.h>
#include <Descriptor/IDatabase.h>

#include "SourceControl.h"

namespace Descriptor
{
	struct ObjectTypeInfo;
}

typedef uint32 DatabaseEntryId;

struct ObjectTypeInfoEntry
{
	ObjectTypeInfoEntry( const Descriptor::ObjectTypeInfo* _pTypeInfo )
		: pTypeInfo(_pTypeInfo)
	{

	}

	string GetGroupName() const;

	const Descriptor::ObjectTypeInfo* pTypeInfo;
};

struct DatabaseEntry
{
	enum 
	{
		InvalidId = (DatabaseEntryId)(~0)
	};

	DatabaseEntry()
		: id(InvalidId)
	{

	}

	void SetDatabaseRecord(Descriptor::DatabaseRecordPtr pDatabaseRecord);
	bool HasBeenModified() const;
	void Revert();
	void BackupOriginal();

	Descriptor::DatabaseRecordPtr Get() const { return m_pDatabaseRecord; }
	
	DatabaseEntryId id;
	string path;
	string name;

private:
	Descriptor::DatabaseRecordPtr m_pDatabaseRecord;
	Descriptor::DatabaseRecordPtr m_pOriginalDatabaseRecord;
};

struct DatabaseEntryNode : public _i_reference_target_t
{
	typedef std::vector<DatabaseEntryNode*> ChildNodes;

	enum Type
	{
		Group = 0,
		Leaf
	};

	enum Flags
	{
		FlagModified = BIT(0),
		FlagInPak    = BIT(1),
		FlagInFolder   = BIT(2)
	};
	typedef CCryFlags<uint8> NodeFlags;

	DatabaseEntryNode( Type _type, const char* _name, const char* _descriptorTypeName )
		: type(_type)
		, name(_name)
		, descriptorTypeName(_descriptorTypeName)
		, databaseEntryId(DatabaseEntry::InvalidId)
		, pParent(NULL)
	{

	}

	Type      type;
	NodeFlags flags;
	string    path;
	string    name;
	string    descriptorTypeName;
	DatabaseEntryId  databaseEntryId;

	DatabaseEntryNode* pParent;
	ChildNodes  children;
};
typedef _smart_ptr<DatabaseEntryNode> DatabaseEntryNodePtr;

class Database 
	: public QObject
	, public Descriptor::IDatabaseEdit::IListener
{
	Q_OBJECT

private:

	typedef std::vector<ObjectTypeInfoEntry> ObjectTypeEntriesList;
	typedef std::map<uint32, DatabaseEntry*> DatabaseEntries;
	typedef std::vector<DatabaseEntryNodePtr> DatabaseEntryNodesList;
	typedef std::map<string, DatabaseEntryNode*, stl::less_stricmp<string> > DatabaseEntryNodesByPath;

	struct DatabaseEntryNodeLeafs
	{
		DatabaseEntryNodesList    all;
		DatabaseEntryNodesByPath byPath;
	};

	struct DatabaseEntryNodeGroups
	{
		DatabaseEntryNodesList    all;
		DatabaseEntryNodesByPath byPath;
	};

	class TypesVisitor
	{
	public:
		TypesVisitor();

		void Enumerate(const Descriptor::ObjectTypeInfo* pTypeInfo);
		void SwapDescriptorsTo( ObjectTypeEntriesList& destinationList );

	private:
		ObjectTypeEntriesList m_types;
	};

	class RecordsVisitor
	{
	public:
		typedef Functor0wRet<uint32> IdGenerator;

		RecordsVisitor( const IdGenerator& generator );

		void Enumerate(const char* recordName, Descriptor::DatabaseRecordPtr pRecord);
		void SwapRecordsTo( DatabaseEntries& destination );

	private:
		DatabaseEntries m_records;
		IdGenerator m_idGenerator;
	};

public:

	Database();
	~Database();
	
	// Record Node queries
	DatabaseEntryNode* GetRoot();
	const DatabaseEntry* GetDatabaseEntryById( const DatabaseEntryId entryId ) const;

	// Type library queries
	size_t GetTypesCount() const;
	const ObjectTypeInfoEntry& GetTypeAt( const size_t index ) const;

	// Database queries
	void AddNewRecord( const char* descriptorTypeName, const char* recordPath );
	void CloneRecord( const char* sourceRecord, const char* clonedRecordPath );
	void DeleteRecord( const char* recordPath );
	void SaveRecord(const char* recordPath);
	bool RenameRecord( DatabaseEntryNodePtr pEntryNode, const char* recordName );
	void RevertRecordChanges(const char* recordPath);
	void ReloadRecord( const char* recordPath );

	void RefreshRecordModified( DatabaseEntryNodePtr pEntryNode );

	void ShowRecordInExplorer( const char* recordPath );
	void ExtractRecordFromPak( const char* recordPath );

	bool HasModifiedRecords() const;
	void SaveAllModifiedRecords();
	void RevertAllModifiedRecords();
	void Reload();

	static string GenerateRecordFolderNameFor( const char* folderPath, const char* folderName );
	static string GenerateRecordNameFor( const char* recordFolderPath, const char* recordName );
	string GetFilePathForRecord( const char* recordPath ) const;

	//Descriptor::IDatabaseEdit::IListener
	virtual void OnRecordChanged( const char* recordName, const Descriptor::IDatabaseEdit::IListener::EChangeType changeType, Descriptor::DatabaseRecordPtr pRecord ) OVERRIDE;
	//~Descriptor::IDatabaseEdit::IListener

signals:
	void SignalBeginAddEntryNode( DatabaseEntryNode* pEntryNode );
	void SignalEndAddEntryNode( DatabaseEntryNode* pEntryNode );
	void SignalBeginDeleteEntryNode( DatabaseEntryNode* pEntryNode );
	void SignalEndDeleteEntryNode();
	void SignalEntryNodeDataChanged( DatabaseEntryNode* pEntryNode );
	void SignalEntryNodeReverted( DatabaseEntryNode* pEntryNode );

//public slots:

protected:

	void Initialize();
	void CleanupAll();
	
	void LoadDescriptorTypes();
	void LoadRecords();

	void InitializeRecordsTree();

	DatabaseEntryNode* CreateGroupEntriesForPath( DatabaseEntryNode* pRootNode, const char* path );
	void LinkEntryNodeToParent( DatabaseEntryNode* pParentNode, DatabaseEntryNode* pChildNode );
	void RemoveEntryNode( DatabaseEntryNodePtr pEntryNode );
	void RemoveEntryNodeGroup( DatabaseEntryNodePtr pEntryNode );
	void RemoveEntryNodeLeaf( DatabaseEntryNodePtr pEntryNode );
	void RemoveEntryNodeChildren( DatabaseEntryNodePtr pEntryNode );
	void UnlinkEntryNodeFromParent( DatabaseEntryNodePtr pEntryNode );

	void AddNewRecord( const char* recordPath, Descriptor::DatabaseRecordPtr& pRecord, bool saveToDisk );
	void SaveEntryNode( DatabaseEntryNodePtr pEntryNode );
	bool RenameLeafNode( DatabaseEntryNodePtr pEntryNode, const char* entryName );
	void UpdateRecordEntryPakState( DatabaseEntryNodePtr pEntryNode );

	const ObjectTypeInfoEntry* GetTypeInfoByName( const char* name ) const;
	uint32 GenerateNextId();
	
	Descriptor::IDatabaseEdit& GetDatabase();
	const Descriptor::IDatabaseEdit& GetDatabase() const;

	ObjectTypeEntriesList m_descriptorTypes;

	DatabaseEntryNodePtr  m_pRoot;
	
	DatabaseEntries       m_databaseEntries;

	DatabaseEntryNodeLeafs  m_leafNodes;
	DatabaseEntryNodeGroups m_groupNodes;

	uint32         m_entryIdGenerator;

	SourceControl  m_sourceControl;
};

#endif 

