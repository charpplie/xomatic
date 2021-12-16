#include "StdAfx.h"
#include "ICompressionPolicy2.h"
#include "ArithModel.h"
#include "BoolCompress2.h"

class CAdaptiveBoolPolicy2
{
public:
	typedef CBoolCompress2::TMemento TMemento;

	bool Load( XmlNodeRef node, const string& filename )
	{
		return true;
	}

	bool ReadMemento( TMemento& memento, CByteInputStream& in ) const
	{
		m_bool.ReadMemento(memento, in);
		return true;
	}

	bool WriteMemento( TMemento memento, CByteOutputStream& out ) const
	{
		m_bool.WriteMemento(memento, out);
		return true;
	}

	void InitMemento( TMemento& memento ) const
	{
		m_bool.InitMemento( memento );
	}

	bool UpdateMemento( TMemento& memento, bool value ) const
	{
		m_bool.UpdateMemento(memento, value);
		return true;
	}
	bool ReadValue( TMemento memento, CCommInputStream& in, CArithModel * pModel, uint32 age, bool& value ) const
	{
		return m_bool.ReadValue(memento, in, value);
	}
	bool WriteValue( TMemento memento, CCommOutputStream& out, CArithModel * pModel, uint32 age, bool value ) const
	{
		return m_bool.WriteValue(memento, out, value);
	}

	template <class T>
	bool UpdateMemento( TMemento& memento, T value ) const
	{
		NetWarning("BoolPolicy: not implemented for generic types");
		return false;
	}
	template <class T>
	bool ReadValue( TMemento memento, CCommInputStream& in, CArithModel * pModel, uint32 age, T& value ) const
	{
		NetWarning("BoolPolicy: not implemented for generic types");
		return false;
	}
	template <class T>
	bool WriteValue( TMemento memento, CCommOutputStream& out, CArithModel * pModel, uint32 age, T value ) const
	{
		NetWarning("BoolPolicy: not implemented for generic types");
		return false;
	}

	void GetMemoryStatistics(ICrySizer* pSizer) const
	{
		SIZER_COMPONENT_NAME(pSizer, "CAdaptiveBoolPolicy");
		pSizer->Add(*this);
	}

#if NET_PROFILE_ENABLE
	int GetBitCount(bool value)
	{
		return m_bool.GetBitCount();
	}

	template <class T>
	int GetBitCount(T value)
	{
		return 0;
	}
#endif
	
private:
	CBoolCompress2 m_bool;
};

REGISTER_COMPRESSION_POLICY2(CAdaptiveBoolPolicy2, "AdaptiveBool2");
