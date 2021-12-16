/*
 * Copyright 1995, 1996 Perforce Software.  All rights reserved.
 *
 * This file is part of Perforce - the FAST SCM System.
 */

/*
 * strtable.h - a string table, using StrDict interface
 *
 * Classes defined:
 *
 *	StrPtrDict - a dictionary whose values we don't own
 *	StrBufDict - a dictionary whose values we do own
 *
 * Public methods:
 *
 *	Clear() - reset table, making all variables unset
 *	GetVar() - look up variable, return value (or 0 if not set)
 *	SetVar() - set variable/value pair
 *
 * XXX Total dumb duplication of StrPtrDict into StrBufDict. 
 */

struct StrPtrEntry;
struct StrBufEntry;
class VarArray;

class StrPtrDict : public StrDict {

    public:
			StrPtrDict();
			~StrPtrDict();

	// virtuals of StrDict

	StrPtr *	VGetVar( const StrPtr &var );
	void		VSetVar( const StrPtr &var, const StrPtr &val );
	void		VRemoveVar( const StrPtr &var );
	int		VGetVarX( int x, StrRef &var, StrRef &val );
	void		VClear() { tabLength = 0; }

    private:
	
	VarArray	*elems;
	int		tabSize;
	int		tabLength;

} ;

class StrBufDict : public StrDict {

    public:
			StrBufDict();
			StrBufDict( StrDict & dict );
			StrBufDict & operator =( StrDict & dict );
			~StrBufDict();

	int		GetCount()
			{
			    return tabLength;
			}

	// virtuals of StrDict

	StrPtr *	VGetVar( const StrPtr &var );
	void		VSetVar( const StrPtr &var, const StrPtr &val );
	void		VRemoveVar( const StrPtr &var );
	int		VGetVarX( int x, StrRef &var, StrRef &val );
	void		VClear() { tabLength = 0; }

    private:
	
	VarArray	*elems;
	int		tabSize;
	int		tabLength;

} ;

