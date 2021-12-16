
#ifndef __MAYANAMEPOOL_H__
#define __MAYANAMEPOOL_H__

class mayaNamePool
{
public:
	mayaNamePool();
	virtual ~mayaNamePool();

	void *getPointer( const char *name );
	void *getPointer( const std::string &name );

private:
	std::vector< std::string* > m_namePool;
};

#endif // __MAYANAMEPOOL_H__