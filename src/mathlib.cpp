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
// mathlib.c -- math primitives

#include "startup.h"
#include "mathlib.h"
#include "flog.h"
#include "entity.h"

vec3_t vec3_origin = {0,0,0};

/*-----------------------------------------------------------------*/

void Btof(btVector3 v, vec3_t p1)
	{
		p1[0] = v.getX();
		p1[1] = v.getY();
		p1[2] = v.getZ();
	}

static long seed;
void InitRandSeed(long s)
{
	seed = s;
}

double NextRandDouble()
{
	// Robert Jenkins' 32 bit integer hash function.
	seed = ((seed + 0x7ed55d16) + (seed << 12)) & 0xffffffff;
	seed = ((seed ^ 0xc761c23c) ^ (seed >> 19)) & 0xffffffff;
	seed = ((seed + 0x165667b1) + (seed << 5)) & 0xffffffff;
	seed = ((seed + 0xd3a2646c) ^ (seed << 9)) & 0xffffffff;
	seed = ((seed + 0xfd7046c5) + (seed << 3)) & 0xffffffff;
	seed = ((seed ^ 0xb55a4f09) ^ (seed >> 16)) & 0xffffffff;
	return static_cast <double> (seed & 0xfffffff) / static_cast <double> (0x10000000);
}

double Drand(double min, double max)
{
	//naiive way
	double result = NextRandDouble();
	while(result < min || result > max)
	{
		result = NextRandDouble() * max;
	}
	return result;
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

//johnfitz -- removed RotatePointAroundVector() becuase it's no longer used and my compiler fucked it up anyway

/*-----------------------------------------------------------------*/


float	anglemod(float a)
{
#if 0
	if (a >= 0)
		a -= 360*(int)(a/360);
	else
		a += 360*( 1 + (int)(-a/360) );
#endif
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

//johnfitz -- the opposite of AngleVectors.  this takes forward and generates pitch yaw roll
//TODO: take right and up vectors to properly set yaw and roll
void VectorAngles (const vec3_t forward, vec3_t angles)
{
	vec3_t temp;

	temp[0] = forward[0];
	temp[1] = forward[1];
	temp[2] = 0;
	angles[PITCH] = -atan2(forward[2], VectorLength(temp)) / M_PI_DIV_180;
	angles[YAW] = atan2(forward[1], forward[0]) / M_PI_DIV_180;
	angles[ROLL] = 0;
}

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	right[0] = (-1*sr*sp*cy+-1*cr*-sy);
	right[1] = (-1*sr*sp*sy+-1*cr*cy);
	right[2] = -1*sr*cp;
	up[0] = (cr*sp*cy+-sr*-sy);
	up[1] = (cr*sp*sy+-sr*cy);
	up[2] = cr*cp;
}

int VectorCompare (vec3_t v1, vec3_t v2)
{
	int		i;

	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;

	return 1;
}

void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

vec_t _DotProduct4 (vec4_t v1, vec4_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2] + v1[3]*v2[3];
}

void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorLength(vec3_t v)
{
	return sqrt(DotProduct(v,v));
}

float VectorNormalize (vec3_t v)
{
	float	length, ilength;

	length = sqrt(DotProduct(v,v));

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;

}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}


int Q_log2(int val)
{
	int answer=0;
	while (val>>=1)
		answer++;
	return answer;
}

void PrintVec3(vec3_t v)
	{
			p("X:%f Y:%f Z:%f", v[0], v[1], v[2]);
	}

void PrintVec4(vec4_t v)
	{
			p("X:%f Y:%f Z:%f W:%f", v[0], v[1], v[2], v[3]);
	}

void PrintMatrix(float matrix[16], char* id)
{
	if(id)
		{
			p("--%s--", id);
			p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 0, matrix[0], matrix[1], matrix[2], matrix[3]);
			p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 1, matrix[4], matrix[5], matrix[6], matrix[7]);
			p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 2, matrix[8], matrix[9], matrix[10], matrix[11]);
			p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 3, matrix[12], matrix[13], matrix[14], matrix[15]);
			return;
		}
	p("------");
	p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 0, matrix[0], matrix[1], matrix[2], matrix[3]);
	p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 1, matrix[4], matrix[5], matrix[6], matrix[7]);
	p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 2, matrix[8], matrix[9], matrix[10], matrix[11]);
	p("Row %d: [ %.6f, %.6f, %.6f, %.6f ]", 3, matrix[12], matrix[13], matrix[14], matrix[15]);
}

//if W is zero, you have a problem, so you should check that beforehand. If it is zero, then that means that the object is in the plane of projection

void WorldToScreen(vec4_t worldp)
	{
		Vec4Xmat4(worldp, cam.mvp);
		if(worldp[3] != 0)
			{
				worldp[0] /= worldp[3];
				worldp[1] /= worldp[3];
				worldp[2] /= worldp[3];
			}
		worldp[0] = ((worldp[0] / 2.0f) * window_width) + (0.5f * window_width);
		worldp[1] = ((worldp[1] / 2.0f) * window_height) + (0.5f * window_height);
		worldp[2] = 0;

		worldp[0] = CLAMP(0, worldp[0], window_width);
		worldp[1] = CLAMP(0, worldp[1], window_height);

	}

void OrthoMatrix(float matrix[16], float left, float right, float bottom, float top, float n, float f)
{
	float tx = -(right + left) / (right - left);
	float ty = (top + bottom) / (top - bottom);
	float tz = -(f + n) / (f - n);

	memset(matrix, 0, 16 * sizeof(float));

	// First column
	matrix[0*4 + 0] = 2.0f / (right-left);

	// Second column
	matrix[1*4 + 1] = -2.0f / (top-bottom);

	// Third column
	matrix[2*4 + 2] = -2.0f / (f-n);

	// Fourth column
	matrix[3*4 + 0] = tx;
	matrix[3*4 + 1] = ty;
	matrix[3*4 + 2] = tz;
	matrix[3*4 + 3] = 1.0f;
}

void Perspective(float matrix[16], float angle, float ratio, float n, float f)
{
	float tan_half_angle = tanf(angle / 2);

	// First column
	matrix[0*4 + 0] = 1.0f / (ratio * tan_half_angle);
	matrix[0*4 + 1] = 0.0f;
	matrix[0*4 + 2] = 0.0f;
	matrix[0*4 + 3] = 0.0f;

	// Second column
	matrix[1*4 + 0] = 0.0f;
	matrix[1*4 + 1] = (1.0f / tan_half_angle);
	matrix[1*4 + 2] = 0.0f;
	matrix[1*4 + 3] = 0.0f;

	// Third column
	matrix[2*4 + 0] = 0.0f;
	matrix[2*4 + 1] = 0.0f;
	matrix[2*4 + 2] = -(f + n) / (f - n);
	matrix[2*4 + 3] = -1.0f;

	// Fourth column
	matrix[3*4 + 0] = 0.0f;
	matrix[3*4 + 1] = 0.0f;
	matrix[3*4 + 2] = -(2 * f * n) / (f - n);
	matrix[3*4 + 3] = 0.0f;
}

/*
====================
AdaptFovx
Adapt a 4:3 horizontal FOV to the current screen size using the "Hor+" scaling:
2.0 * atan(width / height * 3.0 / 4.0 * tan(fov_x / 2.0))
====================
*/
float AdaptFovx (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		error("Bad fov: %f", fov_x);

	if ((x = height / width) == 0.75)
		return fov_x;
	a = atan(0.75 / x * tan(fov_x / 360 * M_PI));
	a = a * 360 / M_PI;
	return a;
}

/*
====================
CalcFovy
====================
*/
float CalcFovy (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		error("Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);
	a = atan(height / x);
	a = a * 360 / M_PI;
	return a;
}

void FrustumMatrix(float matrix[16], float fovx, float fovy)
{
	const float w = 1.0f / tanf(fovx * 0.5f);
	const float h = 1.0f / tanf(fovy * 0.5f);

	const float n = 4; //near

	// - farclip now defaults to 16384.
	//   This should be high enough to handle even the largest open areas without unwanted clipping.
	//   The only reason you'd want to lower this number is if you see z-fighting.

	const float f = 16384; //far clip

	memset(matrix, 0, 16 * sizeof(float));

	// First column
	matrix[0*4 + 0] = w;

	// Second column
	matrix[1*4 + 1] = -h;

	// Third column
	matrix[2*4 + 2] = -f / (f - n);
	matrix[2*4 + 3] = -1.0f;

	// Fourth column
	matrix[3*4 + 2] = -(n * f) / (f - n);
}


//another way to compute view_matrix, slower than lookat
void WorldToCam(float view_matrix[16])
{
	float rotation_matrix[16];

	RotationMatrix(view_matrix, -M_PI / 2.0f, 1.0f, 0.0f, 0.0f);
	RotationMatrix(rotation_matrix,  M_PI / 2.0f, 0.0f, 0.0f, 1.0f);
	MatrixMultiply(view_matrix, rotation_matrix);
	RotationMatrix(rotation_matrix, DEG2RAD(-cam.roll), 1.0f, 0.0f, 0.0f);
	MatrixMultiply(view_matrix, rotation_matrix);
	RotationMatrix(rotation_matrix, DEG2RAD(-cam.pitch), 0.0f, 1.0f, 0.0f);
	MatrixMultiply(view_matrix, rotation_matrix);
	RotationMatrix(rotation_matrix, DEG2RAD(-cam.yaw), 0.0f, 0.0f, 1.0f);
	MatrixMultiply(view_matrix, rotation_matrix);


	float translation_matrix[16];
	memset(translation_matrix, 0, sizeof(translation_matrix));
	TranslationMatrix(translation_matrix, -cam.pos[0], -cam.pos[1], -cam.pos[2]);
	MatrixMultiply(view_matrix, translation_matrix);

//	PrintMatrix(view_matrix);
//	PrintMatrix(cam.view);
}

/*
===================
FloorDivMod

Returns mathematically correct (floor-based) quotient and remainder for
numer and denom, both of which should contain no fractional part. The
quotient must fit in 32 bits.
====================
*/

void FloorDivMod (double numer, double denom, int *quotient, int *rem)
{
	int		q, r;
	double	x;

#ifndef PARANOID
	if (denom <= 0.0)
		printf ("FloorDivMod: bad denominator %f\n", denom);

//	if ((floor(numer) != numer) || (floor(denom) != denom))
//		printf ("FloorDivMod: non-integer numer or denom %f %f\n",
//				numer, denom);
#endif

	if (numer >= 0.0)
	{

		x = floor(numer / denom);
		q = (int)x;
		r = (int)floor(numer - (x * denom));
	}
	else
	{
		//
		// perform operations with positive values, and fix mod to make floor-based
		//
		x = floor(-numer / denom);
		q = -(int)x;
		r = (int)floor(-numer - (x * denom));
		if (r != 0)
		{
			q--;
			r = (int)denom - r;
		}
	}

	*quotient = q;
	*rem = r;
}


/*
===================
GreatestCommonDivisor
====================
*/
int GreatestCommonDivisor (int i1, int i2)
{
	if (i1 > i2)
	{
		if (i2 == 0)
			return (i1);
		return GreatestCommonDivisor (i2, i1 % i2);
	}
	else
	{
		if (i1 == 0)
			return (i2);
		return GreatestCommonDivisor (i1, i2 % i1);
	}
}


/*
===================
Invert24To16

Inverts an 8.24 value to a 16.16 value
====================
*/

/* fixed16_t Invert24To16(fixed16_t val)
{
	if (val < 256)
		return (0xFFFFFFFF);

	return (fixed16_t)
			(((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
} */

/*
===================
MatrixMultiply
====================
*/
void MatrixMultiply(float left[16], float right[16])
{
	float temp[16];
	int column, row, i;

	memcpy(temp, left, 16 * sizeof(float));
	for(row = 0; row < 4; ++row)
	{
		for(column = 0; column < 4; ++column)
		{
			float value = 0.0f;
			for (i = 0; i < 4; ++i)
				value += temp[i*4 + row] * right[column*4 + i];

			left[column * 4 + row] = value;
		}
	}
}

void Vec4Xmat4(vec4_t v, float m[16])
	{
		vec4_t copy;
		zone::Q_memcpy(copy, v, 4*sizeof(float));
		v[0] = copy[0]*m[0] + copy[1]*m[4] + copy[2]*m[8] + copy[3]*m[12];
		v[1] = copy[0]*m[1] + copy[1]*m[5] + copy[2]*m[9] + copy[3]*m[13];
		v[2] = copy[0]*m[2] + copy[1]*m[6] + copy[2]*m[10] + copy[3]*m[14];
		v[3] = copy[0]*m[3] + copy[1]*m[7] + copy[2]*m[11] + copy[3]*m[15];
	}

/*
=============
RotationMatrix

mat4 =
{
	vec4,
	vec4,
	vec4,
	vec4,
}4

=============
*/
void RotationMatrix(float matrix[16], float angle, float x, float y, float z)
{
	const float c = cosf(angle);
	const float s = sinf(angle);

	// First column
	matrix[0*4 + 0] = x * x * (1.0f - c) + c;
	matrix[0*4 + 1] = y * x * (1.0f - c) + z * s;
	matrix[0*4 + 2] = x * z * (1.0f - c) - y * s;
	matrix[0*4 + 3] = 0.0f;

	// Second column
	matrix[1*4 + 0] = x * y * (1.0f - c) - z * s;
	matrix[1*4 + 1] = y * y * (1.0f - c) + c;
	matrix[1*4 + 2] = y * z * (1.0f - c) + x * s;
	matrix[1*4 + 3] = 0.0f;

	// Third column
	matrix[2*4 + 0] = x * z * (1.0f - c) + y * s;
	matrix[2*4 + 1] = y * z * (1.0f - c) - x * s;
	matrix[2*4 + 2] = z * z * (1.0f - c) + c;
	matrix[2*4 + 3] = 0.0f;

	// Fourth column
	matrix[3*4 + 0] = 0.0f;
	matrix[3*4 + 1] = 0.0f;
	matrix[3*4 + 2] = 0.0f;
	matrix[3*4 + 3] = 1.0f;
}

float vec4_mul_inner(vec4_t a, vec4_t b)
{
	float p = 0.0f;
	int i;
	for (i = 0; i < 4; ++i) p += b[i] * a[i];
	return p;
}

void TranslateInPlace(float matrix[16], float x, float y, float z)
{
	vec4_t t = {x, y, z, 0};
	vec4_t r;
	int i;
	for (i = 0; i < 4; ++i)
	{
		for (int k = 0; k < 4; ++k)
		{
			r[k] = matrix[k*4 + i]; //get row values from matrix
		}
		matrix[3*4 + i] += vec4_mul_inner(r, t);
	}
}

//uses colum major format
//[cam_right.x,    cam_right.y,    cam_right.z,     0]
//[cam_up.x,       cam_up.y,       cam_up.z,        0]
//[cam_forward.x,  cam_forward.y,  cam_forward.z,   0]
//[cam_position.x, cam_position.y, cam_position.z,  1]

void LookAt(float matrix[16], vec3_t eye, vec3_t center, vec3_t up)
{
	/* TODO: The negation of of can be spared by swapping the order of
	 *       operands in the following cross products in the right way. */

	vec3_t f;
	_VectorSubtract(center, eye, f); //compute forward Z
	VectorNormalize(f);

	vec3_t s;
	CrossProduct(f, up, s); //compute right X vector
	VectorNormalize(s);

	vec3_t t;
	CrossProduct(s, f, t); //compute UP Y

	// First column
	matrix[0*4 + 0] = s[0];
	matrix[0*4 + 1] = t[0];
	matrix[0*4 + 2] = -f[0];
	matrix[0*4 + 3] = 0.0f;

	// Second column
	matrix[1*4 + 0] = s[1];
	matrix[1*4 + 1] = t[1];
	matrix[1*4 + 2] = -f[1];
	matrix[1*4 + 3] = 0.0f;

	// Third column
	matrix[2*4 + 0] = s[2];
	matrix[2*4 + 1] = t[2];
	matrix[2*4 + 2] = -f[2];
	matrix[2*4 + 3] = 0.0f;

	// Fourth column
	matrix[3*4 + 0] = -_DotProduct(s, eye);
	matrix[3*4 + 1] = -_DotProduct(t, eye);
	matrix[3*4 + 2] = _DotProduct(f, eye);
	matrix[3*4 + 3] = 1.0f;
}

/*
=============
TranslationMatrix
=============
*/
void TranslationMatrix(float matrix[16], float x, float y, float z)
{
	// First column
	matrix[0*4 + 0] = 1.0f;

	// Second column
	matrix[1*4 + 1] = 1.0f;

	// Third column
	matrix[2*4 + 2] = 1.0f;

	// Fourth column
	matrix[3*4 + 0] = x;
	matrix[3*4 + 1] = y;
	matrix[3*4 + 2] = z;
	matrix[3*4 + 3] = 1.0f;
}

/*
=============
ScaleMatrix
=============
*/
void ScaleMatrix(float matrix[16], float x, float y, float z)
{
	// First column
	matrix[0*4 + 0] = x;

	// Second column
	matrix[1*4 + 1] = y;

	// Third column
	matrix[2*4 + 2] = z;

	// Fourth column
	matrix[3*4 + 3] = 1.0f;
}

/*
=============
IdentityMatrix
=============
*/
void IdentityMatrix(float matrix[16])
{
	memset(matrix, 0, 16 * sizeof(float));

	// First column
	matrix[0*4 + 0] = 1.0f;

	// Second column
	matrix[1*4 + 1] = 1.0f;

	// Third column
	matrix[2*4 + 2] = 1.0f;

	// Fourth column
	matrix[3*4 + 3] = 1.0f;
}

void InvertMatrix(float invOut[16], float m[16])
{
	float inv[16], det;
	int i;

	inv[0] = m[5]  * m[10] * m[15] -
	         m[5]  * m[11] * m[14] -
	         m[9]  * m[6]  * m[15] +
	         m[9]  * m[7]  * m[14] +
	         m[13] * m[6]  * m[11] -
	         m[13] * m[7]  * m[10];

	inv[4] = -m[4]  * m[10] * m[15] +
	         m[4]  * m[11] * m[14] +
	         m[8]  * m[6]  * m[15] -
	         m[8]  * m[7]  * m[14] -
	         m[12] * m[6]  * m[11] +
	         m[12] * m[7]  * m[10];

	inv[8] = m[4]  * m[9] * m[15] -
	         m[4]  * m[11] * m[13] -
	         m[8]  * m[5] * m[15] +
	         m[8]  * m[7] * m[13] +
	         m[12] * m[5] * m[11] -
	         m[12] * m[7] * m[9];

	inv[12] = -m[4]  * m[9] * m[14] +
	          m[4]  * m[10] * m[13] +
	          m[8]  * m[5] * m[14] -
	          m[8]  * m[6] * m[13] -
	          m[12] * m[5] * m[10] +
	          m[12] * m[6] * m[9];

	inv[1] = -m[1]  * m[10] * m[15] +
	         m[1]  * m[11] * m[14] +
	         m[9]  * m[2] * m[15] -
	         m[9]  * m[3] * m[14] -
	         m[13] * m[2] * m[11] +
	         m[13] * m[3] * m[10];

	inv[5] = m[0]  * m[10] * m[15] -
	         m[0]  * m[11] * m[14] -
	         m[8]  * m[2] * m[15] +
	         m[8]  * m[3] * m[14] +
	         m[12] * m[2] * m[11] -
	         m[12] * m[3] * m[10];

	inv[9] = -m[0]  * m[9] * m[15] +
	         m[0]  * m[11] * m[13] +
	         m[8]  * m[1] * m[15] -
	         m[8]  * m[3] * m[13] -
	         m[12] * m[1] * m[11] +
	         m[12] * m[3] * m[9];

	inv[13] = m[0]  * m[9] * m[14] -
	          m[0]  * m[10] * m[13] -
	          m[8]  * m[1] * m[14] +
	          m[8]  * m[2] * m[13] +
	          m[12] * m[1] * m[10] -
	          m[12] * m[2] * m[9];

	inv[2] = m[1]  * m[6] * m[15] -
	         m[1]  * m[7] * m[14] -
	         m[5]  * m[2] * m[15] +
	         m[5]  * m[3] * m[14] +
	         m[13] * m[2] * m[7] -
	         m[13] * m[3] * m[6];

	inv[6] = -m[0]  * m[6] * m[15] +
	         m[0]  * m[7] * m[14] +
	         m[4]  * m[2] * m[15] -
	         m[4]  * m[3] * m[14] -
	         m[12] * m[2] * m[7] +
	         m[12] * m[3] * m[6];

	inv[10] = m[0]  * m[5] * m[15] -
	          m[0]  * m[7] * m[13] -
	          m[4]  * m[1] * m[15] +
	          m[4]  * m[3] * m[13] +
	          m[12] * m[1] * m[7] -
	          m[12] * m[3] * m[5];

	inv[14] = -m[0]  * m[5] * m[14] +
	          m[0]  * m[6] * m[13] +
	          m[4]  * m[1] * m[14] -
	          m[4]  * m[2] * m[13] -
	          m[12] * m[1] * m[6] +
	          m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
	         m[1] * m[7] * m[10] +
	         m[5] * m[2] * m[11] -
	         m[5] * m[3] * m[10] -
	         m[9] * m[2] * m[7] +
	         m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
	         m[0] * m[7] * m[10] -
	         m[4] * m[2] * m[11] +
	         m[4] * m[3] * m[10] +
	         m[8] * m[2] * m[7] -
	         m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
	          m[0] * m[7] * m[9] +
	          m[4] * m[1] * m[11] -
	          m[4] * m[3] * m[9] -
	          m[8] * m[1] * m[7] +
	          m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
	          m[0] * m[6] * m[9] -
	          m[4] * m[1] * m[10] +
	          m[4] * m[2] * m[9] +
	          m[8] * m[1] * m[6] -
	          m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	if(!det)
		{
			warn("Invalid InvertMatrix");
			return;
		}
	det = 1.0 / det;

	for (i = 0; i < 16; i++)
	{
		invOut[i] = inv[i] * det;
	}
}
