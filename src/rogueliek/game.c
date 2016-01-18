#include "window.h"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

int main(int argc, char **argv)
{
	createWindow("rogueliek", DEFAULT_WIDTH, DEFAULT_HEIGHT);

	destroyWindow();
	return 0;
}
