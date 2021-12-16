#ifndef STAT_CGF_COMPILER
#define STAT_CGF_COMPILER

#include "Cry_Color.h"
#include "IConvertor.h"
#include "MeshCompiler\MeshCompiler.h"

struct ConvertContext;
class CContentCGF;
class CChunkFile;
class CPhysicsInterface;
struct CMaterialCGF;

class CStatCGFCompiler : public IConvertor, public ICompiler
{
public:
	class Error
	{
	public:
		Error (int nCode);
		Error (const char* szFormat, ...);
		const char* c_str() const { return m_strReason.c_str(); }
	protected:
		string m_strReason;
	};

	CStatCGFCompiler();
	~CStatCGFCompiler();

	// IConvertor methods.
	virtual void Init(IConfig* config, const char* exePath) {}
	virtual void DeInit() {}
	virtual int GetNumPlatforms() const;
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual EPlatform GetPlatform(int index) const;
	virtual int GetNumExt() const { return 2; }
	virtual const char* GetExt(int index) const { return index ? "cga" : "cgf"; }
	virtual DWORD GetTimestamp() const;
	virtual IConvertContext* CreateConvertContext() const { return new ConvertContext; }

	// ICompiler methods.
	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	// ICompiler + IConvertor methods.
	virtual void Release();

private:
	void DeleteOldChunks(CContentCGF *pCGF, CChunkFile &chunkFile);
	void DeleteChunkByType(CChunkFile &chunkFile,int nChunkType);
	bool IsLodFile( const string &filename ) const;

private:
	mesh_compiler::CMeshCompiler m_meshCompiler;
	CPhysicsInterface *m_pPhysicsInterface;

	int m_refCount;
};

#endif