#pragma once

class ISymbolTable
{
public:
	virtual ~ISymbolTable() {}

	virtual void GetSymbol(TAddress address, const char*& name, const char*& filename, int& line ) const = 0;

	virtual void FindFileLineMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const = 0;
	virtual void FindFileMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const = 0;
	virtual void FindFunctionMatchingAddresses(std::vector<TAddress>& addressSetOut, TAddress find) const = 0;
};
