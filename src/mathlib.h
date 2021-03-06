/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __MATHLIB_H
#define __MATHLIB_H

// mathlib.h

#include <math.h>

#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <btBulletDynamicsCommon.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define M_PI_DIV_180 (M_PI / 180.0) //johnfitz

typedef float vec4_t[4];
typedef float vec3_t[3];
typedef float vec_t;
struct mplane_s;

extern vec3_t vec3_origin;

#define	nanmask		(255 << 23)	/* 7F800000 */
#if 0	/* macro is violating strict aliasing rules */
#define	IS_NAN(x)	(((*(int *) (char *) &x) & nanmask) == nanmask)
#else
static inline int IS_NAN (float x)
{
	union
	{
		float f;
		int i;
	} num;
	num.f = x;
	return ((num.i & nanmask) == nanmask);
}
#endif

#define Q_rint(x) ((x) > 0 ? (int)((x) + 0.5) : (int)((x) - 0.5)) //johnfitz -- from joequake

#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define DoublePrecisionDotProduct(x,y) ((double)x[0]*y[0]+(double)x[1]*y[1]+(double)x[2]*y[2])
#define VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];}
#define VectorAdd(a,b,c) {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];}
#define VectorCopy(a,b) {b[0]=a[0];b[1]=a[1];b[2]=a[2];}

//johnfitz -- courtesy of lordhavoc
// QuakeSpasm: To avoid strict aliasing violations, use a float/int union instead of type punning.
#define VectorNormalizeFast(_v)\
{\
	union { float f; int i; } _y, _number;\
	_number.f = DotProduct(_v, _v);\
	if (_number.f != 0.0)\
	{\
		_y.i = 0x5f3759df - (_number.i >> 1);\
		_y.f = _y.f * (1.5f - (_number.f * 0.5f * _y.f * _y.f));\
		VectorScale(_v, _y.f, _v);\
	}\
}

// up / down
#define	PITCH		0
// left / right
#define	YAW		1
// fall over
#define	ROLL		2

#define DEG2RAD( a ) ( (a) * M_PI_DIV_180 )

void Btof(btVector3 v, vec3_t p1);
void InitRandSeed(long s);
double NextRandDouble();
double Drand(double min, double max);
void TurnVector (vec3_t out, const vec3_t forward, const vec3_t side, float angle); //johnfitz
void VectorAngles (const vec3_t forward, vec3_t angles); //johnfitz

void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

vec_t _DotProduct (vec3_t v1, vec3_t v2);
void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy (vec3_t in, vec3_t out);

int VectorCompare (vec3_t v1, vec3_t v2);
vec_t VectorLength (vec3_t v);
void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross);
float VectorNormalize (vec3_t v);		// returns vector length
void VectorInverse (vec3_t v);
void VectorScale (vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);

void WorldToScreen(vec4_t worldp);
void Perspective(float mat[16], float angle, float ratio, float near, float far);
float AdaptFovx (float fov_x, float width, float height);
float CalcFovy (float fov_x, float width, float height);
void PrintMatrix(float matrix[16], char* id = nullptr);
void PrintVec4(vec4_t v);
void PrintVec3(vec3_t v);
void OrthoMatrix(float matrix[16], float left, float right, float bottom, float top, float n, float f);
void FrustumMatrix(float matrix[16], float fovx, float fovy);
void WorldToCam(float view_matrix[16]);

void FloorDivMod (double numer, double denom, int *quotient,
                  int *rem);
//fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor (int i1, int i2);

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float	anglemod(float a);

void MatrixMultiply(float left[16], float right[16]);
void Vec4Xmat4(vec4_t v, float m[16]);
void RotationMatrix(float matrix[16], float angle, float x, float y, float z);
void TranslationMatrix(float matrix[16], float x, float y, float z);
void ScaleMatrix(float matrix[16], float x, float y, float z);
void IdentityMatrix(float matrix[16]);
void InvertMatrix(float invOut[16], float m[16]);
void LookAt(float matrix[16], vec3_t eye, vec3_t center, vec3_t up);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

#endif	/* __MATHLIB_H */

