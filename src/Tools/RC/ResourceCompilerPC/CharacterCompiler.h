#ifndef CHARACTER_COMPILER
#define CHARACTER_COMPILER

#include "IConvertor.h"
#include "MeshCompiler\MeshCompiler.h"

struct ConvertContext;
class CContentCGF;
class CChunkFile;
class CPhysicsInterface;
class ICryXML;



class CharacterCompiler : public IConvertor, public ICompiler
{
public:
	class Error
	{
	public:
		Error (int nCode);
		Error (const char* szFormat, ...);
		const char* c_str()const {return m_strReason.c_str();}
	protected:
		string m_strReason;
	};

	CharacterCompiler(ICryXML * pXml);
	~CharacterCompiler();

	// IConvertor methods.
	virtual void Init(IConfig* config, const char* exePath) {}
	virtual void DeInit() {}
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual int GetNumPlatforms() const;
	virtual EPlatform GetPlatform( int index ) const;
	virtual int GetNumExt() const { return 1; };
	virtual const char* GetExt( int index ) const { return "chr"; };
	virtual DWORD GetTimestamp() const;
	virtual IConvertContext* CreateConvertContext() const {return new ConvertContext;}

	// ICompiler methods.
	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	// ICompiler + IConvertor methods.
	void Release();

	CContentCGF* MakeCompiledCGF( CContentCGF *pCGF );

private:
	void DeleteOldChunks( CContentCGF *pCGF,CChunkFile &chunkFile );
	// Physicalize all meshes in cgf.
	bool Physicalize( CContentCGF *pCGF );

private:
	mesh_compiler::CMeshCompiler m_meshCompiler;
	CPhysicsInterface *m_pPhysicsInterface;

	ICryXML * m_pXML;

	int m_refCount;
};

#endif
