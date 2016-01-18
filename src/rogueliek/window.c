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

static GLuint gltex;

void createWindow(const char *title, int width, int height)
{
	ccDisplayInitialize();
	ccWindowCreate((ccRect){0, 0, width, height}, title, 0);

	ccGLContextBind();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void destroyWindow()
{
	ccFree();
}

bool updateWindow()
{
	while(ccWindowEventPoll()){
		ccEvent event = ccWindowEventGet();
		switch(event.type){
			case CC_EVENT_WINDOW_QUIT:
				return false;
				break;
			case CC_EVENT_KEY_DOWN:
				if(event.keyCode == CC_KEY_ESCAPE){
					return false;
				}
				break;
			default: break;
		}
	}

	return true;
}

void renderWindow(int ms)
{
	glClear(GL_COLOR_BUFFER_BIT);

	ccGLBuffersSwap();
	ccTimeDelay(ms);
}

void hideCursor()
{
	ccWindowMouseSetCursor(CC_CURSOR_NONE);
}

void showCursor()
{
	ccWindowMouseSetCursor(CC_CURSOR_ARROW);
}
