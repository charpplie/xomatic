/*=============================================================================
RemoteCompiler.h : socket wrapper for shader compile server connections
Copyright (c) 2008 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Michael Kopietz

=============================================================================*/

#include "StdAfx.h"
#include "RemoteCompiler.h"














	#pragma comment(lib,"wsock32.lib") 


namespace NRemoteCompiler
{

uint32 CShaderSrv::m_LastWorkingServer=0;

CShaderSrv::CShaderSrv()
{
	Init();
}

void CShaderSrv::Init()
{
#ifdef _MSC_VER
	WSADATA Data;
	if(WSAStartup(MAKEWORD(2,0),&Data))
	{
		iLog->Log("ERROR: CShaderSrv::Init: Could not init root socket\n");
		return;
	}
#endif
}

CShaderSrv& CShaderSrv::Instance()
{
	static CShaderSrv g_ShaderSrv;
	return g_ShaderSrv;
}

string CShaderSrv::CreateXMLNode(const string& rTag,const string& rValue)	const
{
	string Tag=rTag;
	Tag+="=\"";
	Tag+=rValue;
	Tag+="\" ";
	return Tag;
}

/*string CShaderSrv::CreateXMLDataNode(const string& rTag,const string& rValue)	const
{
	string Tag="<";
	Tag+=rTag;
	Tag+="><![CDATA[";
	Tag+=rValue;
	Tag+="]]>";
	return Tag;
}*/

string CShaderSrv::TransformToXML(const string& rIn)	const
{
	string Out;
	for(size_t a=0,Size=rIn.size();a<Size;a++)
	{
		const char C=rIn.c_str()[a];
		if(C=='&')
			Out+="&amp;";
		else
		if(C=='<')
			Out+="&lt;";
		else
		if(C=='>')
			Out+="&gt;";
		else
		if(C=='\"')
			Out+="&quot;";
		else
		if(C=='\'')
			Out+="&apos;";
		else
			Out+=C;
	}
	return Out;
}

bool CShaderSrv::CreateRequest(	std::vector<uint8>&	rVec,
																const char*	pProfile,
																const char*	pProgram,
																const char*	pEntry,
																const char*	pCompileFlags,
																const char* pShaderRequestLine)	const
{

	string Request="<?xml version=\"1.0\"?><Compile ";
	Request	+=	CreateXMLNode("Profile",TransformToXML(pProfile));
	Request	+=	CreateXMLNode("Program",TransformToXML(pProgram));
	Request	+=	CreateXMLNode("Entry",TransformToXML(pEntry));
	Request	+=	CreateXMLNode("CompileFlags",TransformToXML(pCompileFlags));
	Request	+=	CreateXMLNode("ShaderRequest",TransformToXML(pShaderRequestLine));
	Request	+=	" />";
	rVec	=	std::vector<uint8>(Request.c_str(),&Request.c_str()[Request.size()+1]);
	return true;
}

bool CShaderSrv::Compile(	std::vector<uint8>&	rVec,
														const char* pProfile,
														const char* pProgram,
														const char* pEntry,
														const char* pCompileFlags,
														const char* pShaderRequestLine)	const
{
	ECompileError errCompile;

	std::vector<uint8>	CompileData;

	int nRetries = 3;
	do 
	{
		if(!CreateRequest(CompileData,pProfile,pProgram,pEntry,pCompileFlags,pShaderRequestLine))
		{
			iLog->LogError("ERROR: CShaderSrv::Compile: failed composing Request XML\n");
			return false;
		}

		errCompile = Compile(CompileData);
	} while (errCompile == eCompileRecvFailed && nRetries-- > 0);
	
	rVec	=	CompileData;

	if (errCompile != eCompileOK)
	{
		const char *why = (errCompile == eCompileNetworkError || errCompile == eCompileSendFailed || errCompile == eCompileRecvFailed) ? "Network Error" : "";
		iLog->LogError("ERROR: CShaderSrv::Compile: failed to compile %s (%s)",pEntry,why);
		return false;
	}

	return true;
}

bool CShaderSrv::Send(SOCKET Socket, const char* pBuffer,uint32 Size)	const
{
	size_t w;
	size_t wTotal = 0;
	while(wTotal<Size)
	{
		w = send(Socket, pBuffer + wTotal, Size - wTotal, 0);
		if (w < 0)
		{
			iLog->Log("ERROR:CShaderSrv::Send failed (%d, %d)\n",	(int)w, WSAGetLastError());
			return false;
		}
		wTotal += (size_t)w;
	}
	return true;
}

bool CShaderSrv::Send(SOCKET Socket,std::vector<uint8>& rCompileData)	const
{
	const uint64 Size	=	static_cast<uint32>(rCompileData.size());
	return	Send(Socket,(const char*)&Size,8) &&
					Send(Socket,(const char*)&rCompileData[0],static_cast<uint32>(Size));
}

bool CShaderSrv::Recv(SOCKET Socket,std::vector<uint8>& rCompileData)	const
{
//	const uint32 Size	=	static_cast<uint32>(rCompileData.size());
//	return	Send(Socket,(const char*)&Size,4) ||
//		Send(Socket,(const char*)&rCompileData[0],Size);


	//	delete[] optionsBuffer;
	uint32 nMsgLength = 0;
	uint32 nTotalRecived = 0;
	const size_t	BLOCKSIZE	=	16*1024;
	const size_t	SIZELIMIT	=	1024*1024;
	rCompileData.resize(0);
	rCompileData.reserve(64*1024);
	int CurrentPos	=	0;
	while(rCompileData.size()<SIZELIMIT)
	{
		rCompileData.resize(CurrentPos+BLOCKSIZE);
		int Recived = recv(Socket,reinterpret_cast<char*>(&rCompileData[CurrentPos]),BLOCKSIZE, 0);

		if (Recived >= 0)
			nTotalRecived += Recived;

		if (nTotalRecived > 4)
			nMsgLength = *(uint32*)&rCompileData[0] + 4;

		if(Recived == 0 || nTotalRecived == nMsgLength)
		{
			rCompileData.resize(nTotalRecived);
			break;
		}
		if(Recived < 0)
		{
			iLog->LogError("ERROR: CShaderSrv::Compile:  error in recv() from remote server at offset %lu: error %li, sys_net_errno=%i\n",(unsigned long)rCompileData.size(),(long)Recived,WSAGetLastError());
			return false;
		}
		CurrentPos	+=	Recived;
	}
//	iLog->Log("Recv = %d",(unsigned long)rCompileData.size() );
	if (rCompileData.size() > 4)
	{
		memmove( &rCompileData[0],&rCompileData[4],rCompileData.size()-4 );
		rCompileData.resize(rCompileData.size()-4);
	}
	return rCompileData.size()!= 0 && rCompileData.size()!=SIZELIMIT;
}

void CShaderSrv::Tokenize(tdEntryVec& rRet,const string& Tokens,const string& Separator)	const
{
		rRet.clear();
		string::size_type Pt;
		string::size_type Start	= 0;
		string::size_type SSize	=	Separator.size();

		while((Pt = Tokens.find(Separator,Start)) != string::npos)
		{
			string  SubStr	=	Tokens.substr(Start,Pt-Start);
			rRet.push_back(SubStr);
			Start = Pt + SSize;
		}

		rRet.push_back(Tokens.substr(Start));
}

CShaderSrv::ECompileError CShaderSrv::Compile(std::vector<uint8>& rCompileData)	const
{
	SOCKET Socket	=	SOCKET_ERROR;
	int Err = SOCKET_ERROR;

	tdEntryVec ServerVec;
	if(gRenDev->CV_r_ShaderCompilerServer)
		Tokenize(ServerVec,gRenDev->CV_r_ShaderCompilerServer->GetString(),";");

	if(ServerVec.empty())
		ServerVec.push_back("localhost");
	
	//connect
	for(uint32 nRetries=m_LastWorkingServer;nRetries<m_LastWorkingServer+ServerVec.size()+6;nRetries++)
	{
		string Server	=	ServerVec[nRetries%ServerVec.size()];
		Socket = socket(AF_INET, SOCK_STREAM, 0);
		if(Socket == INVALID_SOCKET)
		{
			iLog->LogError("ERROR: CShaderSrv::Compile: can't create client socket: error %i\n",Socket);
			return eCompileNetworkError;
		}
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof addr);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(gRenDev->CV_r_ShaderCompilerPort);
		const char* pHostName	=	Server.c_str();
		bool IP=true;
		for(uint32 a=0,Size=strlen(pHostName);a<Size;a++)
			IP&=(pHostName[a]>='0' && pHostName[a]<='9') || pHostName[a]=='.' ;
		if(IP)
			addr.sin_addr.s_addr = inet_addr(pHostName);
		else
		{


































			hostent* pHost	= gethostbyname( pHostName );
			if (!pHost)
			{
				break;
			}
			addr.sin_addr.s_addr = ((struct in_addr *)(pHost->h_addr))->s_addr;

		}

		Err = connect(Socket, (struct sockaddr *)&addr, sizeof addr);
		if(Err>=0)
		{
			m_LastWorkingServer=nRetries%ServerVec.size();
			break;
		}
		if(Err<0)
		{
			iLog->LogError("ERROR: CShaderSrv::Compile: could not connect to %s\n", Server.c_str());
			//iLog->LogError("ERROR: CShaderSrv::Compile: can't connect to cgserver: error %i, sys_net_errno=%i, retrying %d\n", Err, WSAGetLastError(),nRetries);
			//socketclose(s);
			//return (size_t)-1;
			struct timeval tv;
			struct fd_set emptySet;
			FD_ZERO(&emptySet);
			tv.tv_sec = 1;
			tv.tv_usec = 0;



			closesocket(Socket);
			Socket = INVALID_SOCKET;
			//return eCompileNetworkError;
		}
	}

	if (Socket == INVALID_SOCKET)
	{
		iLog->LogError("ERROR: CShaderSrv::Compile: can't connect to cgserver: error %i, sys_net_errno=%i\n", Err, WSAGetLastError() );
		return eCompileNetworkError;
	}

	if(!Send(Socket,rCompileData))
	{
		closesocket(Socket);
		return eCompileSendFailed;
	}

	if (!Recv(Socket,rCompileData) && rCompileData.size() < 4)
	{
		closesocket(Socket);
		return eCompileRecvFailed;
	}
	closesocket(Socket);

	if (rCompileData.size() < 4)
		return eCompileFailed;
	
	// Decompress incoming shader data
	std::vector<uint8> rCompressedData;
	rCompressedData.swap(rCompileData);

	uint32 nSrcUncompressedLen = *(uint32*)&rCompressedData[0];
	SwapEndian(nSrcUncompressedLen);

	size_t nUncompressedLen = (size_t)nSrcUncompressedLen;

	rCompileData.resize(nUncompressedLen);
	if (nUncompressedLen > 1000000)
	{
		// Shader too big, something is wrong.
		return eCompileFailed;
	}
	if (nUncompressedLen > 0)
	{
		if (!gEnv->pSystem->DecompressDataBlock( &rCompressedData[4],rCompressedData.size()-4,&rCompileData[0],nUncompressedLen ))
			return eCompileFailed;
	}

	if (rCompileData.size() == 0 || strncmp( (char*)&rCompileData[0],"[ERROR]",MIN(rCompileData.size(),7)) == 0)
		return eCompileFailed;
	
	return eCompileOK;
}

}

