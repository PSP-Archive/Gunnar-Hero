#include "md3.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int main(int argc,char **argv)
{
	MD3File md3;

	printf("Dumping file:\n");
	printf("'%s'\n",argv[1]);
	int rc=md3.load(argv[1]);
	printf("Returned: %d\n",rc);
	md3.dump(stdout);
}
