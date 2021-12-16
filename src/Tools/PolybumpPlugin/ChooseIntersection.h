#pragma once




//!
//! /param invStart start vector of ray
//! /param invEnd end vector of ray
//! /return 0xffffffff nothing found, triangle index otherwise
uint32 ChooseNearestIntersection( CIntInfoHighList &inIntersections, const Vec3 &invStart, const Vec3 &invEnd,
																									const Vec3 &refnormal, Vec3 &respoint );

//!
//! /param invStart start vector of ray
//! /param invEnd end vector of ray
//! /return 0xffffffff nothing found, triangle index otherwise
uint32 ChooseNearestAcceptableIntersection( CIntInfoHighList &inIntersections, const Vec3 &invStart, const Vec3 &invEnd,
																									const Vec3 &refnormal, Vec3 &respoint );

//!
//! /param invStart start vector of ray
//! /param invEnd end vector of ray
//! /return 0xffffffff nothing found, triangle index otherwise
uint32 ChooseLatestIntersection( CIntInfoHighList &inIntersections, const Vec3 &invStart, const Vec3 &invEnd, 
																									const Vec3 &refnormal, Vec3 &respoint );
