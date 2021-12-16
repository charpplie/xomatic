/*=============================================================================
  RemoteCompiler.h : socket wrapper for shader compile server connections
  Copyright (c) 2008 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Michael Kopietz

=============================================================================*/

#ifndef REMOTECOMPILER_H
#define REMOTECOMPILER_H

namespace NRemoteCompiler
{






typedef std::vector<string>						tdEntryVec;


class CShaderSrv
{
protected:
	enum ECompileError
	{
		eCompileOK,
		eCompileFailed,
		eCompileNetworkError,
		eCompileSendFailed,
		eCompileRecvFailed,
	};
	static	uint32			m_LastWorkingServer;
											CShaderSrv();

	bool								Send(SOCKET Socket, const char* pBuffer,uint32 Size)	const;
	bool								Send(SOCKET Socket,std::vector<uint8>& rCompileData)	const;
	bool								Recv(SOCKET Socket,std::vector<uint8>& rCompileData)	const;

	void								Tokenize(tdEntryVec& rRet,const string& Tokens,const string& Separator)	const;
	string							TransformToXML(const string& rIn)	const;
	string							CreateXMLNode(const string& rTag,const string& rValue)	const;
//	string							CreateXMLDataNode(const string& rTag,const string& rValue)	const;

	bool								CreateRequest(std::vector<uint8>&	rVec,
																		const char* pProfile,
																		const char* pProgram,
																		const char* pEntry,
																		const char* pCompileFlags,
																		const char* pShaderRequestLine)	const;
	ECompileError       Compile(std::vector<uint8>&	rCompileData)	const;

	void								Init();
public:
	bool                Compile(	std::vector<uint8>&	rVec,
																const char* pProfile,
																const char* pProgram,
																const char* pEntry,
																const char* pCompileFlags,
																const char* pShaderRequestLine)	const;

	static CShaderSrv&	Instance();

};
}

#endif

