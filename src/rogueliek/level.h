#pragma once

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

void levelRegisterLua(lua_State *lua);

void generateMap(int width, int height, int seed, int scale, int erosionpasses);

void renderMap(int x, int y, int width, int height, int mapx, int mapy);
