#pragma once

#include <stdbool.h>

void createWindow(const char *title, int width, int height);
void destroyWindow();

bool updateWindow();
void renderWindow(int ms);

int pickFontFromDir(const char *dir);

void hideCursor();
void showCursor();
