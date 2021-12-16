//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File Name        : 2DSpline.h
//  Author           : Jaewon Jung
//  Time of creation : 12/18/2009   15:35
//  Compilers        : VS2008
//  Description      : Classes for 2D Bezier Spline curves
//  Notice           : <some extra helpfull information>
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __2DSPLINE_H__
#define __2DSPLINE_H__

#include <ISplines.h>

namespace spline
{
	/**	Bezier spline key extended for tangent unify/break.
	*/
	template	<class T>
	struct SplineKeyEx :  public SplineKey<T>
	{
		float theta_from_dd_to_ds;
		float scale_from_dd_to_ds;

		void ComputeThetaAndScale() { assert(0); }
		void SetOutTangentFromIn() { assert(0); }
		void SetInTangentFromOut() { assert(0); }

		SplineKeyEx() : theta_from_dd_to_ds(gf_PI), scale_from_dd_to_ds(1.0f) {}
	};

	template <>
	inline void SplineKeyEx<Vec2>::ComputeThetaAndScale()
	{
		scale_from_dd_to_ds = (ds.GetLength()+1.0f)/(dd.GetLength()+1.0f);
		float out = fabs(dd.x) > 0.000001f ? cry_atanf(dd.y/dd.x) : (dd.x*dd.y>=0?gf_PI/2.0f:-gf_PI/2.0f);
		float in  = fabs(ds.x) > 0.000001f ? cry_atanf(ds.y/ds.x) : (ds.x*ds.y>=0?gf_PI/2.0f:-gf_PI/2.0f);
		theta_from_dd_to_ds = in + gf_PI - out;
	}

	template<>
	inline void SplineKeyEx<Vec2>::SetOutTangentFromIn()
	{
		assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED);
		float outLength = (ds.GetLength()+1.0f) / scale_from_dd_to_ds - 1.0f;
		float in  = fabs(ds.x) > 0.000001f ? cry_atanf(ds.y/ds.x) : (ds.x*ds.y>=0?gf_PI/2.0f:-gf_PI/2.0f);
		dd.x = 1.0f;
		dd.y = cry_tanf(in + gf_PI - theta_from_dd_to_ds);
		dd.Normalize();
		dd *= outLength;
	}

	template<>
	inline void SplineKeyEx<Vec2>::SetInTangentFromOut()
	{
		assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED);
		float inLength = scale_from_dd_to_ds * (dd.GetLength()+1.0f) - 1.0f;
		float out = fabs(dd.x) > 0.000001f ? cry_atanf(dd.y/dd.x) : (dd.x*dd.y>=0?gf_PI/2.0f:-gf_PI/2.0f);
		ds.x = 1.0f;
		ds.y = cry_tanf(out + theta_from_dd_to_ds - gf_PI); 
		ds.Normalize();
		ds *= inLength;
	}

	template <class T> class TrackSplineInterpolator;

	template <>
	class TrackSplineInterpolator<Vec2> : public spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > > 
	{
	public:
		virtual int GetNumDimensions()
		{
			// It's actually one-dimensional since the x component curve is for a time-warping.
			return 1;
		}
		virtual void SerializeSpline( XmlNodeRef &node, bool bLoading ) {};
	private:
		// An utility function for the Newton–Raphson method
		float comp_time_deriv( int from, int to, float u) const
		{
			float u2 = u*u;
			float b0 = -3.0f*u2 + 6.0f*u - 3;
			float b1 = 9.0f*u2 - 12.0f*u + 3;
			float b2 = -9.0f*u2 + 6.0f*u;
			float b3 = 3.0f*u2;

			float p0 = this->value(from).x;
			float p3 = this->value(to).x;
			float p1 = p0 + this->dd(from).x;
			float p2 = p3 - this->ds(to).x;

			return (b0 * p0) + (b1 * p1) + (b2 * p2) + (b3 * p3);		
		}

		float comp_value_deriv( int from, int to, float u) const
		{
			float u2 = u*u;
			float b0 = -3.0f*u2 + 6.0f*u - 3;
			float b1 = 9.0f*u2 - 12.0f*u + 3;
			float b2 = -9.0f*u2 + 6.0f*u;
			float b3 = 3.0f*u2;

			float p0 = this->value(from).y;
			float p3 = this->value(to).y;
			float p1 = p0 + this->dd(from).y;
			float p2 = p3 - this->ds(to).y;

			return (b0 * p0) + (b1 * p1) + (b2 * p2) + (b3 * p3);		
		}

		float search_u( float time, ISplineInterpolator::ValueType & value ) 
		{
			float time_to_check = time;
			int count = 0;
			int curr = seek_key(time);
			int next = (curr < num_keys()-1) ? curr+1 : curr;
			float range_min = this->time(curr);
			float range_max = this->time(next);
			// Clamp the time first.
			if(time < this->time(0))
				time = this->time(0);
			else if(time > this->time(num_keys()-1))
				time = this->time(num_keys()-1);
			// It's somewhat tricky here. We should find the 't' where the x element
			// of the 2D Bezier curve equals to the specified 'time'.
			// The y component of the curve there is our value.
			// We use the 'Newton's method' to find the root.
			float u = 0;
			const float epsilon = 0.001f;
			do
			{
				spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::Interpolate(time_to_check, value);
				// In case of stepping tangents, we don't need this special processing.
				if (GetOutTangentType(curr) == SPLINE_KEY_TANGENT_STEP || GetInTangentType(next) == SPLINE_KEY_TANGENT_STEP)
					break;
				if(fabs(value[0]-time) < epsilon)
					// Finally, we got the solution.
					break;
				else
				{
					// Apply the Newton's method to compute the next time value to try.
					assert(next != curr);
					float timeDelta = this->time(next) - this->time(curr);
					if(timeDelta == 0)
						timeDelta = epsilon;
					u = (time_to_check - this->time(curr)) / timeDelta;
					float dt = comp_time_deriv(curr, next, u);
					u = u - (value[0] - time)/(dt+epsilon);
					if(u < 0)
						u = 0;
					else if(u > 1)
						u = 1;
					time_to_check = u * (this->time(next) - this->time(curr)) + this->time(curr);
				}
				++count;
			}
			while(count < 10);
			// It is expected to converge fast.
			assert(count < 10);	
			return u;
		}

		Vec2 interpolate_tangent(float time, float& u)
		{
			Vec2 tangent;
			const float epsilon = 0.001f;
			int curr = seek_key(time);
			int next = curr+1;
			assert(0 <= curr && next < num_keys());

			ISplineInterpolator::ValueType value;
			u = search_u(time, value);
			tangent.x = comp_time_deriv(curr, next, u);
			tangent.y = comp_value_deriv(curr, next, u);
			tangent /= 3.0f;
			return tangent;
		}
	public:
		// We should override following 4 methods to make it act like an 1D curve although it's actually a 2D curve. 
		virtual void  SetKeyTime( int key,float time )
		{
			ISplineInterpolator::ValueType value;
			ISplineInterpolator::ZeroValue(value);
			spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::GetKeyValue(key, value);
			value[0] = time;
			spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyValue(key, value);
			spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyTime(key, time);
		}
		virtual void  SetKeyValue( int key,ISplineInterpolator::ValueType value )
		{
			ISplineInterpolator::ValueType value0;
			ISplineInterpolator::ZeroValue(value0);
			value0[0] = GetKeyTime(key); 
			value0[1] = value[0]; 
			spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyValue(key, value0);
		}
		virtual bool  GetKeyValue( int key,ISplineInterpolator::ValueType &value )
		{
			if(spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::GetKeyValue(key, value))
			{
				value[0] = value[1];
				value[1] = 0;
				return true;
			}
			return false;
		}
		virtual void Interpolate( float time,ISplineInterpolator::ValueType &value )
		{
			if (empty()) 
				return;
			adjust_time(time);
			search_u(time, value);

			value[0] = value[1];
			value[1] = 0;
		}
		virtual void SetKeyFlags( int k,int flags )
		{
			if (k >= 0 && k < this->num_keys())
			{
				if((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) != SPLINE_KEY_TANGENT_UNIFIED
					&& (flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
					this->key(k).ComputeThetaAndScale();
			}
			spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyFlags(k, flags);
		}
		virtual void SetKeyInTangent( int k,ISplineInterpolator::ValueType tin )
		{
			if (k >= 0 && k < this->num_keys())
			{
				FromValueType( tin,this->key(k).ds );
				if((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
				{
					this->key(k).SetOutTangentFromIn();
					ConstrainOutTangentsOf(k);
				}
				this->SetModified(true);
			}
		}
		virtual void SetKeyOutTangent( int k,ISplineInterpolator::ValueType tout )
		{
			if (k >= 0 && k < this->num_keys())
			{
				FromValueType( tout,this->key(k).dd );
				if((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
				{
					this->key(k).SetInTangentFromOut();
					ConstrainInTangentsOf(k);
				}
				this->SetModified(true);
			}
		}

		// A pair of utility functions to constrain the time range
		// so that the time curve is always monotonically increasing.
		void ConstrainOutTangentsOf(int k)
		{
			if(k < num_keys() - 1
				&& this->key(k).dd.x > (this->time(k+1) - this->time(k)))
				this->key(k).dd *= (this->time(k+1) - this->time(k))/this->key(k).dd.x;
		}
		void ConstrainInTangentsOf(int k)
		{
			if(k > 0
				&& this->key(k).ds.x > (this->time(k) - this->time(k-1)))
				this->key(k).ds *= (this->time(k) - this->time(k-1))/this->key(k).ds.x;
		}

		virtual void comp_deriv()
		{
			spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> >::comp_deriv();

			// To process the 'zero tangent' case more properly,
			// here we override the tangent behavior for the case of SPLINE_KEY_TANGENT_ZERO.
			if (this->num_keys() > 1)
			{
				const float oneThird = 1/3.0f;

				const int last = this->num_keys()-1;

				{
					if(GetOutTangentType(0) == SPLINE_KEY_TANGENT_ZERO)
					{
						this->key(0).dd.x = oneThird * (this->value(1).x - this->value(0).x);
						this->key(0).dd.y = 0;
					}
					else
						ConstrainOutTangentsOf(0);
					// Set the in-tangent same to the out.
					if(GetInTangentType(0) == SPLINE_KEY_TANGENT_ZERO)
					{
						this->key(0).ds.x = oneThird * (this->value(1).x - this->value(0).x);
						this->key(0).ds.y = 0;
					}
					else
						ConstrainInTangentsOf(0);

					if(GetInTangentType(last) == SPLINE_KEY_TANGENT_ZERO)
					{
						this->key(last).ds.x = oneThird * (this->value(last).x - this->value(last-1).x);
						this->key(last).ds.y = 0;
					}
					else
						ConstrainInTangentsOf(last);
					// Set the out-tangent same to the in.
					if(GetOutTangentType(last) == SPLINE_KEY_TANGENT_ZERO)
					{
						this->key(last).dd.x = oneThird * (this->value(last).x - this->value(last-1).x);
						this->key(last).dd.y = 0;
					}
					else
						ConstrainOutTangentsOf(last);
				}

				for (int i = 1; i < last; ++i)
				{
					key_type& key = this->key(i);

					switch (GetInTangentType(i))
					{
					case SPLINE_KEY_TANGENT_ZERO:
						key.ds.x = oneThird * (this->value(i).x - this->value(i-1).x);
						key.ds.y = 0;
						break;
					default:
						ConstrainInTangentsOf(i);
						break;
					}

					switch (GetOutTangentType(i))
					{
					case SPLINE_KEY_TANGENT_ZERO:
						key.dd.x = oneThird * (this->value(i+1).x - this->value(i).x);
						key.dd.y = 0;
						break;
					default:
						ConstrainOutTangentsOf(i);
						break;
					}
				}
			}
		}

		virtual int InsertKey( float t,ISplineInterpolator::ValueType val )
		{
			Vec2 tangent;
			float u = 0;
			bool inRange = false;
			if(num_keys() > 1 && this->time(0) <= t && t <= this->time(num_keys()-1))
			{
				tangent = interpolate_tangent(t, u);
				inRange = true;
			}

			val[1] = val[0];
			val[0] = t;
			int keyIndex = spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::InsertKey(t,val);
			// Sets the default tangents properly.
			if(inRange)
			{
				this->key(keyIndex).ds = tangent * u;
				this->key(keyIndex).dd = tangent * (1-u);
				ConstrainInTangentsOf(keyIndex);
				ConstrainOutTangentsOf(keyIndex);
			}
			else
			{
				const float oneThird = 1/3.0f;
				if(keyIndex == 0)
				{
					u = 0;
					if(num_keys() > 1)
						this->key(0).dd.x = oneThird * (this->value(1).x - this->value(0).x);
					else
						this->key(0).dd.x = 1.0f;	// Just an arbitrary value
					this->key(0).dd.y = 0;
					// Set the in-tangent same to the out.
					this->key(0).ds.x = this->key(0).dd.x;
					this->key(0).ds.y = 0;
				}
				else if(keyIndex == num_keys() - 1)
				{
					u = 1;
					int last = num_keys() - 1;
					this->key(last).ds.x = oneThird * (this->value(last).x - this->value(last-1).x);
					this->key(last).ds.y = 0;
					// Set the out-tangent same to the in.
					this->key(last).dd.x = this->key(last).ds.x;
					this->key(last).dd.y = 0;
				}
				else
					assert(0);
			}
			// Sets the unified tangent handles to the default.
			SetKeyFlags(keyIndex, SPLINE_KEY_TANGENT_UNIFIED);
			// Adjusts neighbors.
			if(keyIndex - 1 >= 0)
			{
				this->key(keyIndex - 1).dd *= u;
				ConstrainOutTangentsOf(keyIndex - 1);
			}
			if(keyIndex + 1 < num_keys())
			{
				this->key(keyIndex + 1).ds *= (1-u);
				ConstrainInTangentsOf(keyIndex + 1);
			}
			return keyIndex;
		}
	};
}; // namespace spline

#endif // __2DSPLINE_H__
