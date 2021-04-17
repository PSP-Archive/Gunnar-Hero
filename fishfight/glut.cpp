#ifdef _WIN32
#include<windows.h>
#include<mmsystem.h>	// timeGetTime()
#else
extern long timeGetTime();
extern long GetTickCount();
#endif

//#define SURVIVALHORROR 1

#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include<vector>
#include<map>
#include<string>

#include "glchk.h"
#include "mesh.h"
#include "loadpng.h"
#ifdef SURVIVALHORROR
#include "../survivalhorror/survivalhorror.h"
#else
#include "fishfight.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int viewmode;

static
void reshape (int w, int h)
{
	GLCHK(glViewport(0, 0, w, h));
	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadIdentity());
//	GLCHK(glOrtho(-20, 20, -20, 20, -200, 200));
//	GLCHK(glFrustum(-20.0,20.0,-20.0,20.0,1.5,40.0));
	GLCHK(gluPerspective(45.0,(float)w/h,1.5,10000));
	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());
}


static float delta = 1.0;
#define NTEX 9
GLuint texid[NTEX];
int nextTex;
std::map<std::string,int> texMap;

int lastTex=-1;

void display_mesh(Mesh *mesh)
{
	int i,j;
	if(!mesh) return;
	if(mesh->mat.size()>0 && mesh->mat.begin()->textureName.size()>0) {
		std::string name=mesh->mat.begin()->textureName;
		if(texMap.find(name)!=texMap.end()) {
			int id=texMap[name];
			Image *image=mesh->mat.begin()->texture;
			if(id!=lastTex) {
//				printf("Binding texture for image %s\n",name.c_str());
				lastTex=id;
				GLCHK(glBindTexture(GL_TEXTURE_2D, id));
				GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data));
				GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
				GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			}
		} else {
			return;
		}
	}
//printf("Displaying mesh %08x (%d poly, %d verts)\n",mesh,mesh->polyCount,mesh->vertCount);
	glBegin(GL_TRIANGLES);
		for(i=0;i<mesh->polyCount;i++) {
			for(j=0;j<3;j++) {
				int vt=mesh->poly[i].v[j];
				struct Vertex3D *vert=mesh->vert+vt;
				glColor3f(1, 1, 1);
				glTexCoord2f(vert->u,vert->v);
				glNormal3f(vert->nx,vert->ny,vert->nz);
				glVertex3f(vert->x, vert->y, vert->z);
			}
		}
	GLCHK(glEnd());
}

void display_frame(Frame *frame)
{
	if(!frame) return;
	std::vector<Mesh *>::iterator q;
	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glPushMatrix());
	GLCHK(glMultMatrixf(frame->transform));
//	printf("displaying Frame: %s\n",frame->name.c_str());
//	int i=0;
	for(q=frame->mesh.begin();q!=frame->mesh.end();q++) {
		Mesh *mesh=*q;
		display_mesh(mesh);
		/*
		char buf[16];
		sprintf(buf,"%d.tmsh",i++);
		export_tmsh6((frame->name+buf).c_str(),mesh);
		*/
	}

	GLCHK(glPopMatrix());
}

void upload_textures(TextureHolder *entity)
{
	std::map<std::string,Image *>::iterator p;
	
	if(!entity) return;
	for(p=entity->textureBegin();p!=entity->textureEnd();p++) {
		std::string name=p->first;
		Image *image=p->second;
		if(nextTex<NTEX) {
			GLCHK(glBindTexture(GL_TEXTURE_2D, texid[nextTex]));
			psp_log("uploading texture data for %d\n", texid[nextTex]);
			GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data));
			texMap[name]=texid[nextTex];
			nextTex++;
		}
	}
}

static int freeze=0;
static int step=0;

#ifdef SURVIVALHORROR
SurvivalHorror fishFight;
#else
FishFight fishFight;
#endif

static
void display (void)
{
	int i,j;
	static GLfloat angle;

	// Display the 2d background
	GLCHK(glClear(GL_COLOR_BUFFER_BIT));

#ifdef SURVIVALHORROR
	GLCHK(glClear(GL_DEPTH_BUFFER_BIT));
	std::string bgFilename;
	Image *bg=0;
	Viewpoint *camera=fishFight.getCamera();
	if(camera) {
		bgFilename=camera->bgFilename;
		bg=camera->bg;
		printf("Displaying bg: %s (%08x)\n",bgFilename.c_str(),camera->bg);
//		bgFilename=vp->bgFilename;		
	}
	
#if 0
	if(bgFilename.size()>0) {
		if(texMap.find(bgFilename)!=texMap.end()) {
			int id=texMap[bgFilename];
			Image *image=fishFight.findTexture(bgFilename.c_str());
			//printf("Found Image %08x (%08x is correct)\n",image,bg);
			if(id!=lastTex) {
//				printf("Binding texture for image %s\n",name.c_str());
				lastTex=id;
				GLCHK(glBindTexture(GL_TEXTURE_2D, id));
				GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data));
				GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
				GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			}
			GLCHK(glMatrixMode(GL_MODELVIEW));
			GLCHK(glLoadIdentity());
			GLCHK(glBegin(GL_QUADS));
			(glTexCoord2f(0,0));
			(glVertex3f(-20,-20,0));
			(glTexCoord2f(1,0));
			(glVertex3f(20,-20,0));
			(glTexCoord2f(1,1));
			(glVertex3f(20,20,0));
			(glTexCoord2f(0,1));
			(glVertex3f(-20,20,0));
			GLCHK(glEnd());
		}
	}
#endif
#endif

	// Now 3d time
	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());
#ifdef SURVIVALHORROR
	if(camera) {
			GLCHK(gluLookAt(camera->transform[12],camera->transform[13],camera->transform[14],
			camera->at[0],camera->at[1],camera->at[2],
			0,0,1));
	} else 
#endif
	{
		GLCHK(glScalef(.5f,0.5f,.5f));
		GLCHK(glTranslatef(0.0f, -15.0f, -2.50f));
		GLCHK(glRotatef(-90,1.0f,0.0f,0.0f));
		GLCHK(glRotatef(45*(viewmode-4),0.0f,0.0f,1.0f));
	}
	
	GLCHK(glShadeModel(GL_SMOOTH));

	GLCHK(glClear(GL_DEPTH_BUFFER_BIT));


	GLCHK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));	// Handy pre-lighting
	GLCHK(glEnable(GL_BLEND));
	GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	static int currFrame=0;
#if 1
//printf("Displaying scene\n");	
	if(fishFight.sceneEntity) {
		GLCHK(glPushMatrix());
		GLCHK(glScalef(9.0f,9.0f,9.0f));
		std::vector<Frame *>::iterator p;
		for(p=fishFight.sceneEntity->frame.begin();p!=fishFight.sceneEntity->frame.end();p++) {
			Frame *f=*p;
			display_frame(f);
		}
		GLCHK(glPopMatrix());
	}
#endif
#ifdef SURVIVALHORROR
	if(fishFight.getCollisionFrame()) {
		Frame *f=fishFight.getCollisionFrame();
		display_frame(f);
	}
#endif
//	display_frame(naru->frame[0]);
	
//printf("Displaying model\n");	
#if 1
	GLCHK(glPushMatrix());
	GLCHK(glMultMatrixf(fishFight.getHero()->getTransform()));
	display_frame(fishFight.getHero()->getCurrLowerFrame());
	GLCHK(glPushMatrix());
	GLCHK(glMultMatrixf(fishFight.getHero()->getCurrLowerTransform()));
	display_frame(fishFight.getHero()->getCurrUpperFrame());
	GLCHK(glPopMatrix());
	GLCHK(glPopMatrix());
#endif

	float *t=fishFight.getHero()->getTransform();
	printf("Hero pos: %.2f,%.2f,%.2f\n",t[12],t[13],t[14]);	
	fishFight.updateLevel(GetTickCount());		// Windowsism

	/*
	for(p=model->frame.begin();p!=model->frame.end();p++) {
		Frame *f=&*p;
		display_frame(f);
	}
	*/
//printf("Done display\n");	
	glutSwapBuffers();
	glutPostRedisplay();
//printf("Exit display\n");	
}

float scale=4.0f;

static
void keydown (unsigned char key, int x, int y)
{
	switch (key) {
	case 'a':
		fishFight.buttonEvent(FF_LEFT,1,timeGetTime());
		break;
	case 's':
		fishFight.buttonEvent(FF_UP,1,timeGetTime());
		break;
	case 'z':
		fishFight.buttonEvent(FF_DOWN,1,timeGetTime());
		break;
	case 'd':
		fishFight.buttonEvent(FF_RIGHT,1,timeGetTime());
		break;
	case '\n':
	case 'w':
		fishFight.buttonEvent(FF_CROSS,1,timeGetTime());
		break;
	case 27:
	case 'x':			/* cross button */
		exit(0);
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		viewmode=key-'0';
		break;
	case '.': freeze=1-freeze; break;
	case ' ':
		freeze=1;
		step=1;
		break;
	default:
		;
	}
}


static
void keyup (unsigned char key, int x, int y)
{
	switch (key) {
	case 'a':
		fishFight.buttonEvent(FF_LEFT,0,timeGetTime());
		break;
	case 's':
		fishFight.buttonEvent(FF_UP,0,timeGetTime());
		break;
	case 'z':
		fishFight.buttonEvent(FF_DOWN,0,timeGetTime());
		break;
	case 'd':
		fishFight.buttonEvent(FF_RIGHT,0,timeGetTime());
		break;
	case 'w':
		fishFight.buttonEvent(FF_CROSS,0,timeGetTime());
		break;
	default:
		;
	}
}

static
void specialdown (int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		fishFight.buttonEvent(FF_LEFT,1,timeGetTime());
		break;
	case GLUT_KEY_UP:
		fishFight.buttonEvent(FF_UP,1,timeGetTime());
		break;
	case GLUT_KEY_DOWN:
		fishFight.buttonEvent(FF_DOWN,1,timeGetTime());
		break;
	case GLUT_KEY_RIGHT:
		fishFight.buttonEvent(FF_RIGHT,1,timeGetTime());
		break;
	default:
		;
	}
}


static
void specialup (int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		fishFight.buttonEvent(FF_LEFT,0,timeGetTime());
		break;
	case GLUT_KEY_UP:
		fishFight.buttonEvent(FF_UP,0,timeGetTime());
		break;
	case GLUT_KEY_DOWN:
		fishFight.buttonEvent(FF_DOWN,0,timeGetTime());
		break;
	case GLUT_KEY_RIGHT:
		fishFight.buttonEvent(FF_RIGHT,0,timeGetTime());
		break;
	default:
		;
	}
}

static
void joystick (unsigned int buttonMask, int x, int y, int z)
{
	GLCHK(glClearColor(x * 1.0f/2000.0f + 0.5f, y * 1.0f/2000.0f + 0.5f, 1.0f, 1.0f));
}


int main(int argc, char* argv[])
{
printf("Started.\n");
	fishFight.newGame();
	fishFight.load(1);
printf("New game initialized\n");
	glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
        glutInitWindowSize(480, 272);
	glutCreateWindow( __FILE__ );
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
    glutSpecialFunc(specialdown);
    glutSpecialUpFunc(specialup);
	glutJoystickFunc(joystick, 0);
printf("Generating textures\n");
	GLCHK(glGenTextures(NTEX, texid));	// Hard coded.
#ifndef SURVIVALHORROR
	upload_textures(fishFight.getSceneEntity());
#else
	upload_textures(&fishFight);
#endif
	upload_textures(fishFight.getHero()->getTextures());
	if(fishFight.npc.size()>0) {
		Actor *npc=*fishFight.npc.begin();
		upload_textures(npc->getTextures());
	}
	
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
printf("Started.\n");

	GLCHK(glFrontFace(GL_CW));
	GLCHK(glEnable(GL_DEPTH_TEST));
	GLCHK(glEnable(GL_CULL_FACE));
	GLCHK(glEnable(GL_TEXTURE_2D));

	glutMainLoop();
	return 0;
}

