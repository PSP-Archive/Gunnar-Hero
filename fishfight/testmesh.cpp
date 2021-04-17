#include <stdlib.h>

#include<vector>
#include<map>
#include<string>

#include "mesh.h"
#include "loadpng.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int main(int argc, char* argv[])
{
	printf("Started.\n");
	static Entity *model=0;

	model=load_md3("models/players/shinobu/");
	printf("Calling free entity\n");
	free_entity(model);
	printf("Done free entity\n");
	model=0;
	return 0;
}

