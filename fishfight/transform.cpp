#include <math.h>

#include "transform.h"


void multTransform(float *t1,float *t2)
{
	int i,j,k;
	float r[16];
	for(i=0;i<16;i++) r[i]=0;
	for(i=0;i<4;i++) {
		for(j=0;j<4;j++) {
			for(k=0;k<4;k++) {
				r[i*4+j]+=t1[i*4+k]*t2[k*4+j];
			}
		}
	}
#ifdef _DEBUG
	for(i=0;i<16;i+=4) {
		printf("[%.2f %.2f %.2f %.2f]x",t1[i+0],t1[i+1],t1[i+2],t1[i+3]);
		printf("[%.2f %.2f %.2f %.2f]=",t2[i+0],t2[i+1],t2[i+2],t2[i+3]);
		printf("[%.2f %.2f %.2f %.2f]\n",r[i+0],r[i+1],r[i+2],r[i+3]);
	}
	printf("\n");
#endif
	for(i=0;i<16;i++) t1[i]=r[i];
}

void rotateTransformXYZ(float *transform,float xrot,float yrot,float zrot)
{
	float t[16];
	int i;
	int from[]={1,0,0};
	int to[]={2,2,1};
	float angle[3];
	int dir;

	angle[0]=xrot;
	angle[1]=yrot;
	angle[2]=zrot;

	for(dir=0;dir<3;dir++) {
		for(i=0;i<16;i++) t[i]=(i%4)==i/4?1:0;
		float c=cosf(angle[dir]);
		float s=sinf(angle[dir]);
		t[from[dir]*4+from[dir]]=c;
		t[to[dir]*4+from[dir]]=-s;
		t[from[dir]*4+to[dir]]=s;
		t[to[dir]*4+to[dir]]=c;

		multTransform(transform,t);
	}
}

void scaleTransform(float *transform,float x,float y,float z)
{
	float t[16];
	int i;

	for(i=0;i<16;i++) t[i]=(i%4)==i/4?1:0;
	t[0]=x;
	t[5]=y;
	t[10]=z;
	multTransform(transform,t);
}

void translateTransform(float *transform,float x,float y,float z)
{
	float t[16];
	int i;

	for(i=0;i<16;i++) t[i]=(i%4)==i/4?1:0;
	t[12]=x;
	t[13]=y;
	t[14]=z;
	multTransform(transform,t);
}
