#include <stdio.h>
#include <stdlib.h>

#include "overlay.h"
#include "loadpng.h"

Overlay::Overlay() {
	image=0;
	subimage=false;
	x=0;
	y=0;
}

FastFont::FastFont()
{
	texture=0;
	height=1;
	for(int i=0;i<256;i++) {
		glyph[i].active=false;
	}
}

int FastFont::load(const char *filename)
{
	height=1;
	if(texture) return 1;	// already loaded.
	FILE *file=fopen(filename,"r");
	if(!file) {
		texture=0;
		return 0;
	}
	texture=0;
	char line[256];
	line[255]=0;
	while( fgets(line,255,file) ) {
		if(strncmp(line,"Char=",5)==0) {
			int ch=0;
			int x=0,y=0,w=0,h=0,kern_left=0,kern_right=0,kern_bottom=0;
			// Char="z"; or Char=0020;
			char c=0;
			int res=sscanf(line,"Char=\"%c\"; %d,%d,%d,%d; %d,%d,%d;",&c,&x,&y,&w,&h,&kern_left,&kern_right,&kern_bottom);
			ch=c;
			if(res<5) {
				res=sscanf(line,"Char=%x; %d,%d,%d,%d; %d,%d,%d;",&ch,&x,&y,&w,&h,&kern_left,&kern_right,&kern_bottom);
			}
			if(res>=5) {
				glyph[(unsigned int)ch].active=true;
				glyph[(unsigned int)ch].x=x;
				glyph[(unsigned int)ch].y=y;
				glyph[(unsigned int)ch].w=w;
				glyph[(unsigned int)ch].h=h;
				glyph[(unsigned int)ch].kern_left=kern_left;
				glyph[(unsigned int)ch].kern_right=kern_right;
				glyph[(unsigned int)ch].kern_bottom=kern_bottom;
			}
		} else if(strncmp(line,"Height=",7)==0) {
			height=1;
			sscanf(line,"Height=%d",&height);
		} else if(strncmp(line,"Bitmap=",7)==0) {
			std::string imagePath;
			if(strstr(line,"\n")) strstr(line,"\n")[0]=0;
			if(strstr(line,"\r")) strstr(line,"\r")[0]=0;
			imagePath=line+7;
			texture=loadImage(imagePath.c_str());
			if(!texture) texture=loadImage(((std::string)"data/"+imagePath).c_str());
			if(texture) printf("Loaded %s\n",imagePath.c_str());
			else printf("Could not find '%s'\n",imagePath.c_str());
		}
	}
	if(!texture) return 0;
	return 1;
}

Message::Message()
{
	align=ALIGN_LEFT;
	font=0;
	x=0;
	y=0;
	id=0;
}

Message::Message(FastFont *f,int posx,int posy,const char *msg)
{
	align=ALIGN_LEFT;
	font=f;
	x=posx;
	y=posy;
	message=msg;
	id=0;
}

Message::Message(FastFont *f,int posx,int posy,std::string &msg) 
{
	align=ALIGN_LEFT;
	font=f;
	x=posx;
	y=posy;
	message=msg;
	id=0;
}

Message::Message(int i,FastFont *f,int posx,int posy,const char *msg) 
{
	align=ALIGN_LEFT;
	font=f;
	x=posx;
	y=posy;
	message=msg;
	id=i;
}

void Message::set(FastFont *f,int posx,int posy,const char *msg)
{
	align=ALIGN_LEFT;
	font=f;
	x=posx;
	y=posy;
	message=msg;
}

