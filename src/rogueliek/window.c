#include <ccFont/ccFont.h>
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
static ccfFont font;

static void loadFont(const char *file, char type)
{
	if(type != 'c'){
		fprintf(stderr, "Non ccf binary font not implemented yet!\n");
		exit(1);
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
	
int pickFontFromDir(const char *assetdir)
{
	ccFileDir file;
	if(ccFileDirFindFirst(&file, assetdir) != CC_SUCCESS){
		fprintf(stderr, "Can not open asset directory \"%s\"\n", assetdir);
		exit(1);
	}
	
	while(ccFileDirFind(&file) == CC_SUCCESS){
		if(file.isDirectory){
			continue;
		}

		const char *ext = strrchr(file.name, '.');
		if(!ext || ext == file.name){
			continue;
		} else if(strcmp(ext + 1, "ccf") == 0){
			char fontfile[strlen(assetdir) + strlen(file.name) + 1];
			strcpy(fontfile, assetdir);
			strcpy(fontfile + strlen(assetdir), file.name);
			loadFont(fontfile, 'c');
			return 0;
		}
	}

	return -1;
}

void hideCursor()
{
	ccWindowMouseSetCursor(CC_CURSOR_NONE);
}

void showCursor()
{
	ccWindowMouseSetCursor(CC_CURSOR_ARROW);
}
