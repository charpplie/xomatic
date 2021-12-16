#ifndef __DXPSERROR__
#define __DXPSERROR__

#include <string>

struct CServerError
{
	CServerError( const std::string &in_text,const std::string &in_compilerOutput) : text(in_text),compilerOutput(in_compilerOutput) {}

	std::string text;
	std::string compilerOutput;
};

#define CrySimple_ERROR(X) throw CServerError(X,"")
#define CryCompiler_ERROR(X,CompilerOutput) throw CServerError(X,CompilerOutput)

#define CrySimple_SECURE_START try{
#define CrySimple_SECURE_END }catch(const CServerError err){printf(err.text.c_str());}

//#define CrySimple_ERROR(X) printf("%s\n", std::string(X).c_str())
//
//#define CrySimple_SECURE_START
//#define CrySimple_SECURE_END

#endif
