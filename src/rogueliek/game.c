#include "window.h"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

int main(int argc, char **argv)
{
	createWindow("rogueliek", DEFAULT_WIDTH, DEFAULT_HEIGHT);
	hideCursor();

	while(true){
		if(!updateWindow()){
			break;
		}
		renderWindow(6);
	}

	showCursor();
	destroyWindow();
	return 0;
}
