#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/time.h>
#include <ccore/file.h>

#ifdef WINDOWS
#include <gl/GL.h>
#else
#include <GL/glew.h>
#endif

void createWindow(const char *title, int width, int height)
{
	ccDisplayInitialize();
	ccWindowCreate((ccRect){0, 0, width, height}, title, 0);

	ccGLContextBind();
}

void destroyWindow()
{
	ccFree();
}
