#include "utils.h"

#include <string.h>
#include <ccore/file.h>

char *findFileFromExtension(const char *dir, const char *extension)
{
	ccFileDir file;
	if(ccFileDirFindFirst(&file, dir) != CC_SUCCESS){
		fprintf(stderr, "Can not open directory \"%s\"\n", dir);
		return NULL;
	}
	
	while(ccFileDirFind(&file) == CC_SUCCESS){
		if(file.isDirectory){
			continue;
		}

		const char *ext = strrchr(file.name, '.');
		if(!ext || ext == file.name){
			continue;
		} else if(strcmp(ext + 1, extension) == 0){
			char *ffile = (char*)calloc(strlen(dir) + strlen(file.name) + 1, 1);
			strcpy(ffile, dir);
			strcpy(ffile + strlen(dir), file.name);
			return ffile;
		}
	}

	return NULL;
}
