#ifndef __ICRYTELEMETRY_H__
#define __ICRYTELEMETRY_H__

#include "ICryLobby.h"

// May be expanded in future, at present when callback fires data will be de-queued immediately after.
enum ECryTelemetryResult
{
	eCTR_Ok,				// No errors in sending data to external site
	eCTR_Failed			// Some sort of error occurred (likely to be a fail in the socket send)
};

typedef void (*CryTelemetryCallback)(ECryTelemetryResult res, void* arg);

struct STelemetryData : public CMultiThreadRefCount
{
	STelemetryData(uint32 dataSize)
	{
		pData = new char[dataSize];
		length = dataSize;
		telemCb = NULL;
		userArg = NULL;
	}
	~STelemetryData()
	{
		delete [] pData;
	}

	CryTelemetryCallback	telemCb;			// Callback function to indicate success/failure of posting
	void*									userArg;			// Application specific callback data
	char*									pData;				// Pointer to data to upload
	uint32								length;				// Length of data
};
typedef _smart_ptr< STelemetryData > STelemetryDataPtr;

struct ICryTelemetry
{
	// ConfigureRemoteServer
	// Specifies the relevant information for the remote server. Must be called before using any other function in this interface
	// url				- URL for remote server
	// port				- Port used to communicate with server
	virtual bool ConfigureRemoteServer(const char *url, uint16 port) = 0;

	// UploadData
	// Queues data for uploading to remote server. Note data will be deleted after upload completes/fails.
	// return			- True if successfully queued, false indicates internal failure (queue full maybe)
	virtual bool UploadData(STelemetryDataPtr pData) = 0;
};

#endif // __ICRYTELEMETRY_H__
