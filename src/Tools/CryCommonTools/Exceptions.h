#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <stdexcept>
#include <string>

class BaseException : public std::exception
{
public:
	BaseException(const std::string& msg): msg(msg) {}
	virtual const char* what() const {return msg.c_str();}

private:
	std::string msg;
};

template <typename Tag> class Exception : public BaseException
{
public:
	Exception(const string& msg): BaseException(msg) {}
};

#endif //__EXCEPTIONS_H__
