#include "delaunay.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>

#define max(a ,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define SORT_MAX_LEVELS 64
#define FLOAT_EPSILON 0.0001f

typedef struct {
	float x, y;
} _vert;

typedef struct {
	_vert p1, p2, p3;
} _tri;

typedef struct {
	_vert p;
	float r;
} _cir;

static int l_delaunayTriangulate(lua_State *lua)
{
	luaL_checktype(lua, 1, LUA_TTABLE);
	luaL_checktype(lua, 2, LUA_TTABLE);

	int len = luaL_len(lua, 1);
	if(luaL_len(lua, 2) != len){
		luaL_error(lua, "Lua: delaunaytriangulate, x and y arrays need to be the same length\n");
		exit(1);
	}

	len--;
	float *xs = (float*)malloc(len * sizeof(float));
	float *ys = (float*)malloc(len * sizeof(float));

	for(int i = 1; i < len + 1; i++){
		lua_pushinteger(lua, i);
		lua_gettable(lua, 1);
		xs[i - 1] = lua_tonumber(lua, -1);

		lua_pushinteger(lua, i);
		lua_gettable(lua, 2);
		ys[i - 1] = lua_tonumber(lua, -1);
	}

	int *ids = NULL;
	delaunayTriangulate(&ids, xs, ys, len);

	free(xs);
	free(ys);

	return 0;
}

static inline bool sortFunc(_vert v1, _vert v2)
{
	if(v1.x < v2.x){
		return true;
	}else if(v1.x == v2.x){
		return v1.y < v2.y;
	}
	return false;
}

static inline void swapInt(int *x, int *y){
	*x ^= *y;
	*y ^= *x;
	*x ^= *y;
}

// Quicksort
static void sortPositions(_vert *vs, int amount)
{
	int beg[SORT_MAX_LEVELS], end[SORT_MAX_LEVELS];

	beg[0] = 0;
	end[0] = amount;
	for(int i = 0; i >= 0;){
		int l = beg[i];
		int r = end[i] - 1;
		if(l < r){
			_vert piv = vs[l];
			while(l < r){
				while(sortFunc(piv, vs[r]) && l < r){
					r--;
				}
				if(l < r){
					vs[l++] = vs[r];
				}
				while(!sortFunc(piv, vs[l]) && l < r){
					l++;
				}
				if(l < r){
					vs[r--] = vs[l];
				}
			}
			vs[l] = piv;
			beg[i + 1] = l + 1;
			end[i + 1] = end[i];
			end[i++] = l;
			if(end[i] - beg[i] > end[i - 1] - beg[i - 1]){
				swapInt(beg + i, beg + i - 1);
				swapInt(end + i, end + i - 1);
			}
		}else{
			i--;
		}
	}
}

// Returns if point lies in the circum circle
static inline bool getCircumCircle(_vert p, _tri t, _cir *c)
{
	float dy1 = abs(t.p1.y - t.p2.y);
	float dy2 = abs(t.p2.y - t.p3.y);

	if(dy1 < FLOAT_EPSILON){
		if(dy2 < FLOAT_EPSILON){
			return false;
		}

		float m2 = - (t.p3.x - t.p2.x) / (t.p3.y - t.p2.y);
		float mx2 = (t.p2.x + t.p3.x) * 0.5f;
		float my2 = (t.p2.y + t.p3.y) * 0.5f;
		c->p.x = (t.p2.x + t.p1.x) * 0.5f;
		c->p.y = m2 * (c->p.x - mx2) + my2;
	}else if(dy2 < FLOAT_EPSILON){
		float m1 = - (t.p2.x - t.p1.x) / (t.p2.y - t.p1.y);
		float mx1 = (t.p1.x + t.p2.x) * 0.5f;
		float my1 = (t.p1.y + t.p2.y) * 0.5f;
		c->p.x = (t.p3.x + t.p2.x) * 0.5f;
		c->p.y = m1 * (c->p.x - mx1) + my1;
	}else{
		float m1 = - (t.p2.x - t.p1.x) / (t.p2.y - t.p1.y);
		float m2 = - (t.p3.x - t.p2.x) / (t.p3.y - t.p2.y);
		float mx1 = (t.p1.x + t.p2.x) * 0.5f;
		float mx2 = (t.p2.x + t.p3.x) * 0.5f;
		float my1 = (t.p1.y + t.p2.y) * 0.5f;
		float my2 = (t.p2.y + t.p3.y) * 0.5f;
		c->p.x = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
		if(dy1 > dy2){
			c->p.y = m1 * (c->p.x - mx1) + my1;
		}else{
			c->p.y = m2 * (c->p.x - mx2) + my2;
		}
	}

	float dx = t.p2.x - c->p.x;
	float dy = t.p2.y - c->p.y;
	c->r = dx * dx + dy * dy;

	dx = p.x - c->p.x;
	dy = p.y - c->p.y;
	float dr = dx * dx + dy * dy;

	return (dr - c->r) <= FLOAT_EPSILON;
}

static inline bool sameEdge(_vert p1, _vert p2, _vert p3, _vert p4)
{
	return ((p1.x == p3.x) && (p1.y == p3.y) &&
		(p2.x == p4.x) && (p2.y == p4.y)) ||
		((p1.x == p4.x) && (p1.y == p4.y) &&
		(p2.x == p3.x) && (p2.y == p3.y));
}

static inline int getSharedEdge(_tri t1, _vert p1, _vert p2)
{
	if(sameEdge(t1.p1, t1.p2, p1, p2)){
		return 1;
	}else if(sameEdge(t1.p2, t1.p3, p1, p2)){
		return 2;
	}else if(sameEdge(t1.p3, t1.p1, p1, p2)){
		return 3;
	}
	return -1;
}

void delaunayRegisterLua(lua_State *lua)
{
	lua_register(lua, "delaunaytriangulate", l_delaunayTriangulate);
}

int delaunayTriangulate(int **ids, const float *x, const float *y, int amount)
{
	if(amount < 3){
		return -1;
	}

	_vert *vs = (_vert*)malloc(amount * sizeof(_vert));

	for(int i = 0; i < amount; i++){
		vs[i].x = x[i];
		vs[i].y = y[i];
	}

	sortPositions(vs, amount);

	// Find super triangle
	float xmin = vs[0].x;
	float ymin = vs[0].y;
	float xmax = xmin;
	float ymax = ymin;

	for(int i = 1; i < amount; i++){
		xmin = min(xmin, vs[i].x);
		ymin = min(ymin, vs[i].y);
		xmax = max(xmax, vs[i].x);
		ymax = max(ymax, vs[i].y);
	}

	float dx = xmax - xmin;
	float dy = ymax - ymin;
	float dmax = max(dx, dy);
	float xmid = (xmin + dx) * 0.5f;
	float ymid = (ymin + dy) * 0.5f;

	_vert sp1 = {.x = xmid - 20 * dmax, .y = ymid - dmax};
	_vert sp2 = {.x = xmid, .y = ymid + 20 * dmax};
	_vert sp3 = {.x = xmid + 20 * dmax, .y = ymid - dmax};

	// Create triangle list and add the super triangle to it
	_tri *tris = (_tri*)malloc(amount * sizeof(_tri));
	tris[0] = (_tri){sp1, sp2, sp3};
	int ntris = 1;

	_tri **badtris = (_tri**)malloc(amount * sizeof(_tri*));
	_vert *edges = (_vert*)malloc(amount * sizeof(_vert) * 2);

	// Add all points one by one to the triangle list by recomputing triangles
	for(int i = 0; i < amount; i++){
		int nbadtris = 0;
		int nedges = 0;
		// Find all invalid triangles
		for(int j = 0; j < ntris; j++){
			_cir c;
			if(getCircumCircle(vs[i], tris[j], &c)){
				badtris[nbadtris] = &tris[j];
				nbadtris++;
			}
		}

		printf("%d ", nbadtris);

		// Find the polygonal boundary
		for(int j = 0; j < nbadtris; j++){
			_tri t1 = *(badtris[j]);
			for(int k = j + 1; k < nbadtris; k++){
				_tri t2 = *(badtris[k]);
				int edge = getSharedEdge(t2, t1.p1, t1.p2);
				if(edge == 1){
					edges[nedges++] = t2.p1;
					edges[nedges++] = t2.p2;
				}else if(edge == 2){
					edges[nedges++] = t2.p2;
					edges[nedges++] = t2.p3;
				}else if(edge == 3){
					edges[nedges++] = t2.p3;
					edges[nedges++] = t2.p1;
				}
			}
		}

		// Remove triangles from tris which are also in the boundary polygon
		for(int j = 0; j < nbadtris; j++){
			_tri *t = badtris[j];
			for(int k = ntris - 1; k >= 0; k++){
				if(tris + k == t){
					ntris--;
					memmove(tris + k, tris + k + 1, ntris - k);
				}
			}
		}

		for(int j = 0; j < nedges; j += 2){
			tris[ntris++] = (_tri){edges[j], edges[j + 1], vs[i]};
		}

		printf("%d\n", ntris);
	}

	free(badtris);
	free(tris);
	free(vs);

	return 0;
}
