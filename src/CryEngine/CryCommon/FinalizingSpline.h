////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   IFacialAnimation.h
//  Version:     v1.00
//  Created:     7/10/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __FinalizingSpline_h__
#define __FinalizingSpline_h__

#include "ISplines.h"

template<class T>
struct base_ptr
{
	base_ptr( T* p = 0 )
		: m_ptr(p) {}

	operator T*()
		{ return m_ptr; }
	operator const T*() const
		{ return m_ptr; }

	T* operator ->()
		{ return m_ptr; }
	const T* operator ->() const
		{ return m_ptr; }

	T& operator *()
		{ return *m_ptr; }
	const T& operator *() const
		{ return *m_ptr; }

	bool operator !() const
		{ return !m_ptr; }
	bool operator +() const
		{ return !!m_ptr; }

protected:
	T*		m_ptr;
};

// deep_ptr: acts as a value, type, copying the pointee value.
template<class T, bool bINIT = false>
struct deep_ptr: base_ptr<T>
{
	~deep_ptr()
		{ delete m_ptr; }

	deep_ptr()
		: base_ptr<T>( bINIT ? new T() : 0 ) {}

	deep_ptr( T* p )
		: base_ptr<T>(p) {}

	deep_ptr( const deep_ptr<T,bINIT>& init )
		: base_ptr<T>( init.m_ptr ? new T(*init.m_ptr) : 0) {}

	deep_ptr& operator=( const deep_ptr<T,bINIT>& init )
	{
		if (init.m_ptr != m_ptr)
		{
			delete m_ptr;
			m_ptr = init.m_ptr ? new T(*init.m_ptr) : 0;
		}
		return *this;
	}

	deep_ptr& operator=( T* p )
	{
		if (p != m_ptr)
		{
			delete m_ptr;
			m_ptr = p;
		}
		return *this;
	}

private:
	using base_ptr<T>::m_ptr;
};


namespace spline
{
	//////////////////////////////////////////////////////////////////////////
	// FinalizingSpline
	//////////////////////////////////////////////////////////////////////////

	template <class Source>
	class	FinalizingSpline
	{
	public:
		using_type(Source, value_type);
		using_type(Source, key_type);

		// Adaptors for base spline functions.
		int num_keys() const
			{ return m_pSource->num_keys(); }
		void resize( int num )
			{ m_pSource->resize(num); }
		int insert_key( float t, const value_type& val )
			{ return m_pSource->insert_key(t, val); }
		void erase( int key )
			{ m_pSource->erase(key); }
		void find_keys_in_range(float startTime, float endTime, int& firstFoundKey, int& numFoundKeys) const
			{ m_pSource->find_keys_in_range(startTime, endTime, firstFoundKey, numFoundKeys); }
		void remove_keys_in_range(float startTime, float endTime)
			{ m_pSource->remove_keys_in_range(startTime, endTime); }
		key_type&	key( int n )
			{ return m_pSource->key(n); }
		const key_type& key( int n ) const
			{ return m_pSource->key(n); }
		void SetModified( bool bOn, bool bSort = false )
			{ m_pSource->SetModified(bOn, bSort ); }

		bool is_updated() const
		{
			return !m_pSource || m_pSource->is_updated();
		}

		void finalize()
		{
			// Optimise by removing source spline data.
			// Any further spline updates will crash 
			// (we could assert for non-null pointer, but crashing has the same effect).
			m_pSource = 0;
		}
		bool is_finalized() const
		{
			return !m_pSource;
		}

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			if (m_pSource)
			{
				pSizer->Add(*m_pSource);
				size_t nSize = m_pSource->mem_size();
				if (nSize)
					pSizer->AddObject(this, nSize);
			}
		}

	protected:

		deep_ptr<Source,true>		m_pSource;
	};

	//////////////////////////////////////////////////////////////////////////
	// LookupTableSpline
	//////////////////////////////////////////////////////////////////////////

	template <class S, class Source>
	class	LookupTableSpline: public FinalizingSpline<Source>
	{
		typedef FinalizingSpline<Source> super_type;
		using super_type::m_pSource;
		using super_type::is_updated;

	public:

		using_type(super_type, value_type);

		LookupTableSpline()
		{ 
			init(); 
		}

		LookupTableSpline( const LookupTableSpline& other )
			: super_type(other)
		{
			init();
			update();
		}
		void operator=( const LookupTableSpline& other )
		{
			super_type::operator=(other);
			update();
		}

		~LookupTableSpline()
		{
			set_table_size(0);
		}

		inline void interpolate( float t, value_type& val )
		{
			if (!is_updated())
				update();
			fast_interpolate( t, val );
		}

		// Additional methods.
		SPU_NO_INLINE void fast_interpolate( float t, value_type& val ) const
		{
			assert(is_updated());
			assert(m_table && m_tableCount);
			t *= (float)m_rangeScale;
			t -= (float)m_indexStart;
			t = clamp(t, 0.f, (float)m_tableCount - 1.001f);
			int i = int(t);
			t -= float(i);
			val = value_type(m_table[i]) * (1.f-t);
			val += value_type(m_table[i+1]) * t;
		}

		inline void min_value( value_type& val ) const
		{
			val = m_minVal;
		}

		// Overrides.
		void finalize()
		{
			if (!is_updated())
				update();
			super_type::finalize();
		}

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			super_type::GetMemoryUsage(pSizer);
			if (m_tableCount > 1)
				pSizer->AddObject(m_table, m_tableCount * sizeof(S));
		}

	protected:

		void update()
		{
			// Construct lookup table.
			m_rangeScale = m_indexStart = 0;

			if (!m_pSource || !m_pSource->num_keys())
			{
				set_table_size(1);
				m_minVal = value_type(1.f);
				return;
			}

			m_pSource->update();
			int num_keys = m_pSource->num_keys();
			if (num_keys > 1)
			{
				enum { nMAX_ENTRIES = 128 };
				enum { nENTRIES_PER_KEY = 8 };

				// Compute optimal number of entries, such that shortest key has nENTRIES_PER_KEY.
				float min_time = m_pSource->time(1) - m_pSource->time(0);
				for (int i = 1; i < num_keys-1; i++)
					min_time = min( min_time, m_pSource->time(i+1) - m_pSource->time(i) );

				// Compute number of entries for entire range, then limit to active range.
				m_rangeScale = int_ceil( div_min( (float)nENTRIES_PER_KEY, min_time, (float)nMAX_ENTRIES ) );
				m_indexStart = int( m_pSource->time(0) * m_rangeScale );
				int indexEnd = int_ceil( m_pSource->time(num_keys-1) * m_rangeScale );

				// Alloc and fill table.
				set_table_size(indexEnd+1 - m_indexStart);
				value_type min_val(1.f);
				float index_scale = 1.f / m_rangeScale;
				for (int i = 0; i < m_tableCount; i++)
				{
					float t = float(i+m_indexStart) * index_scale;
					value_type val;
					m_pSource->interpolate(t, val);
					min_val = min(min_val, val);
					m_table[i] = val;
				}

				// Cache min value at end of table.
				m_minVal = min_val;
			}
			else
			{
				// Single value curve, simply store it in m_minVal (and don't delete).
				set_table_size(1);
				m_minVal = m_pSource->value(0);
			}
		}

		void init()
		{
			m_table = &m_minVal;
			m_tableCount = m_rangeScale = m_indexStart = 0;
		}

		bool is_updated() const
		{
			return super_type::is_updated() && (!m_pSource || m_tableCount > 0);
		}

		void set_table_size(int new_size)
		{
			if (new_size > m_tableCount || new_size <= 1)
			{
				if (m_tableCount > 1)
					delete[] m_table;
				if (new_size > 1)
					m_table = new S[new_size];
				else
					m_table = &m_minVal;
			}
			m_tableCount = check_cast<uint8>(new_size);
		}

		S*			m_table;
		uint8		m_tableCount,
						m_rangeScale,
						m_indexStart;
		S				m_minVal;
	};

	//////////////////////////////////////////////////////////////////////////
	// OptSpline
	//////////////////////////////////////////////////////////////////////////

	/*
		Control points v0 = v(0), va = v(1/3), vb = v(2/3), v1 = v(1)
		Basis vars t, u = 1-t, ttu, uut
		Solve basis coefficients a, b, c, d :
			v(t) = a u + b t + c uut + d utt
		Requires solving linear system:
			{	1,		0,		0,	 0
				2/3,	1/3,	4/27,	2/27
				1/3,	2/3,	2/27,	4/27
				0,		1,		0,		0		} * {a,b,c,d} = {v0,va,vb,v1}
		Result, thanks to Wolfram Alpha:
			inv {{1,0,0,0},{2/3,1/3,4/27,2/27},{1/3,2/3,2/27,4/27},{0,1,0,0}}.{v0,va,vb,v1}
			= {v0, v1, -(9 v0)/2+9 va-(9 vb)/2, -(9 va)/2+9 vb-(9 v1)/2}

			a = v0
			b = v1
			c = 9 (va - (v0 + vb)/2)
			d = 9 (vb - (va + v1)/2)
	*/

	template <class UStorage, class SStorage, class Source>
	class	OptSpline: public FinalizingSpline<Source>
	{
		typedef OptSpline<UStorage, SStorage, Source> self_type;
		typedef FinalizingSpline<Source> super_type;

	public:
		using_type(super_type, value_type);
		using_type(super_type, key_type);
		typedef value_type T;

	protected:
		typedef typename UStorage::store_type	UStore;
		typedef typename SStorage::store_type	SStore;

		using super_type::m_pSource;

		struct OptElem
		{
			float			time_scale;
			SStore		sva, svb;
			UStore		sv1;

			inline void eval(T& val, const UStore& sv0, float t) const
			{
				float u = 1.f - t,
							tu9 = t*u*9.f;

				val = sv0 * (u * UStorage::FromScale());
				val += sv1 * (t * UStorage::FromScale());
				val += sva * (tu9 * u * SStorage::FromScale());
				val += svb * (tu9 * t * SStorage::FromScale());
			}

			// Compute coeffs based on 2 endpoints & slopes.
			void set( float t0, T v0, T s0, float t1, T v1, T s1 )
			{
				time_scale = 1.f / (t1-t0);

				// Evaluate spline at t = 1/3 and 2/3.
				TCoeffBasis<T> coeffs;
				coeffs.set(0.f, v0, s0, 1.f, v1, s1);

				T va, vb;
				coeffs.eval(va, 1.f/3.f);
				coeffs.eval(vb, 2.f/3.f);

				// Construct control values.
				sva = SStorage::ToStorage(va - (v0 + vb)*0.5f);
				svb = SStorage::ToStorage(vb - (va + v1)*0.5f);
				sv1 = UStorage::ToStorage(v1);
			}
		};

	public:

		~OptSpline()
		{ 
			delete[] m_elems; 
		}
		OptSpline()
		{ 
			init(); 
		}
		OptSpline( const self_type& in )
			: super_type(in)
		{ 
			init();
			update();
		}
		self_type& operator=( const self_type& in )
		{
			super_type::operator=(in);
			init();
			update();
			return *this;
		}

		inline void interpolate( float t, value_type& val )
		{
			update();
			fast_interpolate( t, val );
		}

		// Overrides.

		bool is_updated() const
		{
			if (m_pSource)
				return m_pSource->is_updated() && m_time0 >= 0.f;
			return true;
		}

		void finalize()
		{
			update();
			super_type::finalize();
		}

		void GetMemoryUsage(ICrySizer* pSizer) const
		{
			super_type::GetMemoryUsage(pSizer);
			pSizer->AddObject(m_elems, m_elemcount * sizeof(OptElem));
		}

		// Additional methods.
		void fast_interpolate( float t, value_type& val ) const
		{
			assert(is_updated());

			t -= m_time0;
			if (t <= 0.f)
				val = m_value0 * UStorage::FromScale();
			else
			{
				UStore prev_val = m_value0;
				for (range_iter<const OptElem> it(m_elems, m_elems+m_elemcount); it; ++it )
				{
					t *= it->time_scale;
					if (t <= 1.f)
					{
						it->eval( val, prev_val, t );
						return;
					}
					t -= 1.f;
					prev_val = it->sv1;
				}

				// Last point value.
				val = prev_val * UStorage::FromScale();
			}
		}

		inline void min_value( value_type& val ) const
		{
			non_const(*this).update();
			val = m_value0 * UStorage::FromScale();
			for (range_iter<const OptElem> it(m_elems, m_elems+m_elemcount); it; ++it )
				val = min(val, it->sv1 * UStorage::FromScale());
		}

		inline void max_value( value_type& val ) const
		{
			non_const(*this).update();
			val = m_value0 * UStorage::FromScale();
			for (range_iter<const OptElem> it(m_elems, m_elems+m_elemcount); it; ++it )
				val = max(val, it->sv1 * UStorage::FromScale());
		}

	protected:

		void init()
		{
			m_elems = 0;
			m_elemcount = 0;
			m_time0 = -FLT_MAX;
		}

		void update()
		{
			if (is_updated())
				return;
			m_pSource->update();

			delete[] m_elems;
			int num_keys = m_pSource->num_keys();
			if (num_keys > 1)
			{
				m_value0 = UStorage::ToStorage(m_pSource->key(0).value);
				m_time0 = m_pSource->key(0).time;
				float prev_time_scale = 1.f;

				m_elemcount = check_cast<uint8>(num_keys-1);
				m_elems = new OptElem[m_elemcount];
				for (int i = 0; i < m_elemcount; i++)
				{
					// Coefficients for each key.
					const key_type& key = m_pSource->key(i);
					const key_type& next_key = m_pSource->key(i+1);
					m_elems[i].set( key.time, key.value, key.dd, next_key.time, next_key.value, next_key.ds );
					m_elems[i].time_scale /= prev_time_scale;
					prev_time_scale *= m_elems[i].time_scale;
				}
			}
			else
			{
				m_elems = 0;
				m_elemcount = 0;
				m_value0 = UStorage::ToStorage(num_keys == 1 ? m_pSource->key(0).value : T(1.f));
				m_time0 = FLT_MAX;
			}
		}

		OptElem*			m_elems;			// It hurts to use manual array management rather than DynArray, but saves a few bytes.
		uint8					m_elemcount;	// Will combine with m_value0 for TStore types we use.
		UStore				m_value0;
		float					m_time0;
	};
};

#endif // __FinalizingSpline_h__
