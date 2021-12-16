#ifndef __EXPORTFILETYPE_H__
#define __EXPORTFILETYPE_H__

#define CRY_FILE_TYPE_NONE		0x0000
#define CRY_FILE_TYPE_CGF		0x0001
#define CRY_FILE_TYPE_CGA		0x0002
#define CRY_FILE_TYPE_CHR		0x0004
#define CRY_FILE_TYPE_CAF		0x0008
#define CRY_FILE_TYPE_ANM		0x0010


namespace ExportFileTypeHelpers
{
	const char* CryFileTypeToString(int cryFileType);
	int StringToCryFileType(const char* str);
};


#endif

