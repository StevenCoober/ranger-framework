//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//
// This file is part of RecastDemo, and has been modified by RangerUFO.
//

#include "MeshLoaderObj.h"
#include <RecastAlloc.h>	// Added by RangerUFO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <new>

rcMeshLoaderObj::rcMeshLoaderObj() :
	m_scale(1.0f),
	m_verts(0),
	m_tris(0),
	m_normals(0),
	m_vertCount(0),
	m_triCount(0)
{
}

rcMeshLoaderObj::~rcMeshLoaderObj()
{
	// Modified by RangerUFO
	// @{
	if (m_verts) rcFree(m_verts);
	if (m_normals) rcFree(m_normals);
	if (m_tris) rcFree(m_tris);
	// @}
}
		
bool rcMeshLoaderObj::addVertex(float x, float y, float z, int& cap)	// Modified by RangerUFO
{
	if (m_vertCount+1 > cap)
	{
		cap = !cap ? 8 : cap*2;
		float* nv = static_cast<float*>(rcAlloc(cap * 3 * sizeof(float), RC_ALLOC_PERM));	// Modified by RangerUFO
		if (!nv) return false;	// Added by RangerUFO
		if (m_vertCount)
			memcpy(nv, m_verts, m_vertCount*3*sizeof(float));
		if (m_verts) rcFree(m_verts);	// Modified by RangerUFO
		m_verts = nv;
	}
	float* dst = &m_verts[m_vertCount*3];
	*dst++ = x*m_scale;
	*dst++ = y*m_scale;
	*dst++ = z*m_scale;
	m_vertCount++;
	return true;	// Added by RangerUFO
}

bool rcMeshLoaderObj::addTriangle(int a, int b, int c, int& cap)	// Modified by RangerUFO
{
	if (m_triCount+1 > cap)
	{
		cap = !cap ? 8 : cap*2;
		int* nv = static_cast<int*>(rcAlloc(cap * 3 * sizeof(int), RC_ALLOC_PERM));	// Modified by RangerUFO
		if (!nv) return false;	// Added by RangerUFO
		if (m_triCount)
			memcpy(nv, m_tris, m_triCount*3*sizeof(int));
		if (m_tris) rcFree(m_tris);	// Modified by RangerUFO
		m_tris = nv;
	}
	int* dst = &m_tris[m_triCount*3];
	*dst++ = a;
	*dst++ = b;
	*dst++ = c;
	m_triCount++;
	return true;	// Added by RangerUFO
}

static char* parseRow(char* buf, char* bufEnd, char* row, int len)
{
	bool cont = false;
	bool start = true;
	bool done = false;
	int n = 0;
	while (!done && buf < bufEnd)
	{
		char c = *buf;
		buf++;
		// multirow
		switch (c)
		{
			case '\\':
				cont = true; // multirow
				break;
			case '\n':
				if (start) break;
				done = true;
				break;
			case '\r':
				break;
			case '\t':
			case ' ':
				if (start) break;
			default:
				start = false;
				cont = false;
				row[n++] = c;
				if (n >= len-1)
					done = true;
				break;
		}
	}
	row[n] = '\0';
	return buf;
}

static int parseFace(char* row, int* data, int n, int vcnt)
{
	int j = 0;
	while (*row != '\0')
	{
		// Skip initial white space
		while (*row != '\0' && (*row == ' ' || *row == '\t'))
			row++;
		char* s = row;
		// Find vertex delimiter and terminated the string there for conversion.
		while (*row != '\0' && *row != ' ' && *row != '\t')
		{
			if (*row == '/') *row = '\0';
			row++;
		}
		if (*s == '\0')
			continue;
		int vi = atoi(s);
		data[j++] = vi < 0 ? vi+vcnt : vi-1;
		if (j >= n) return j;
	}
	return j;
}

bool rcMeshLoaderObj::load(const char* filename)
{
	char* buf = 0;
	FILE* fp = fopen(filename, "rb");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	int bufSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buf = static_cast<char*>(rcAlloc(bufSize * sizeof(char), RC_ALLOC_PERM));	// Modified by RangerUFO
	if (!buf)
	{
		fclose(fp);
		return false;
	}
	fread(buf, bufSize, 1, fp);
	fclose(fp);

	char* src = buf;
	char* srcEnd = buf + bufSize;
	char row[512];
	int face[32];
	float x,y,z;
	int nv;
	int vcap = 0;
	int tcap = 0;
	
	while (src < srcEnd)
	{
		// Parse one row
		row[0] = '\0';
		src = parseRow(src, srcEnd, row, sizeof(row)/sizeof(char));
		// Skip comments
		if (row[0] == '#') continue;
		if (row[0] == 'v' && row[1] != 'n' && row[1] != 't')
		{
			// Vertex pos
			sscanf(row+1, "%f %f %f", &x, &y, &z);
			// Modified by RangerUFO
			// @{
			if (!addVertex(x, y, z, vcap))
			{
				rcFree(buf);
				return false;
			}
			// @}
		}
		if (row[0] == 'f')
		{
			// Faces
			nv = parseFace(row+1, face, 32, m_vertCount);
			for (int i = 2; i < nv; ++i)
			{
				const int a = face[0];
				const int b = face[i-1];
				const int c = face[i];
				if (a < 0 || a >= m_vertCount || b < 0 || b >= m_vertCount || c < 0 || c >= m_vertCount)
					continue;
				// Modified by RangerUFO
				// @{
				if (!addTriangle(a, b, c, tcap))
				{
					rcFree(buf);
					return false;
				}
				// @}
			}
		}
	}

	rcFree(buf);	// Modified by RangerUFO

	// Calculate normals.
	m_normals = static_cast<float*>(rcAlloc(m_triCount * 3 * sizeof(float), RC_ALLOC_PERM));	// Modified by RangerUFO
	if (!m_normals) return false;	// Added by RangerUFO
	for (int i = 0; i < m_triCount*3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i]*3];
		const float* v1 = &m_verts[m_tris[i+1]*3];
		const float* v2 = &m_verts[m_tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1]*e1[2] - e0[2]*e1[1];
		n[1] = e0[2]*e1[0] - e0[0]*e1[2];
		n[2] = e0[0]*e1[1] - e0[1]*e1[0];
		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
	
	strncpy(m_filename, filename, sizeof(m_filename));
	m_filename[sizeof(m_filename)-1] = '\0';
	
	return true;
}

// Added by RangerUFO
// @{
rcMeshLoaderObj* rcAllocMeshLoaderObj()
{
	void* p = rcAlloc(sizeof(rcMeshLoaderObj), RC_ALLOC_PERM);
	if (!p) return 0;
	return ::new(p) rcMeshLoaderObj;
}

void rcFreeMeshLoaderObj(rcMeshLoaderObj* p)
{
	if (!p) return;
	p->~rcMeshLoaderObj();
	rcFree(p);
}
// @}
