/*
 * FishFight render engine
 * Licensed under the BSD license, see LICENSE in source root for details.
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <psprtc.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <pspgu.h>
#include <pspgum.h>

#include "callbacks.h"
#include "vram.h"
#include "graphics.h"
#include "mesh.h"
#include "fishfight.h"

PSP_MODULE_INFO("Fish Fight", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static unsigned int __attribute__((aligned(16))) list[262144];
unsigned char *logo_start;

struct Vertex
{
	float u, v;
	unsigned int color;
	float x,y,z;
};

struct Vertex __attribute__((aligned(16))) vertices[12*3] =
{
	// right (surf 0)
	{0, 0, 0xff007f00, 1,-1,-1}, // 2
	{1, 0, 0xff007f00, 1,-1, 1}, // 1
	{1, 1, 0xff007f00, 1, 1, 1}, // 5

	{0, 0, 0xff007f00, 1,-1,-1}, // 2
	{1, 1, 0xff007f00, 1, 1, 1}, // 5
	{0, 1, 0xff007f00, 1, 1,-1}, // 6

	// back (surf 1)
	{0, 0, 0xff7f0000,-1,-1,-1}, // 3
	{1, 0, 0xff7f0000, 1,-1,-1}, // 2
	{1, 1, 0xff7f0000, 1, 1,-1}, // 6

	{0, 0, 0xff7f0000,-1,-1,-1}, // 3
	{1, 1, 0xff7f0000, 1, 1,-1}, // 6
	{0, 1, 0xff7f0000,-1, 1,-1}, // 7

	// left (surf 2)
	{1, 0, 0xff007f00,-1,-1,-1}, // 3
	{1, 1, 0xff007f00,-1, 1,-1}, // 7
	{0, 1, 0xff007f00,-1, 1, 1}, // 4

	{1, 0, 0xff007f00,-1,-1,-1}, // 3
	{0, 1, 0xff007f00,-1, 1, 1}, // 4
	{0, 0, 0xff007f00,-1,-1, 1}, // 0

	// ground (surf 3)
	{0, 0, 0xff00007f,-1, 1,-1}, // 7
	{1, 0, 0xff00007f, 1, 1,-1}, // 6
	{1, 1, 0xff00007f, 1, 1, 1}, // 5

	{0, 0, 0xff00007f,-1, 1,-1}, // 7
	{1, 1, 0xff00007f, 1, 1, 1}, // 5
	{0, 1, 0xff00007f,-1, 1, 1}, // 4

	// sky (surf 4)
	{1, 0, 0xff00007f,-1,-1,-1}, // 3
	{1, 1, 0xff00007f,-1,-1, 1}, // 0
	{0, 1, 0xff00007f, 1,-1, 1}, // 1

	{1, 0, 0xff00007f,-1,-1,-1}, // 3
	{0, 1, 0xff00007f, 1,-1, 1}, // 1
	{0, 0, 0xff00007f, 1,-1,-1}, // 2

	// front (surf 5)
	{1, 0, 0xff7f0000,-1,-1, 1}, // 0
	{1, 1, 0xff7f0000,-1, 1, 1}, // 4
	{0, 1, 0xff7f0000, 1, 1, 1}, // 5

	{1, 0, 0xff7f0000,-1,-1, 1}, // 0
	{0, 1, 0xff7f0000, 1, 1, 1}, // 5
	{0, 0, 0xff7f0000, 1,-1, 1}, // 1

};

long int timeGetTime()
{
#if 1
	u64 tick;
	sceRtcGetCurrentTick(&tick);
#else
	u64 tick=sceKernelGetSystemTimeWide();
#endif
	return (long)tick;
}

void drawMesh(Mesh *mesh)
{
	if(mesh->mat.size()>0 && mesh->mat.begin()->textureName.size()>0) {
		Image *image=mesh->mat.begin()->texture;
		sceGuTexImage(0,image->imageWidth,image->imageHeight,image->textureWidth,image->data);
	}

	sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D,mesh->polyCount*3,mesh->poly,mesh->vert);
}

void drawFrame(Frame *frame)
{
	std::vector<Mesh *>::iterator p;

	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();

	sceGumMultMatrix((ScePspFMatrix4 *)frame->transform);

	for(p=frame->mesh.begin();p!=frame->mesh.end();p++) {
		Mesh *mesh=*p;
		drawMesh(mesh);
	}
	sceGumMatrixMode(GU_MODEL);
	sceGumPopMatrix();
}

void drawEntity(Entity *entity)
{
	std::vector<Frame *>::iterator p;

	if(!entity) return;

	for(p=entity->frame.begin();p!=entity->frame.end();p++) {
		Frame *frame=*p;
		drawFrame(frame);
	}
}

void drawEntity(Entity *entity,int &frameNo)
{
	if(frameNo>=entity->frame.size()) frameNo=0;

	Frame *frame=entity->frame[frameNo];
	drawFrame(frame);
}

void drawActor(Actor *actor)
{
	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumMultMatrix((ScePspFMatrix4 *)actor->getTransform());
	drawFrame(actor->getCurrLowerFrame());

	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumMultMatrix((ScePspFMatrix4 *)actor->getCurrLowerTransform());
	drawFrame(actor->getCurrUpperFrame());

	sceGumMatrixMode(GU_MODEL);
	sceGumPopMatrix();
	sceGumPopMatrix();
}

FishFight fishFight;

void display()
{
	sceGuStart(GU_DIRECT,list);

	// clear screen

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

	// setup matrices for cube

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(45.0f,16.0f/9.0f,1.5f,512.0f);

	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 pos = { 0, 0, -2.5f*6.0f };
		//ScePspFVector3 rot = { val * 0.79f * (GU_PI/180.0f), val * 0.98f * (GU_PI/180.0f), val * 1.32f * (GU_PI/180.0f) };
		float camerax=0.0;
		ScePspFVector3 rot= { 180*GU_PI/180.0f,-camerax * (GU_PI/180.0f),0};
		ScePspFVector3 scale = { 9.0f, 9.0f, 9.0f };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
		sceGumScale(&scale);
	}

	// setup texture

	sceGuTexMode(GU_PSM_8888,0,0,0);
	sceGuTexImage(0,512,512,512,logo_start);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexEnvColor(0xffff00);
	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexScale(1.0f,1.0f);
	//sceGuTexOffset(texu,texv);
	sceGuAmbientColor(0xffffffff);
	// draw skybox

	sceGuFrontFace(GU_CCW);
	sceGuDisable(GU_DEPTH_TEST);
	sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,12*3,0,vertices);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);

	// New object, new matrix
	sceGumLoadIdentity();
	{
		ScePspFVector3 pos = { 0, 0, -2.5f };
		//ScePspFVector3 rot = { val * 0.79f * (GU_PI/180.0f), val * 0.98f * (GU_PI/180.0f), val * 1.32f * (GU_PI/180.0f) };
		ScePspFVector3 rot= { -90*GU_PI/180.0f,0,0};
		ScePspFVector3 scale = { 0.25f, 0.25f, 0.25f };
		pos.z*=9;
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
		sceGumScale(&scale);
	}

	// Draw particular frame of Shinobu
	drawEntity(fishFight.sceneEntity);

	drawActor(fishFight.getHero());
#if 1
	std::vector<Actor *>::iterator p;
	for(p=fishFight.npc.begin();p!=fishFight.npc.end();p++) {
		Actor *actor=*p;
		drawActor(actor);
	}
#endif

	fishFight.updateLevel(timeGetTime());

	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();	// Probably wastes some cycles here.
	sceGuSwapBuffers();
}	

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
static int frameNo=0;
static int animationActive=0;

int main(int argc, char* argv[])
{
	SceCtrlData pad;

	setupCallbacks();

	// setup GU

	void* fbp0 = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	void* fbp1 = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	void* zbp = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_4444);


	pspDebugScreenInit();
	pspDebugScreenPrintf("Generic Survival Horror Clone #1\nLoading models...\n");
	//LoadM3T("skybox.m3t",&model);
	//Entity *model=load_xof("wolf-textest.x.gz");
	//Entity *model=load_md3("models/players/shinobu/");
	//pspDebugScreenPrintf("shinobu=%08x.\n",(unsigned int)model);
	//Entity *scene=load_xof("market1.x.gz");
	//pspDebugScreenPrintf("market1=%08x.\n",(unsigned int)scene);
	fishFight.newGame();
	pspDebugScreenPrintf("Press [X] to start.\n");

	do {
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(&pad,1);
	} while(!(pad.Buttons & PSP_CTRL_CROSS));
	// run sample

	float camerax=0.0,cameray=0.0,cameraz=1.0;
	int mode=0;

	// Load in the texture file.
	Image *bg=loadImage("skybox.png");
	if(bg) logo_start=(unsigned char *)bg->data;

	/* Properly map the surfaces to the skybox */
	int i,j;
	for(i=0;i<6;i++) {
		float texu,texv;
	
		texu=(i%3)/3.0f;
		texv=(i/3)/3.0f;
		for(j=i*6;j<(i+1)*6;j++) {
			vertices[j].u=vertices[j].u/3.0f+texu;
			vertices[j].v=vertices[j].v/3.0f+texv;
		}
	}
	pspDebugScreenPrintf("i=%d,j=%d\n",i,j);
	for(i=0;i<12*3;i++) {
		pspDebugScreenPrintf("verticies[%d].uv=(%.3f,%.3f)\n",i,vertices[i].u,vertices[i].v);
	}
	do {
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(&pad,1);
	} while((pad.Buttons & PSP_CTRL_CROSS));

#if 1
	sceGuInit();

	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888,fbp0,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,fbp1,BUF_WIDTH);
	sceGuDepthBuffer(zbp,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	//sceGuDisable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	int oldButtons=0;
	int stepMode=0;

	while(running())
	{
		int changedButtons;
		display();

        sceCtrlReadBufferPositive(&pad,1);
        changedButtons=oldButtons ^ pad.Buttons;
        unsigned long time=timeGetTime();
        if(changedButtons & PSP_CTRL_DOWN) {
			fishFight.buttonEvent(FF_DOWN,(pad.Buttons&PSP_CTRL_DOWN)!=0,time);
		}
        if(changedButtons & PSP_CTRL_UP) {
			fishFight.buttonEvent(FF_UP,(pad.Buttons&PSP_CTRL_UP)!=0,time);
		}
        if(changedButtons & PSP_CTRL_LEFT) {
			fishFight.buttonEvent(FF_LEFT,(pad.Buttons&PSP_CTRL_LEFT)!=0,time);
		}
        if(changedButtons & PSP_CTRL_RIGHT) {
			fishFight.buttonEvent(FF_RIGHT,(pad.Buttons&PSP_CTRL_RIGHT)!=0,time);
		}
        if(changedButtons & PSP_CTRL_SQUARE) {
			fishFight.buttonEvent(FF_SQUARE,(pad.Buttons&PSP_CTRL_SQUARE)!=0,time);
		}
        if(changedButtons & PSP_CTRL_CIRCLE) {
			fishFight.buttonEvent(FF_CIRCLE,(pad.Buttons&PSP_CTRL_CIRCLE)!=0,time);
		}
        if(changedButtons & PSP_CTRL_TRIANGLE) {
			fishFight.buttonEvent(FF_TRIANGLE,(pad.Buttons&PSP_CTRL_TRIANGLE)!=0,time);
		}
        if(changedButtons & PSP_CTRL_CROSS) {
			fishFight.buttonEvent(FF_CROSS,(pad.Buttons&PSP_CTRL_CROSS)!=0,time);
		}
        if(changedButtons & PSP_CTRL_SELECT) {
			fishFight.buttonEvent(FF_SELECT,(pad.Buttons&PSP_CTRL_SELECT)!=0,time);
		}
        if(changedButtons & PSP_CTRL_START) {
			fishFight.buttonEvent(FF_START,(pad.Buttons&PSP_CTRL_START)!=0,time);
		}
        if(changedButtons & PSP_CTRL_HOLD) {
			fishFight.buttonEvent(FF_HOLD,(pad.Buttons&PSP_CTRL_HOLD)!=0,time);
		}
		oldButtons=pad.Buttons;	
	}

	sceGuTerm();
#endif

	sceKernelExitGame();
	return 0;
}
