#include "stdafx.h"
#include "IReplayDatabaseConnection.h"
#include "ReplayDatabaseConnection.h"

SharedPtr<IReplayDatabaseConnection> IReplayDatabaseConnection::Open(const char* filename)
{
	SharedPtr<ReplayDatabaseConnection> db = new ReplayDatabaseConnection();
	if (db->Open(filename))
		return db;

	return SharedPtr<ReplayDatabaseConnection>();
}
