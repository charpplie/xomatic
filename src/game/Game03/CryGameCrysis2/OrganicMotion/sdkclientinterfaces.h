/*	
 *	=========================================================================================
 *	Copyright(c) 2007 Organic Motion, Inc. All Rights Reserved.
 *	The coded instructions, statements, computer programs, and/or related
 *	material (collectively the "Code") in these files contain unpublished
 *	information proprietary to Organic Motion, Inc., which is protected by
 *	United States of America federal copyright law and by international treaties.
 *	Use, duplication, and or distribution only with written permission from Organic Motion, Inc.
 *	THE CODE IS PROVIDED "AS IS" AND WITHOUT WARRANTY.
 *	=========================================================================================
 */

//=======================================================================================
//	SDKClientInterfaces.h
//		Defines the interfaces with which any developer can communicate with
//		the Organic Motion tracking system.
//=======================================================================================
#pragma once

#include "omsdktypes.h"

class IStreamDataFrame;
class IOMDataReader;
class IOMFrameDataReader;
class IOMTimeDataReader;
class IOMBoneDataReader;
class IOMMeshVertexDataReader;
class IOMMeshIndexDataReader;
class IOMTexDataReader;
class IOMImageDataReader;
class IOMBoneSetupInfo;


/*
*	ISDClient
*		Top-level interface for talking to an Organic Motion data server.
*/
class ISDKClient
{
public:

	/*
	*	Connect()
	*		Initiates the base communication layer with the server. After a successful connection,
	*		a stream can be created, or any non-real-time call can be made.
	*		Parameters:
	*			strServerIP: IP address of the server
	*			ulMilliseconds: number of milliseconds to wait before successfully connecting.
	*							This call blocks for ulMilliseconds, or until a successful connection 
	*							is established.
	*		Returns:
	*			OMOK on success
	*			OMALREADYCONNECTED if already connected
	*			OMFAIL otherwise
	*/
	virtual OMRESULT	Connect(char * strServerIP="127.0.0.1",unsigned long ulMilliseconds=1000) = 0;
	/*
	*	Disconnect()
	*		Shuts down the base communication layer established by Connect().
	*		Destroys any streams present as well, so this is the only necessary call to shut down.
	*		Returns:
	*			OMOK on success
	*			OMALRADYDISCONNECTED if already disconnected
	*			OMFAIL otherwise
	*/
	virtual OMRESULT	Disconnect() = 0;
	/*
	*	IsConnected()
	*		Returns:
	*			OMTRUE if communication with the server is working properly, 
	*			OMFALSE otherwise.
	*/
	virtual uint32		IsConnected() = 0;


	/*
	*	CreateStream()
	*		Once connected, creates a stream specified by the parameters.
	*		For each OMDataKey supplied, only the 'type' field needs to be set.
	*		If the function succeeds, the 'key' field will be greater than or equal zero
	*		if the type request, was accepted, or less than zero if not.
	*		Currently, the OMSDK only supports one stream at a time (though this is to change
	*		in future versions).
	*		Parameters:
	*			pKeys: an array of OMDataKey structures, each with their type fields filled in.
	*			numKeys: the number of keys passed into pKeys
	*		Returns:
	*			OMOK on success 
	*			OMNOTCONNECTED if there is no communication with the server
	*			OMFAIL otherwise
	*/
	virtual OMRESULT	CreateStream(OMDataKey * pKeys,int numKeys) = 0;

	/*
	*	DestroryStream()
	*		Stops the current stream if necessary, then destroys it. 
	*		Does nothing if there is no stream present.
	*		Returns OMOK on success, OMFAIL otherwise.
	*/
	virtual OMRESULT	DestroyStream() = 0;

	/*
	*	StreamCreated()
	*		Returns: OMTRUE is there is a stream present, OMFALSE otherwise.
	*/
	virtual uint32		StreamCreated() = 0;

	/*
	*	StartStream()
	*		Starts the transfer of data
	*		Returns OMOK on success, OMFAIL otherwise (usually if no stream is present).
	*/
	virtual OMRESULT	StartStream() = 0;

	/*
	*	StopStream()
	*		Stops the transfer of data
	*		Returns OMOK on success, OMFAIL otherwise (usually if no stream is present).
	*/
	virtual OMRESULT	StopStream() = 0;

	/*
	*	StreamStarted()
	*		Returns: OMTRUE if the stream has been started, OMFALSE otherwise.
	*/
	virtual uint32		StreamStarted() = 0;

	/*
	*	GetFrame()
	*		Acquires a single frame from data storage, returning it as an IStreamDataFrame.
	*		A millisecond timeout value can be supplied to make the function block for that period
	*		of time. A value of OMINFINITE can be supplied to wait forever. 
	*		This parameter is defaulted to 0, which causes the function to return immediately.
	*
	*		Frame acquisition is sequential, so GetFrame() always gets the NEXT frame available,
	*		NOT the most recent frame. Therefore, while a stream is started, if GetFrame()
	*		is not called at a reasonable pace, the data storage buffer may fill up, and 
	*		prevent any new frames from being recorded until old ones are consumed.
	*		Therefore, if the server is sending data at 60 fps, the client loop should be calling
	*		GetFrame() at least this quickly. 
	*
	*		Parameters:
	*			pFrame - a valid IStreamDataFrame object (generated from the factory)
	*			ulTimeoutMs - amount of time to wait (in milliseconds) before returning with no data.
	*		Returns:
	*			OMOK on success
	*			OMINVALIDARGUMENTS if a NULL pointer has been supplied
	*			OMTIMEOUT if ulTimeoutMs milliseconds elapsed without any new data
	*			OMFAIL otherwise
	*/
	virtual OMRESULT	GetFrame(IStreamDataFrame * pFrame,unsigned long ulTimeoutMs=0) = 0;

	/*
	*	GetSystemVersionNumber()
	*		Obtains the version number of the server.
	*		This is a non-real-time query, so it can be called as long as a Connect() call
	*		has been successfully made.
	*		Parameters:
	*			version - the version number of the system. 
	*		Returns OMOK on success, OMFAIL otherwise.
	*/
	virtual OMRESULT	GetSystemVersionNumber(OMSystemVersion * version) = 0;

	/*
	*	GetSDKServerVersionNumber() and
	*	GetSDKClientVersionNumber()
	*		Obtains the version numbers of the SDK subsystems of the server and client
	*		respectively. This is a non-real-time query, so it can be called as long as 
	*		a Connect() call has been successfully made.
	*		Parameters:
	*			version - the version number of the SDK subsystem
	*		Returns 
	*			OMOK on success 
	*			OMINVALIDARGUMENTS if NULL pointer is supplied
	*			OMFAIL otherwise.
	*/
	virtual OMRESULT	GetSDKServerVersionNumber(OMSDKVersion * version) = 0;
	virtual OMRESULT	GetSDKClientVersionNumber(OMSDKVersion * version) = 0;

	/*
	*	GetBoneSetupInfo()
	*		Obtains a pointers to an IOMBoneSetupInfo interface, to allow investigation
	*		of the current setup of the Organic Motion skeletal structure.
	*		This is a non-real-time query, so it can be called as long as a Connect() call
	*		has been successfully made.
	*		Parameters: 
	*			ppBoneInfo - double pointer to an IOMBoneSetupInfo interface. This 
	*			pointer does not require destruction, but becomes invalid when the ISDKClient
	*			object is destroyed.
	*		Returns:
	*			OMOK on success 
	*			OMINVALIDARGUMENTS on a NULL double pointer
	*			OMFAIL otherwise
	*/
	virtual	OMRESULT	GetBoneSetupInfo(IOMBoneSetupInfo ** ppBoneInfo) = 0;

};

/*
*	IStreamDataFrame
*		Interface for a packet of data that may contain several different 
*		Organic Motion data types.
*/
class IStreamDataFrame
{
public:

	/*
	*	OpenReader()
	*		Retuns a pointer to an IOMDataReader interface, that will allow typed access
	*		to the data within a frame The actual pointer type will depend on the type 
	*		field of the supplied OMDataKey, so a cast to the proper derived interface 
	*		will be necessary.
	*		Parameters:
	*			dataKey - dataKey specifying the type and instance of data within a frame
	*			pReader - double pointer to an IOMDataReader interface.
	*		Returns 
	*			OMOK on success 
	*			OMINVALIDARGUMENTS on a NULL double pointer
	*			OMFAIL otherwise
	*/
	virtual OMRESULT OpenReader(OMDataKey dataKey,IOMDataReader ** pReader)=0;

	/*
	*	CloseReader()
	*		Invalidates the IOMDataReader passed in. 
	*		pReader - pointer to the IOMDataReader to be closed.
	*		Returns 
	*			OMOK on success 
	*			OMINVALIDARGUMENTS on a NULL pointer
	*			OMFAIL otherwise	
	*/
	virtual OMRESULT CloseReader(IOMDataReader * pReader)=0;


	/*
	*	The following functions perform the same operation as above, 
	*	but are pre-typed. Destruction on the returned pointer is not necessary,
	*	but the pointer becomes invalid after the IStreamDataFrame is passed into
	*	a new GetFrame() call, or after the IStreamDataFrame object is destroyed.
	*
	*	The default parameter of -1 requests the first instance of that data type
	*	in a frame. If a specific key is supplied, the function will look for that
	*	specific instance of the data. Therefore, if only one instance of a type has been
	*	requested (i.e., one OMDataKey with that type has been successfully passed 
	*	into CreateStream()), the default parameter can be used without hesitation.
	*/
	virtual IOMTimeDataReader * TimeData(int key=-1)=0;
	virtual IOMBoneDataReader * BoneData(int key=-1)=0;

	//not yet supported
	virtual IOMMeshVertexDataReader * MeshVertexData(int key=-1)=0;
	//not yet supported
	virtual IOMMeshIndexDataReader * MeshIndexData(int key=-1)=0;
	//not yet supported
	virtual IOMTexDataReader * TexData(int key=-1)=0;

	virtual IOMImageDataReader * ImageData(int key=-1)=0;

};

/*
*	IOM<type>DataReader
*		Several interfaces for strongly-typed access to data within a frame.
*/
class IOMDataReader
{
public:
	/*
	*	GetType()
	*	Returns an OMDataType indicating the type of data this IOMDataReader can read.
	*/
	virtual OMDataType GetType() const=0;
};

/*
*	IOMTimeDataReader
*		Please see omsdktypes.h for explanation of what each function in this interface returns.
*/
class IOMTimeDataReader : public IOMDataReader
{
public:
	virtual unsigned int	GetFrameId() = 0;
	virtual __int64			GetTimeStamp() = 0;
	virtual __int64			GetTimeStampFreq() = 0;
	virtual void			GetAllTimeInfo(unsigned int & iFrameId,
		__int64 & timestamp,
		__int64 & timestampFreq)=0;
};

/*
*	IOMTimeDataReader
*/
class IOMBoneDataReader : public IOMDataReader
{
public:
	/*
	*	GetNumBones()
	*		Returns the number of bones found in the frame. In normal circumstances,
	*		this will equal the number of bones returned by the non-real-time function 
	*		in IOMBoneSetupInfo.
	*/
	virtual int				GetNumBones() = 0;
	/*
	*	GetBoneMatrix() and GetAllBoneMatrices()
	*		Are the singular- and multiple- version of the same function.
	*		GetBoneMatrix() returns one bone matrix, specified by 'index', into 'mat'
	*		GetAllBoneMatrices() returns all (when numMatrices is -1) or some (when numMatrices > 0)
	*							 into 'mats.'
	*		Return:
	*			OMOK	on success
	*			OMINVALIDARGUMENTS	if a NULL pointer is supplied, 
	*								or if index if out of range (0 to max bones-1)	[for GetBoneMatrix]
	*								or if numMatrices is too large (1 to max bones)	[for GetAllBoneMatrices]
	*			OMFAIL	otherwise.
	*
	*/
	virtual OMRESULT		GetBoneMatrix(int index, OMBoneMatrix * mat) = 0;
	virtual OMRESULT		GetAllBoneMatrices(OMBoneMatrix * mats,int numMatrices=-1) = 0;
};


/*
*	class IOMMeshVertexDataReader
*		not yet supported
*/
class IOMMeshVertexDataReader : public IOMDataReader
{
public:
	virtual int			GetNumVertices() = 0;
	virtual OMRESULT	GetVertex(int index, OMMeshVertex * vertex) = 0;
	virtual OMRESULT	GetAllVertices(OMMeshVertex * vertex, int numVertices=-1) = 0;
};

/*
*	class IOMMeshIndexDataReader
*		not yet supported
*/
class IOMMeshIndexDataReader : public IOMDataReader
{
public:
	virtual int			GetNumIndices() = 0;
	virtual OMRESULT	GetIndex(int index, OMMeshIndex * meshIndex) = 0;
	virtual OMRESULT	GetAllIndices(OMMeshIndex * meshIndices, int numIndices=-1) = 0;
};

/*
*	class IOMTexDataReader
*		not yet supported
*/
class IOMTexDataReader : public IOMDataReader
{
public:
	virtual int			GetTexResX() = 0;
	virtual int			GetTexResY() = 0;
	virtual int			GetAllTexData(OMPixelBW * texData, int numPixels) = 0;
};

/*
*	class IOMImageDataReader
*/
class IOMImageDataReader : public IOMDataReader
{
public: 
	virtual OMRESULT GetImageRes(OMTexResWHD * res) = 0;
	virtual OMRESULT GetSourceId(unsigned int * uiSourceId) = 0;
	virtual OMRESULT GetSourceName(unsigned char * name, int maxNameSize) = 0;
	virtual OMRESULT GetImageId(unsigned int * uiImageId) = 0;
	virtual OMRESULT GetImageData(OMPixelBW * imageData, int numPixels) = 0;
};

/*
*	IOMBoneSetupInfo
*		Interface for querying information about the bones.
*		The calls in this interface are non-realtime, so they can be made 
*		after a successful call to ISDKClient::Connect()
*/
class IOMBoneSetupInfo
{
public:
	/*
	*	GetNumBones()
	*		Obtains the number of bones in the current skeletal structure.
	*		Returns OMOK on success, OMFAIL otherwise.
	*/
	virtual OMRESULT	GetNumBones(int *numBones) = 0;
	/*
	*	GetBoneNames()
	*		Obtains a list of the names of each of the bones in the current skeleton structure
	*		Parameters:
	*			names - an array of char pointers, each a string of max size <maxNameSize>
	*			maxNameSize - the number of characters allocated for each string in 'names.'
	*			numBones -	[in] the number of bones requested
	*						[out] the minimum of the requested number of bones, and the 
	*							number of bones currently supported (i.e. the value 
	*							returned by GetNumBones())
	*		Returns OMOK on success, OMFAIL otherwise.
	*/
	virtual	OMRESULT	GetBoneNames(char * names,int maxNameSize,int & numBones) = 0;
	/*
	*	GetBoneParentList()
	*		Obtains a list of indices that represent the parent of each bone. A value of -1
	*		in the list indicates that the bone has no parent.
	*		Example: parentList[5] returns 4
	*			Index 5 is the left foot, index 4 is the left lower leg. 
	*			So this indicates that left foot's parent is the lowerleft leg.
	*
	*		Parameters:
	*			parentlist	array of ints, each representing the index of the parent bone
	*			numBones -	[in] the number of bones requested
	*						[out] the minimum of the requested number of bones, and the 
	*							number of bones currently supported (i.e. the value 
	*							returned by GetNumBones())
	*		Returns OMOK on success, OMFAIL otherwise.
	*/	
	virtual	OMRESULT	GetBoneParentList(int * parentList,int & numBones) = 0;
	/*
	*	GetBoneSizeList()
	*		Obtains a list of OMBoneDimension structures, each representing the size of a bone.
	*		Parameters:
	*			boneDimList - array of OMBoneDimension structures
	*			numBones -	[in] the number of bones requested
	*						[out] the minimum of the requested number of bones, and the 
	*							number of bones currently supported (i.e. the value 
	*							returned by GetNumBones())
	*		Returns OMOK on success, OMFAIL otherwise.
	*/	
	virtual	OMRESULT	GetBoneSizeList(OMBoneDimension * boneDimList,int & numBones) = 0;
};