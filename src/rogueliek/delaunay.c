#include "delaunay.h"

#include <stdlib.h>
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

typedef struct {
	float x, y;
} _vert;

typedef struct {
	_vert p1, p2, p3;
} _tri;

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

static inline bool pointInCircumcircle(_vert p, _tri t)
{
	float px2 = p.x * p.x;
	float py2 = p.y * p.y;

	float a = t.p1.x - p.x;
	float b = t.p1.y - p.y;
	float c = (t.p1.x * t.p1.x - px2) + (t.p1.y * t.p1.y - py2);
	
	float d = t.p2.x - p.x;
	float e = t.p2.y - p.y;
	float f = (t.p2.x * t.p2.x - px2) + (t.p2.y * t.p2.y - py2);

	float g = t.p3.x - p.x;
	float h = t.p3.y - p.y;
	float i = (t.p3.x * t.p3.x - px2) + (t.p3.y * t.p3.y - py2);

	float determinant = a * e * i + b * f * g - c * e * g - b * d * i - a * f * h;
	return determinant > 0;
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
	_vert *vs = (_vert*)malloc(amount * sizeof(_vert));

	for(int i = 0; i < amount; i++){
		vs[i].x = x[i];
		vs[i].y = y[i];
	}

	sortPositions(vs, amount);

	// Find super triangle
	float xmin = FLT_MAX;
	float ymin = FLT_MAX;
	float xmax = FLT_MIN;
	float ymax = FLT_MIN;

	for(int i = 0; i < amount; i++){
		xmin = min(xmin, vs[i].x);
		ymin = min(ymin, vs[i].y);
		xmax = max(xmax, vs[i].x);
		ymax = max(ymax, vs[i].y);
	}

	float dx = xmax - xmin;
	float dy = ymax - ymin;
	float dmax = max(dx, dy);
	float xmid = xmin + dx * 0.5f;
	float ymid = ymin + dy * 0.5f;

	_vert sp1 = {.x = xmid - 20 * dmax, .y = ymid - dmax};
	_vert sp2 = {.x = xmid, .y = ymid + 20 * dmax};
	_vert sp3 = {.x = xmid + 20 * dmax, .y = ymid - dmax};

	// Create triangle list and add the super triangle to it
	_tri *tris = (_tri*)malloc(amount * sizeof(_tri));
	tris[0] = (_tri){sp1, sp2, sp3};
	int ntris = 1;

	_tri **badtris = (_tri**)malloc(amount * sizeof(_tri*));
	int nbadtris = 0;

	_vert *poly = (_vert*)malloc(amount * sizeof(_vert) * 2);
	int npoly = 0;

	// Add all points one by one to the triangle list by recomputing triangles
	for(int i = 0; i < amount; i++){
		nbadtris = 0;
		// Find all invalid triangles
		for(int j = 0; j < ntris; j++){
			if(pointInCircumcircle(vs[i], tris[j])){
				badtris[nbadtris] = &tris[j];
				nbadtris++;
			}
		}

		// Find the polygonal boundary
		for(int j = 0; j < nbadtris; j++){
			_tri t1 = *(badtris[j]);
			for(int k = j + 1; k < nbadtris; k++){
				_tri t2 = *(badtris[k]);
				int edge = getSharedEdge(t2, t1.p1, t1.p2);
				if(edge == 1){
					poly[npoly++] = t2.p1;
					poly[npoly++] = t2.p2;
				}else if(edge == 2){
					poly[npoly++] = t2.p2;
					poly[npoly++] = t2.p3;
				}else if(edge == 3){
					poly[npoly++] = t2.p3;
					poly[npoly++] = t2.p1;
				}
			}
		}

		// Remove triangles from tris which are also in the boundary polygon
		for(int j = 0; j < nbadtris; j++){
			_tri *t = badtris[j];
			for(int k = ntris - 1; k >= 0; k++){
				if(tris + i == t){
					ntris--;
				}
			}
		}

		printf("%d\n", nbadtris);
	}

	free(badtris);
	free(tris);
	free(vs);

	return 0;
}
