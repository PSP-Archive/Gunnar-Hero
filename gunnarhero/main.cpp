/*
 * FishFight render engine
 * Licensed under the BSD license, see LICENSE in source root for details.
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <psprtc.h>
#include <pspaudiolib.h>
#include <psppower.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#include <deque>

#include <pspgu.h>
#include <pspgum.h>

//#define MIKMOD_PLAYER
#ifdef MIKMOD_PLAYER
#include <mikmod.h>
#endif

#include "callbacks.h"
#include "vram.h"
#include "graphics.h"
#include "mesh.h"
#include "gunnarhero.h"
#include "mp3player.h"
//#include "fontloader.h"
//#include "mjpegplayer.h"

PSP_MODULE_INFO("Gunnar Hero", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static unsigned int __attribute__((aligned(16))) gulist[262144];

int irfd=-1;	// Infra Red file descriptor

//Font *font;
Image *numbers;

long int timeGetTime()
{
	u64 tick;
	sceRtcGetCurrentTick(&tick);

	return (long)tick/1000;
}

bool vramIt=false;

void swizzleFast(Image *source)
{
   unsigned int width=source->textureWidth*4;
   unsigned int height=source->textureHeight;
   u32 *out=0;
   if(vramIt) {
	out=(u32 *)getStaticVramTexture(source->textureWidth,source->textureHeight,GU_PSM_8888);
	printf( ">=>= vraming %dx%d texture\n",source->textureWidth,source->textureHeight);
   } else {
	out=(u32 *)memalign(16,width*height);
   }
   unsigned int blockx, blocky;
   int i;
 
   unsigned int widthBlocks = (width / 16);
   unsigned int heightBlocks = (height / 8);
 
   unsigned int srcPitch = (width-16)/4;
   unsigned int srcRow = width * 8;
 
   const u8* ysrc = (u8 *)source->data;;
   u32* dst = out;
 
   for (blocky = 0; blocky < heightBlocks; ++blocky) {
      const u8* xsrc = ysrc;
      for (blockx = 0; blockx < widthBlocks; ++blockx) {
         const u32* src = (u32*)xsrc;
         for (i=0;i<8;i++) {
            *(dst++) = *(src++);
            *(dst++) = *(src++);
            *(dst++) = *(src++);
            *(dst++) = *(src++);
            src += srcPitch;
         }
         xsrc += 16;
     }
     ysrc += srcRow;
   }
   free(source->data);
   source->data=(Color *)out;
   source->swizzled=1;
}

void drawMesh(Mesh *mesh)
{
#if FF_TEXTURE_ENABLE
#if 0
	printf("mesh bounding sphere: %.2f,%.2f,%.2f (%.2f radius)\n",
		mesh->boundingSphere[0],
		mesh->boundingSphere[1],
		mesh->boundingSphere[2],
		mesh->boundingSphere[3]);
	printf("mesh bounding cube: (%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)\n",
		mesh->boundingRect[0],
		mesh->boundingRect[1],
		mesh->boundingRect[2],
		mesh->boundingRect[3],
		mesh->boundingRect[4],
		mesh->boundingRect[5]);
#endif
	if(mesh->mat.size()>0 && mesh->mat.begin()->textureName.size()>0) {
		Image *image=mesh->mat.begin()->texture;
		if(!image) {
			printf("Mesh missing texture %s\n",mesh->mat.begin()->textureName.c_str());
		} else {
			if(image->swizzled==0) {
				if(mesh->mat.begin()->textureName.compare("map1.bmp")==0 ||
				mesh->mat.begin()->textureName.compare("leaf.bmp")==0 ||
				mesh->mat.begin()->textureName.compare("backmap.png")==0 ) {
					vramIt=true;
				}
				swizzleFast(image);
				vramIt=false;
			}
			sceGuTexMode(GU_PSM_8888,0,0,image->swizzled);
			sceGuTexImage(0,image->imageWidth,image->imageHeight,image->textureWidth,image->data);
		}
	}
	sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D,mesh->polyCount*3,mesh->poly,mesh->vert);
#else
#if FF_VERTEXCOLOR_ENABLE
	sceGumDrawArray(GU_TRIANGLES,GU_NORMAL_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D,mesh->polyCount*3,mesh->poly,mesh->vert);
#else
	sceGuColor(0xff4488);
	sceGumDrawArray(GU_TRIANGLES,GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D,mesh->polyCount*3,mesh->poly,mesh->vert);
#endif
#endif
}

void drawFrame(Frame *frame)
{
	std::vector<Frame *>::iterator q;
	std::vector<Mesh *>::iterator p;

	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();

	sceGumMultMatrix((ScePspFMatrix4 *)frame->transform);

	for(q=frame->frame.begin();q!=frame->frame.end();q++) {
		Frame *frame=*q;
		drawFrame(frame);
	}
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

void blitAlphaImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy)
{
	sceGuEnable(GU_BLEND);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	if(source->swizzled==0) {
		swizzleFast(source);
	}
	sceGuTexMode(GU_PSM_8888,0,0,source->swizzled);
	sceGuTexImage(0, source->textureWidth, source->textureHeight, source->textureWidth, (void*) source->data);
	float u = 1.0f / ((float)source->textureWidth);
	float v = 1.0f / ((float)source->textureHeight);
	sceGuTexScale(u, v);
	sceGuDisable(GU_DEPTH_TEST);
	
	int j = 0;
	while (j < width) {
		struct Vertex {
			unsigned short u,v;
			short x,y,z;
		}* vertices = (Vertex*) sceGuGetMemory(2 * sizeof(Vertex));
		int sliceWidth = 64;
		if (j + sliceWidth > width) sliceWidth = width - j;
		vertices[0].u = sx + j;
		vertices[0].v = sy;
		vertices[0].x = dx + j;
		vertices[0].y = dy;
		vertices[0].z = 0;
		vertices[1].u = sx + j + sliceWidth;
		vertices[1].v = sy + height;
		vertices[1].x = dx + j + sliceWidth;
		vertices[1].y = dy + height;
		vertices[1].z = 0;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}

	sceGuEnable(GU_DEPTH_TEST);
	sceGuTexScale(1.0, 1.0);
	sceGuTexMode(GU_PSM_8888,0,0,0);
	//sceGuDisable(GU_BLEND);
	//sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	//sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

void PrintNumber(int x,int y,const char *message)
{
	static const struct FontMap {
		char ch;
		int left,top,width,height;
		int kern_left,kern_bottom,kern_right;
	} map[]={
		{ '0', 1,1,11,17, 0,-2,2 },
		{ '1', 13,1,8,17, 0,-2,2 },
		{ '2', 22,1,12,17, 0,-2,2 },
		{ '3', 35,1,11,17, 0,-2,2 },
		{ '4', 47,1,12,17, 0,-2,2 },
		{ '5', 1,19,11,17, 0,-2,2 },
		{ '6', 13,19,11,17, 0,-2,2 },
		{ '7', 25,19,11,17, 0,-2,2 },
		{ '8', 37,19,11,17, 0,-2,2 },
		{ '9', 49,19,11,17, 0,-2,2 },
		{ ':', 60,1,4,13, 0,-2,6 },
	};
	unsigned int i;
	if(!numbers) numbers=loadImage("data/ArialNum_15.png");
	if(!numbers) return;
	for(i=0;i<strlen(message);i++) {
		if(message[i]>='0' && message[i]<='9' || message[i]==':') {
			int ch=message[i]-'0';
			if(message[i]==':') ch=10;
			blitAlphaImageToScreen(map[ch].left,map[ch].top,
				map[ch].width,map[ch].height,numbers,
				x+map[ch].kern_left,y+map[ch].kern_bottom);
			x+=map[ch].width+map[ch].kern_right;
		} else {
			x+=19;
		}
	}
}
void printFastFont(FastFont *font,int x,int y,const char *message)
{
	unsigned int i;
	if(!font->texture) return;
	for(i=0;i<strlen(message);i++) {
		int ch=(unsigned char)message[i];
		if(font->glyph[ch].active) {
			blitAlphaImageToScreen(font->glyph[ch].x,
				font->glyph[ch].y,font->glyph[ch].w,
				font->glyph[ch].h,font->texture,
				x+font->glyph[ch].kern_left,
				y+font->glyph[ch].kern_bottom);
			x+=font->glyph[ch].w+font->glyph[ch].kern_right+1;
		} else {
			printf("Missing font char: '%c'\n",(char)ch);
			x+=font->height;
		}
	}
}

GunnarHero fishFight;
void *drawBuffer;
#define LIGHT_DISTANCE 3
float camerax=200,cameray=165,cameraz=0,cameradist=200;
float cameraangle=0.0f,cameraup=0.0f;

unsigned long currentFrame,oldFrame,frozenTime;

void display()
{
	currentFrame=timeGetTime()-frozenTime;
	sceGuStart(GU_DIRECT,gulist);

	// clear screen
	sceGuClearColor(0x00000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(45.0f,16.0f/9.0f,1.5f,768.0f);

	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	{
	ScePspFVector3 from={ fishFight.cameraStage.from[0],fishFight.cameraStage.from[1],fishFight.cameraStage.from[2]};
	ScePspFVector3 to= { fishFight.cameraStage.to[0],fishFight.cameraStage.to[1],fishFight.cameraStage.to[2] };
	ScePspFVector3 up= { 0,1,0 };
	// Now add in camera look around.
#if 0
	//printf("stage from: %.2f,%.2f,%.2f to: %.2f,%.2f,%.2f ",from.x,from.y,from.z,to.x,to.y,to.z);
	from.x=to.x+camerax;
	from.y=to.y+cameray;
	from.z=to.z+cameraz;
	//printf("dist=%.2f, angle %.2f x %.2f actual from: %.2f,%.2f,%.2f\n",cameradist,cameraangle,cameraup,from.x,from.y,from.z);
#endif

	sceGumLookAt(&from,&to,&up);
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	}

	std::deque<Piece *>::iterator p;
	// Draw eye candy.
	//sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	for(p=fishFight.piece.begin();p!=fishFight.piece.end();p++) {
		Piece *piece=*p;
		if(piece->isHidden()) continue;
		if(piece->type==PT_ROOM || piece->type==PT_BOARD) {
			sceGumPushMatrix();
			sceGumMultMatrix((ScePspFMatrix4 *)piece->getTransform());
			drawEntity(piece->getModel());
			sceGumPopMatrix();
		}
	}

#if 1
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	{
	ScePspFVector3 from={ fishFight.cameraBoard.from[0],
		fishFight.cameraBoard.from[1],
		fishFight.cameraBoard.from[2]};
	ScePspFVector3 to= { fishFight.cameraBoard.to[0],
		fishFight.cameraBoard.to[1],
		fishFight.cameraBoard.to[2] };
	ScePspFVector3 up= { 0,1,0 };
	sceGumLookAt(&from,&to,&up);
	}

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
#endif

#if 1
	sceGuClear(GU_DEPTH_BUFFER_BIT);
	// Draw solid objects. (should be front to back)
	//sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	for(p=fishFight.piece.begin();p!=fishFight.piece.end();p++) {
		Piece *piece=*p;
		if(piece->isHidden()) continue;
		if(piece->type==PT_ROOM || piece->type==PT_BOARD) continue;
			if(piece->transparent==false) {
			sceGumPushMatrix();
			sceGumMultMatrix((ScePspFMatrix4 *)piece->getTransform());
			drawEntity(piece->getModel());
			sceGumPopMatrix();
		}
	}
	// Now draw objects with transparency. (should be back to front)
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	for(p=fishFight.piece.begin();p!=fishFight.piece.end();p++) {
		Piece *piece=*p;
		if(piece->type==PT_ROOM || piece->type==PT_BOARD) continue;
		if(piece->transparent==true) {
			sceGumPushMatrix();
			sceGumMultMatrix((ScePspFMatrix4 *)piece->getTransform());
			drawEntity(piece->getModel());
			sceGumPopMatrix();
		}
	}

#endif
	//currentFrame=oldFrame+1000/24;		// for vid capture
	fishFight.updateLevel(currentFrame);

	std::deque<Overlay *>::iterator q;
	for(q=fishFight.overlay.begin();q!=fishFight.overlay.end();q++) {
		Overlay *over=*q;
		if(over->subimage) 
			blitAlphaImageToScreen(over->ox,over->oy,over->ow,over->oh,over->image,over->x,over->y);
		else
			blitAlphaImageToScreen(0,0,over->image->imageWidth,over->image->imageHeight,over->image,over->x,over->y);
	}
	std::deque<Message *>::iterator r;
	for(r=fishFight.message.begin();r!=fishFight.message.end();r++) {
		Message *msg=*r;
		printFastFont(msg->font,msg->x,msg->y,msg->message.c_str());
	}

	sceGuFinish();
	sceKernelDcacheWritebackAll();
	sceGuSync(0,0);

	sceGuStart(GU_DIRECT,gulist);

	sceGuFinish();
	sceKernelDcacheWritebackAll();
	sceGuSync(0,0);
	

	pspDebugScreenSetXY(0,0);
	//Piece *piece=*(fishFight.piece.begin());
	//float *t=piece->getTransform();
	//pspDebugScreenPrintf("piece: %.2f,%.2f,%.2f\n",t[12],t[13],t[14]);
	//pspDebugScreenPrintf("Frame count %d\n",piece->getModel()->frame.size());
//pspDebugScreenPrintf("Camera: %.3f %.3f %.3f\n",camerax,cameray,cameraz);

#if 0
	if(piece->getModel()->frame.size()>0) {
		Frame *f=*piece->getModel()->frame.begin();
		pspDebugScreenPrintf("Mesh count %d\n",f->mesh.size());
		if(f->mesh.size()>0) {
			Mesh *m=*f->mesh.begin();
			pspDebugScreenPrintf("Mesh verts: %d, polys: %d\n",m->vertCount,m->polyCount);
		}
	}
#endif
	int elapsed=currentFrame-oldFrame;
	if(elapsed>1000) elapsed=1000;
	if(elapsed<1) elapsed=1;
//	pspDebugScreenPrintf("FPS %.2f (%d usec)",1000.0f/elapsed,elapsed);
	oldFrame=currentFrame;

//	sceDisplayWaitVblankStart();	// Probably wastes some cycles here.
	drawBuffer=sceGuSwapBuffers();
	pspDebugScreenSetOffset((int)drawBuffer);
}	

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)

#define MAXSONG 5

#ifdef MIKMOD_PLAYER
SAMPLE *sfx[S_MAX];
UNIMOD *song[MAXSONG];
#endif
char *sfxpath[]={
	"intro.wav",
	"start.wav",
	"winner.wav",
	"computerwon.wav",
	"droppiece.wav",
	"badmove.wav",
	"arrow.wav",
};

#ifdef MIKMOD_PLAYER
void mikmodErrorHandler(void)
{
	printf("_mm_critical %d\n", _mm_critical);
	printf("_mm_errno %d\n", _mm_errno);
	printf("%s\n", _mm_errmsg[_mm_errno]);
	return;
}
#endif

void playSound(int id)
{
	if(fishFight.muteSfx) return;
#ifdef MIKMOD_PLAYER
	if(id>=0 && id<S_MAX && sfx[id]!=0) {
		int voice = MikMod_PlaySample(sfx[id],0,0);
		printf("playing %s on voice %d\n",sfxpath[id],voice);
	}
#endif
	return;
}

void saveScreenshot(const char *fname)
{
	saveImage(fname,(Color *)(0x04000000+(char *)drawBuffer),480,272,512,0);
}


unsigned int colors[4] = { 0xffffffff, 0xff00ff00, 0xff0000ff, 0xffff00ff };

int main(int argc, char* argv[])
{
	SceCtrlData pad;

	setupCallbacks();
	scePowerSetClockFrequency(333, 333, 166);

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);

	pspDebugScreenInit();
	// setup GU

	void* fbp0 = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	void* fbp1 = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	void* zbp = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_4444);

	Image *source; //=loadImage("data/bg.png");
	source=loadImage("data/pxpcompo.png");

	sceGuInit();
	sceGuStart(GU_DIRECT,gulist);
	sceGuDrawBuffer(GU_PSM_8888,fbp0,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,fbp1,BUF_WIDTH);
	sceGuClearColor(0xff774433);
	sceGuClear(GU_COLOR_BUFFER_BIT);
#if 1
	if(source) 
		sceGuCopyImage(GU_PSM_8888, 0, 0, source->imageWidth, source->imageHeight, source->textureWidth, source->data, 0, 0, PSP_LINE_SIZE, (Color *)((unsigned int)drawBuffer+0x4000000));
#else
	if(source) 
		blitAlphaImageToScreen(0, 0, source->imageWidth, source->imageHeight, source, 0, 0);
#endif
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	drawBuffer=sceGuSwapBuffers();
	sceGuDisplay(GU_TRUE);

#ifdef MIKMOD_PLAYER
	_mm_RegisterErrorHandler(mikmodErrorHandler);
	MikMod_RegisterAllLoaders();
	MikMod_RegisterAllDrivers();
	md_mode = DMODE_16BITS|DMODE_STEREO|DMODE_SOFT_SNDFX|DMODE_SOFT_MUSIC;
	md_reverb = 0;
	md_pansep = 128;
	MikMod_Init();
	MikMod_SetNumVoices(-1, 6);

	Player_Start(NULL);

	int i;
	for(i=0;i<S_MAX;i++) {
		printf("reading sound file %d...\n",i);
		FILE *file=fopen(sfxpath[i],"rb");
		if(file) {
			printf("opening file %s\n",sfxpath[i]);
			fclose(file);
			sfx[i]=WAV_LoadFN(sfxpath[i]);
		}
		if(!sfx[i]) printf("Warning: missing sound file '%s'\n",sfxpath[i]);
	}
#else
	int i;
	pspAudioInit();
	printf("Initialized the audio subsystem\n");
#endif	

	playSound(S_INTRO);
	//playMJPEG("intro.mjpeg",30);


#if 0
	font=Font_Load("data/arial.ttf");
	Font_SetSize(font,18);
#endif

	//pspDebugScreenPrintf("PSP Connect 4\nLoading models...\n");
	fishFight.newGame();
	fishFight.load(1);

	if(source) freeImage(source);

#if 0
	source=loadImage("data/pxpcompo.png");
	if(source) {
		sceGuStart(GU_DIRECT,gulist);
		sceGuCopyImage(GU_PSM_8888, 0, 0, source->imageWidth, source->imageHeight, source->textureWidth, source->data, 0, 0, PSP_LINE_SIZE, (Color *)((unsigned int)drawBuffer+0x4000000));
		
		sceGuFinish();
		sceGuSync(0,0);
		sceDisplayWaitVblankStart();
		drawBuffer=sceGuSwapBuffers();

		// Now wait a while
#if 0
		int i;
		for(i=0;i<120;i++) {
			sceDisplayWaitVblankStart();
		}
#endif
	}
	if(source) freeImage(source);
#endif
	source=0;
#if 0
	pspDebugScreenPrintf("Press [X] to start.\n");

	do {
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(&pad,1);
	} while(!(pad.Buttons & PSP_CTRL_CROSS));
	// run sample

	do {
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(&pad,1);
	} while((pad.Buttons & PSP_CTRL_CROSS));
#endif

#if 1

	sceGuStart(GU_DIRECT,gulist);
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
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	//sceGuEnable(GU_LINE_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	//sceGuDisable(GU_CLIP_PLANES);
#ifdef FF_TEXTURE_ENABLE
	sceGuTexMode(GU_PSM_8888,0,0,0);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
	//sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexEnvColor(0xffff00);
	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexScale(1.0f,1.0f);
#endif
#if 1
	sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	//sceGuEnable(GU_LIGHT1);
	//sceGuEnable(GU_LIGHT2);
	//sceGuEnable(GU_LIGHT3);
#endif
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	int oldButtons=0;

	while(running())
	{
		int changedButtons;

		ScePspFVector3 pos = { camerax, cameray, cameraz };
		sceGuStart(GU_DIRECT,gulist);
		sceGuLight(0,GU_POINTLIGHT,GU_AMBIENT_AND_DIFFUSE,&pos);
		sceGuLightColor(0,GU_AMBIENT,GU_COLOR(0.25f,0.25f,0.25f,1.0f));
		sceGuLightColor(0,GU_DIFFUSE,GU_COLOR(0.5f,0.5f,0.5f,1.0f));
		sceGuFinish();
		sceGuSync(0,0);

		display();

		sceCtrlReadBufferPositive(&pad,1);
		changedButtons=oldButtons ^ pad.Buttons;
		unsigned long time=timeGetTime();
		if(pad.Buttons & PSP_CTRL_LEFT) {
			cameraangle+=0.1f;
			if(cameraangle>M_PI/1.0f) cameraangle=M_PI/1.0f;
			camerax=cosf(cameraangle)*cameradist*sinf(cameraup);
			cameray=cosf(cameraup)*cameradist;
			cameraz=sinf(cameraangle)*cameradist*sinf(cameraup);
		}
		if(pad.Buttons & PSP_CTRL_RIGHT) {
			cameraangle-=0.1f;
			if(cameraangle<-M_PI/1.0f) cameraangle=-M_PI/1.0f;
			camerax=cosf(cameraangle)*cameradist*sinf(cameraup);
			cameray=cosf(cameraup)*cameradist;
			cameraz=sinf(cameraangle)*cameradist*sinf(cameraup);
		}
		if(pad.Buttons & PSP_CTRL_UP) {
			cameraup+=0.1f;
			if(cameraup>M_PI/1.0f) cameraup=M_PI/1.0f;
			camerax=cosf(cameraangle)*cameradist*sinf(cameraup);
			cameray=cosf(cameraup)*cameradist;
			cameraz=sinf(cameraangle)*cameradist*sinf(cameraup);
		}
		if(pad.Buttons & PSP_CTRL_DOWN) {
			cameraup-=0.1f;
			if(cameraup<-M_PI/1.0f) cameraup=-M_PI/1.0f;
			camerax=cosf(cameraangle)*cameradist*sinf(cameraup);
			cameray=cosf(cameraup)*cameradist;
			cameraz=sinf(cameraangle)*cameradist*sinf(cameraup);
		}
		if(pad.Buttons & PSP_CTRL_LTRIGGER) {
			cameradist+=0.1f;
			if(cameradist>350) cameradist=350;
			camerax=cosf(cameraangle)*cameradist*sinf(cameraup);
			cameray=cosf(cameraup)*cameradist;
			cameraz=sinf(cameraangle)*cameradist*sinf(cameraup);
		}
		if(pad.Buttons & PSP_CTRL_RTRIGGER) {
			cameradist-=0.1f;
			if(cameradist<1) cameradist=1;
			camerax=cosf(cameraangle)*cameradist*sinf(cameraup);
			cameray=cosf(cameraup)*cameradist;
			cameraz=sinf(cameraangle)*cameradist*sinf(cameraup);
		}
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
		if(changedButtons & PSP_CTRL_LTRIGGER) {
			fishFight.buttonEvent(FF_LTRIGGER,(pad.Buttons&PSP_CTRL_LTRIGGER)!=0,time);
		}
		if(changedButtons & PSP_CTRL_RTRIGGER) {
			fishFight.buttonEvent(FF_RTRIGGER,(pad.Buttons&PSP_CTRL_RTRIGGER)!=0,time);
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
		if(fishFight.mode==GM_QUIT) break;
#ifdef VID_CAPTURE
		{
			unsigned long now=timeGetTime();
			static int shotNo=0;
			char fname[256];
			shotNo++;
			sprintf(fname,"gheroframe%03d.png",shotNo);
			saveImage(fname,(Color *)(0x04000000+(char *)drawBuffer),480,272,512,0);
			frozenTime+=timeGetTime()-now;
		}
#endif
	}

	sceGuTerm();
#endif

	sceKernelExitGame();
	return 0;
}
