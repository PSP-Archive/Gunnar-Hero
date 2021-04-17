#include <stdio.h>
#include <math.h>

#include "mesh.h"
#include "piece.h"

#ifdef DMALLOC
#include<dmalloc.h>
#endif

Piece::Piece() {
	int i;
	for(i=0;i<16;i++)
			transform[i]=(i/4)==(i%4)?1:0;
	action=PA_NONE;
	type=PT_NONE;
	model=0;
	transparent=false;
}

void Piece::setModel(Entity *entity,enum PieceType type)
{
	model=entity;
	this->type=type;
}

Entity *Piece::getModel()
{
	return model;
}

float *Piece::getTransform()
{
	return transform;
}

void Piece::setPos(float x,float y,float z)
{
	transform[12]=x;
	transform[13]=y;
	transform[14]=z;
	action=PA_NONE;
}

void Piece::setMoveTo(float x,float y,float z,float seconds)
{
	moveTo[0]=x;
	moveTo[1]=y;
	moveTo[2]=z;
	moveToTimeLeft=(int)(seconds*1000);
	action=PA_MOVETO;
	//printf("Setting move to over: %d\n",moveToTimeLeft);
	if(moveToTimeLeft<=0) action=PA_NONE;
}

void Piece::update(long elapsed)
{
	//printf("Updating piece (%d elapsed)\n",elapsed);
	if(action==PA_MOVETO) {
		float ratio=(float)elapsed/(float)moveToTimeLeft;
		//printf("Ratio = %.3f\n",ratio);
		if(ratio>1.0f) ratio=1.0;
		//printf("Ratio = %.3f\n",ratio);
		transform[12]=transform[12]*(1.0f-ratio)+moveTo[0]*ratio;
		transform[13]=transform[13]*(1.0f-ratio)+moveTo[1]*ratio;
		transform[14]=transform[14]*(1.0f-ratio)+moveTo[2]*ratio;
		//printf("Moving piece to %.2f,%.2f,%.2f\n",moveTo[0],moveTo[1],moveTo[2]);
		//printf("Moving piece (%d of %d) @ %.2f,%.2f,%.2f\n",elapsed,moveToTimeLeft,transform[12],transform[13],transform[14]);

		moveToTimeLeft-=elapsed;
		if(moveToTimeLeft<=0) action=PA_NONE;
	}
}

/*
 * Returns the amount of time left to move.
 */
long Piece::getMoveToTimeLeft()
{
	return moveToTimeLeft;
}

/*
 * Discover if the type of this piece is a button.
 */
bool Piece::isButton()
{
	switch(type) {
	case PT_BUTTONRED:
	case PT_BUTTONBLUE:
	case PT_BUTTONGREEN:
	case PT_BUTTONYELLOW:
	case PT_BUTTONORANGE:
		return true;
	default:
		return false;
	}
	return false;
}

/*
 * Check to see if this function is a sustain bar.
 */
bool Piece::isBar()
{
	switch(type) {
	case PT_BARRED:
	case PT_BARBLUE:
	case PT_BARGREEN:
	case PT_BARYELLOW:
	case PT_BARORANGE:
		return true;
	default:
		return false;
	}
	return false;
}

bool Piece::isHidden()
{
	// 10 seconds is far away.
	if(moveToTimeLeft>10000) return true;
	return false;
}

SpritePiece::SpritePiece()
{
	currentFrame=0;
	animDuration=500;
	animTimeLeft=500;
}

SpritePiece::~SpritePiece()
{
	std::vector<Image *>::iterator p;
	for(p=animFrames.begin();p!=animFrames.end();p++) {
		Image *img=*p;
		delete img;			// Not sure if this is a good idea, but...
	}
	animFrames.erase(animFrames.begin(),animFrames.end());
}

void SpritePiece::update(long elapsed)
{
	if(animTimeLeft>elapsed) {
		animTimeLeft-=elapsed;
		return;
	}
	if(elapsed>animDuration)
		animTimeLeft=animDuration;
	else
		animTimeLeft+=animDuration-elapsed;
	if(animFrames.size()<2) return;
	currentFrame++;
	if(currentFrame>=animFrames.size()) currentFrame=0;
	Image *img=animFrames[currentFrame];
	model->swapTexture(animFilename.c_str(),img);
}

Camera::Camera()
{
	keypoint=0;
	from[0]=0;
	from[1]=0;
	from[2]=0;
	to[0]=0;
	to[1]=0;
	to[2]=0;
	keypointCount=0;
	duration=0;
	pathToTimeLeft=0;
}

Camera::~Camera()
{
	if(keypoint) delete keypoint;
	keypoint=0;
}

void Camera::loadCameraPath(const char *filename)
{
	loadCameraPath(filename,false);	
}


static void orientxyz(float &x,float &y,float &z)
{
	float t1,t2,t3;
	t1=z;
	t2=-x;
	t3=y;
	y=t1;
	z=-t2;
	x=t3;
}

void Camera::loadCameraPath(const char *filename,bool orient)
{

	if(keypoint) delete keypoint;
	keypoint=0;
	from[0]=0;
	from[1]=0;
	from[2]=0;
	to[0]=0;
	to[1]=0;
	to[2]=0;
	keypointCount=0;
	duration=0;
	pathToTimeLeft=0;
	FILE *file=fopen(filename,"r");
	if(!file) {
		printf("Couldn't read '%s'\n",filename);
		return;
	}
	printf("Camera object loading from '%s'\n",filename);
	char line[257];
	float origin[3]={0,0,0};
	int id=0;
	enum AseMode { ASE_START, ASE_CAMERAFROM, ASE_CAMERATO, ASE_LINE } mode=ASE_START;
	std::string name;
	int framespeed=1;
	int frames=0;
	while(fgets(line,255,file)) {
		if(strstr(line,"*NODE_NAME")) {
			char *s=strstr(line, "\"");
			if(s) {
				s++;
				char *e=strstr(s,"\"");
				if(e) e[0]=0;
				name=s;
				if(name.compare("Camera01")==0) {
					//printf("Camera from\n");
					mode=ASE_CAMERAFROM;
				} else if(name.compare("Camera01.Target")==0) {
					//printf("Camera to\n");
					mode=ASE_CAMERATO;
				} else if(name.compare("Line01")==0) {
					//printf("Line\n");
					mode=ASE_LINE;
				} else {
					//printf("Skipping Camera '%s'\n",name.c_str());
					mode=ASE_START;
				}
			}
		} else if(strstr(line,"*TM_POS")) {
			char *s=strstr(line,"*TM_POS ");
			if(s) {
				int res=0;
				s+=8;
				//printf("pos says '%s'\n",s);
				if(mode==ASE_CAMERAFROM) {
					res=sscanf(s,"%f%f%f",&from[0],&from[1],&from[2]);
					if(orient) {
						// rotatexyz(-m_pi/2,m_pi);
						orientxyz(from[0],from[1],from[2]);
					}
					//printf("from[%d]: %.2f,%.2f,%.2f\n",res,from[0],from[1],from[2]);
				} else if(mode==ASE_CAMERATO) {
					res=sscanf(s,"%f%f%f",&to[0],&to[1],&to[2]);
					if(orient) {
						// rotatexyz(-m_pi/2,m_pi);
						orientxyz(to[0],to[1],to[2]);
					}
					//printf("to[%d]: %.2f,%.2f,%.2f\n",res,to[0],to[1],to[2]);
				} else if(mode==ASE_LINE) {
					res=sscanf(s,"%f%f%f",&origin[0],&origin[1],&origin[2]);
					//printf("line[%d]: %.2f,%.2f,%.2f\n",res,origin[0],origin[1],origin[2]);
				}
			}
		} else if(strstr(line,"*SCENE_LASTFRAME")) {
			char *s=strstr(line,"*SCENE_LASTFRAME");
			if(s) {
				// skip whitespace and command name.
				while ((*s>'9' || *s<'0') && *s!='.' && *s!=0) s++;
				sscanf(s,"%d",&frames);
				//printf("last frame: %d\n",frames);
			}
		} else if(strstr(line,"*SCENE_FRAMESPEED")) {
			char *s=strstr(line,"*SCENE_FRAMESPEED");
			if(s) {
				// skip whitespace and command name.
				while ((*s>'9' || *s<'0') && *s!='.' && *s!=0) s++;
				sscanf(s,"%d",&framespeed);
				//printf("frame speed: %d\n",framespeed);
			}
		} else if(strstr(line,"*SHAPE_VERTEXCOUNT")) {
			char *s=strstr(line,"*SHAPE_VERTEXCOUNT");
			if(s) {
				s+=19;
				int count;
				sscanf(s,"%d",&count);
				keypointCount=count;
				keypoint=new float[count*3];
				int i;
				for(i=0;i<count*3;i++) {
					keypoint[i]=0;
				}
				//printf("keyframe count: %d\n",count);
			}
		} else if(strstr(line,"*SHAPE_VERTEX_")) {
			char *s=strstr(line,"*SHAPE_VERTEX_");
			if(s) {
				// skip whitespace and command name.
				while ((*s>'9' || *s<'0') && *s!='.' && *s!=0) s++;
				int num=0;
				float x=0,y=0,z=0;
				int res=sscanf(s,"%d%f%f%f",&num,&x,&y,&z);
				if(num<keypointCount && num>=0 && res==4) {
					//printf("Got [%d]: %.2f,%.2f,%.2f\n",num,x,y,z);
					//x+=origin[0];
					//y+=origin[1];
					//z+=origin[2];
					if(orient) {
						// rotatexyz(-m_pi/2,m_pi);
						orientxyz(x,y,z);
					}
					keypoint[num*3+0]=x;
					keypoint[num*3+1]=y;
					keypoint[num*3+2]=z;
				} else {
					printf("Bad data [%d]: %.2f,%.2f,%.2f (from '%s')\n",num,x,y,z,s);
				}
			}
		}
		
	}
	fclose(file);
	duration=1000*frames/framespeed;
	pathToTimeLeft=duration;
	printf("Camera object loaded ('%s')\n",filename);
}

void Camera::update(unsigned long elapsed)
{
	if(elapsed<=0) return;
	if((long)elapsed>pathToTimeLeft) {
		pathToTimeLeft=0;
	} else {
		pathToTimeLeft-=elapsed;
	}
	if(keypointCount<1) return;
	if(duration<1) return;
	long segment=keypointCount*(duration-pathToTimeLeft)/duration;
	if(segment>=keypointCount-1) return;
	if(segment<0) segment=0;
	// Now look up the relevant points.
	class Pt {
	public:
		float pos[3];
		Pt() { }
		inline Pt(float *p) { pos[0]=p[0]; pos[1]=p[1]; pos[2]=p[2]; }
		inline Pt(float x,float y,float z) { pos[0]=x; pos[1]=y; pos[2]=z; }
		inline Pt operator+(Pt p) {
			Pt res(pos[0]+p.pos[0],pos[1]+p.pos[1],pos[2]+p.pos[2]);
			return res;
		}
		inline Pt operator-(Pt p) {
			Pt res(pos[0]-p.pos[0],pos[1]-p.pos[1],pos[2]-p.pos[2]);
			return res;
		}
		inline Pt operator*(Pt p) {
			Pt res(pos[0]*p.pos[0],pos[1]*p.pos[1],pos[2]*p.pos[2]);
			return res;
		}
		inline Pt operator*(float scalar) {
			Pt res(pos[0]*scalar,pos[1]*scalar,pos[2]*scalar);
			return res;
		}
		inline Pt& operator=(float *p) {
			pos[0]=p[0];
			pos[1]=p[1];
			pos[2]=p[2];
			return *this;			
		}
		inline Pt& operator=(Pt p) {
			pos[0]=p.pos[0];
			pos[1]=p.pos[1];
			pos[2]=p.pos[2];
			return *this;			
		}
		inline float dist() {
			return sqrtf(pos[0]*pos[0]+pos[1]*pos[1]+pos[2]*pos[2]);
		}
	};
#if 0
	Pt pt[4];
	pt[0]=Pt(keypoint+segment*3);
	pt[3]=Pt(keypoint+segment*3+3);
	if(segment>0) {
		Pt delta3N=pt[3]-Pt(keypoint+segment*3-3);

		pt[1]=pt[0]+(delta3N)*(1.0f/3.0f);
	} else {
		pt[1]=pt[0]+(pt[3]-pt[0])*(1.0f/3.0f);		
	}
	if(segment<keypointCount-1) {
		Pt delta40=Pt(keypoint+segment*3+6)-pt[0];
		
		pt[2]=pt[3]+(delta40)*(-1.0f/3.0f);
	} else {
		pt[2]=pt[3]+(pt[3]-pt[0])*(-1.0f/3.0f);
	}
	// Now do the formula
	float t=((duration-pathToTimeLeft)%(duration/keypointCount))/(float)(duration/keypointCount);
	float t1=1.0f-t;
	Pt p=pt[0]*(t1*t1*t1)+pt[1]*(t*t1*t1)+pt[2]*(t*t*t1)+pt[3]*(t*t*t);
#else
	float past=duration-pathToTimeLeft;
	float range=(float)duration/keypointCount;
	float seg=floor(past/range);
	float t=past/range-seg;
	Pt ptstart(keypoint+segment*3);
	Pt ptend(keypoint+segment*3+3);
	Pt p=ptstart+(ptend-ptstart)*t;
#endif
	from[0]=p.pos[0];
	from[1]=p.pos[1];
	from[2]=p.pos[2];
	//printf("From seg=%d (t=%.2f)-> %.2f,%.2f,%.2f\n",segment,t,from[0],from[1],from[2]);
}

void Camera::doPath()
{
	pathToTimeLeft=duration;
}

/*
 * Checks to see if the camera path is still active, or if it is at the end of the path.
 * 
 * \returns true if the camera is still active.
 */
bool Camera::cameraActive()
{
	return pathToTimeLeft>0;
}
