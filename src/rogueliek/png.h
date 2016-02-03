#pragma once

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

void pngRegisterLua(lua_State *lua);

int loadPng(const char *file, const char *name);

int getPngId(const char *name);

int getPngWidth(int id);
int getPngHeight(int id);
const unsigned char *getPngData(int id);

void renderPng(const char *name, int x, int y);
