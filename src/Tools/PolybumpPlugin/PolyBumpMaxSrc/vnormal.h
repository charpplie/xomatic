// originally from the MAX SDK
//
// * m.m. modified because of a problem with one mesh

class VNormal 
{
public:
	Point3			norm;
	DWORD				smooth;
	VNormal *		next;
	BOOL				init;

	//! constructor
	VNormal()
	{
		smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);
	}

	//! constructor
	VNormal( Point3 &n,DWORD s )
	{
		next=NULL;init=TRUE;norm=n;smooth=s;
	}

	//! destructor
	~VNormal() 
	{
		delete next;
	}

	//!
	void AddNormal( Point3 &n,DWORD s );

	//!
	//! /return normalized result
	Point3 GetNormal( DWORD s );

	//!
	void Normalize();
};

 
void VNormal::AddNormal( Point3 &n,DWORD s )
{
	if(!(s&smooth) && init)
	{
		if (next) next->AddNormal(n,s);
		else 
		{
			next = new VNormal(n,s);
		}
	} 
	else 
	{
		norm   += n;
		smooth |= s; 
		init    = TRUE;
	}
}

Point3 VNormal::GetNormal( DWORD s )
{
	Point3 ret;

	if (smooth&s || !next) ret=norm;
	else ret=next->GetNormal(s);	

	ret=ret.Normalize();				// neccessary?
	return(ret);
}


void VNormal::Normalize()
{
	VNormal *ptr = next, *prev = this;

	while(ptr) 
	{
		if(ptr->smooth&smooth)						// m.m.  removed this optimization, because we had some problems with a mesh
			norm += ptr->norm;

		ptr = ptr->next;
/*
		if(ptr->smooth&smooth) 
		{
			norm += ptr->norm;
			prev->next = ptr->next;

			assert(ptr!=prev);
			assert(ptr!=this);

			delete ptr;
			ptr = prev->next;
		} 
		else 
		{
			prev = ptr;
			ptr  = ptr->next;
		}
*/
	}
	norm = ::Normalize(norm);
	if(next) next->Normalize();
}


