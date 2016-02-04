#pragma once

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

void delaunayRegisterLua(lua_State *lua);

int delaunayTriangulate(int **ids, const float *x, const float *y, int amount);
