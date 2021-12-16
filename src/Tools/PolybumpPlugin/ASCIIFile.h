// ASCIIFile.h: Schnittstelle für die Klasse CASCIIFile.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASCIIFILE_H__6D4E92CB_AC6C_4ED7_BE1C_FFF26A95937C__INCLUDED_)
#define AFX_ASCIIFILE_H__6D4E92CB_AC6C_4ED7_BE1C_FFF26A95937C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// 0 Byte terminierte char ASCII Datei
// für FILE-IO

typedef unsigned long DWORD;
 
class CASCIIFile  
{
public:
	//! constructor
									CASCIIFile					();

	//! copy construtor
									CASCIIFile					( CASCIIFile &a );

	//! destructor
	virtual					~CASCIIFile					();												

	//! assignment operator
 const CASCIIFile operator=						( const CASCIIFile &a );

	void						CoverThisData				( char *ptr, DWORD size );		// altes freigeben, neues übernehmen, keine Kopie machen,
																		// size ist ohne Nullterm., Data soll nullterminiert sein

	//! get data size (without zero termination byte)
	DWORD						GetDataSize					( void );

	//! get memory access
	char *					GetDataPtr					( void );

	//! free data
	void						ReleaseData					( void );

	static bool			IO_GetAvailability	( const char *pathname );
	static DWORD		IO_GetFileSize			( const char *pathname );

	bool						IO_SaveASCIIFile		( const char *pathname );	
	bool						IO_LoadASCIIFile		( const char *pathname );

protected:
	char		*m_Data;				// Zeiger auf malloc Speicher
	DWORD		m_Size;					// einschließlich terminierendes 0Byte (sonst 0)
};

#endif // !defined(AFX_ASCIIFILE_H__6D4E92CB_AC6C_4ED7_BE1C_FFF26A95937C__INCLUDED_)
