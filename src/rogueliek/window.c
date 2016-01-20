#include <stdbool.h>

#include <ccFont/ccFont.h>
#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/time.h>
#include <ccore/file.h>

#include "utils.h"

#ifdef WINDOWS
#include <gl/GL.h>
#else
#include <GL/glew.h>
#endif

typedef struct {
	unsigned char r, g, b;
} pixel_t;

typedef struct {
	char c;
	unsigned char r, g, b;
} rchar_t;

static GLuint gltex;
static ccfFont font;
static rchar_t *letters;
static pixel_t *pixels;
static int wwidth, wheight, lwidth, lheight;
static bool updatescreen;

static void renderLetters()
{
	memset(pixels, 0, wwidth * wheight * sizeof(pixel_t));

	ccfFontConfiguration conf = {.x = 0, .width = 0, .wraptype = 0};

	int y;
	for(y = 0; y < lheight; y++){
		conf.y = y * font.gheight;
		int x;
		for(x = 0; x < lwidth; x++){
			conf.x = x * font.gwidth;
			rchar_t l = letters[x + y * lwidth];
			if(l.r == 0 && l.g == 0 && l.b == 0){
				continue;
			}
			conf.color[0] = l.r / 255.0;
			conf.color[1] = l.g / 255.0;
			conf.color[2] = l.b / 255.0;
			ccfGLTexBlitChar(&font, l.c, &conf, wwidth, wheight, GL_RGB, GL_UNSIGNED_BYTE, (void*)pixels);
		}
	}
}

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

	wwidth = width;
	wheight = height;
	pixels = (pixel_t*)calloc(wwidth * wheight, sizeof(pixel_t));

	lwidth = width / font.gwidth;
	lheight = height / font.gheight;
	letters = (rchar_t*)calloc(lwidth * lheight, sizeof(rchar_t));

	updatescreen = true;
}

void destroyWindow()
{
	ccFree();
	free(letters);
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
	if(updatescreen){
		renderLetters();
		updatescreen = false;
	}
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wwidth, wheight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f, 1.0f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);

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

int pickFontFromDir(const char *assetdir)
{
	char *file = findFileFromExtension(assetdir, "ccf");
	if(file == NULL){
		return -1;
	}

	unsigned len = ccFileInfoGet(file).size;

	FILE *fp = fopen(file, "rb");
	if(!fp){
		fprintf(stderr, "Can not open file: %s\n", file);
		exit(1);
	}

	unsigned char *bin = (unsigned char*)malloc(len);
	fread(bin, 1, len, fp);

	fclose(fp);

	if(ccfBinToFont(&font, bin, len) == -1){
		fprintf(stderr, "Binary font failed: invalid version\n");
		exit(1);
	}

	free(bin);
	free(file);

	return 0;
}

void drawChar(int x, int y, char c, unsigned char r, unsigned char g, unsigned char b)
{
	if(x < 0 || y < 0 || x > lwidth || y > lwidth){
		return;
	}

	letters[x + y * lwidth] = (rchar_t){.c = c, .r = r, .g = g, .b = b};

	updatescreen = true;
}

void drawString(int x, int y, const char *text, unsigned char r, unsigned char g, unsigned char b)
{
	size_t len = strlen(text);

	int i;
	for(i = 0; i < len; i++){
		drawChar(x + i, y, text[i], r, g, b);
	}
}

int getWidth()
{
	return lwidth;
}

int getHeight()
{
	return lheight;
}
