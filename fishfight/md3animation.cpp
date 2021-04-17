#include<stdio.h>
#include<string.h>
#include<malloc.h>

#include"textureholder.h"
#include"md3.h"
#include"mesh.h"
#include"md3animation.h"

MD3AnimationFile::MD3AnimationFile()
{
	sex='m';

	int i;
	for(i=0;i<BOTH_MAX;i++) {
		anim[i].firstFrame=1;
		anim[i].numFrames=1;
		anim[i].loopingFrames=0;
		anim[i].framesPerSecond=25;
	}
}

int MD3AnimationFile::load(const char *filename)
{
	FILE *file=fopen(filename,"r");

	if(!file) return -1;	// File not available.

	char buffer[256];
	buffer[255]=0;

	int i=0;
	int offset=0;
	while( fgets(buffer,255,file)) {
		if(buffer[0]>='0' && buffer[0]<='9') {
			// We have an animation specification.  Get 4 numbers.
			int first=1,num=1,looping=0,per=25;
			int rc=sscanf(buffer,"%d%d%d%d",&first,&num,&looping,&per);
			if(rc==4) {	// Valid anim line
				if(i==BOTH_MAX) {
					fclose(file);
					return -3;	// File is too long!
				}
				if(i==LEGS_WALKCR) offset=first-anim[TORSO_GESTURE].firstFrame;
				first-=offset;
				anim[i].firstFrame=first;
				anim[i].numFrames=num;
				anim[i].loopingFrames=looping;
				if(per==0) per=1;
				anim[i].framesPerSecond=per;
				i++;
			}
		} else if(strncmp(buffer,"sex ",4)==0) {
			sex=buffer[4];	// Hopefully one of m, f.
		}
	}
	fclose(file);
	if(i<BOTH_MAX) return -2;	// File is too short

	return 0;
}

MD3Entity *load_animated_md3(const char *pathname)
{
	MD3Entity *e=new MD3Entity();
	if(e->load(pathname)>=0) 
		return 	e;
	delete e;
	return 0;
}

int MD3Entity::load(const char *pathname)
{
	// Slurp in the MD3
	char filename[256];

	strcpy(filename,pathname);
	strcat(filename,"lower.md3");
	MD3File lower;
	lower.load(filename);

	strcpy(filename,pathname);
	strcat(filename,"upper.md3");
	MD3File upper;
	upper.load(filename);

	strcpy(filename,pathname);
	strcat(filename,"animation.cfg");
//	MD3AnimationFile md3anim;
	md3anim.load(filename);

	int i;
	int tagTorso=0;
	int tagWeapon=0;
	Frame *frame;
	for(i=0;i<lower.header->num_tags;i++) {
		if(strcmp(lower.tag[i].name,"tag_torso")==0) {
			tagTorso=i;
		}
	}
	for(i=0;i<upper.header->num_tags;i++) {
		if(strcmp(upper.tag[i].name,"tag_weapon")==0) {
			tagWeapon=i;
		}
	}
	
	for(i=0;i<lower.header->num_frames;i++) {
		std::vector<MD3FileSurface>::iterator p;
		// Now create 1 Frame per animation frame
		frame=new Frame();
		frame->name="lower";
		lowerFrame.push_back(frame);

		for(p=lower.surface.begin();p!=lower.surface.end();p++) {
			load_md3_surface(this,frame,&*p,i);
		}

		// Now grab the first triangle from lower, and use that to attach upper to it.
		Matrix4x4 trans;
		int j;
		for(j=0;j<3;j++) {
			trans.m[j+4*0]=lower.tag[i*lower.header->num_tags+tagTorso].axis[j+3*0];
			trans.m[j+4*1]=lower.tag[i*lower.header->num_tags+tagTorso].axis[j+3*1];
			trans.m[j+4*2]=lower.tag[i*lower.header->num_tags+tagTorso].axis[j+3*2];
			trans.m[j+4*3]=lower.tag[i*lower.header->num_tags+tagTorso].origin[j];
		}
		lowerTransform.push_back(trans);
	}
	for(i=0;i<upper.header->num_frames;i++) {
		std::vector<MD3FileSurface>::iterator p;
		frame=new Frame();
		frame->name="upper";
		upperFrame.push_back(frame);

		for(p=upper.surface.begin();p!=upper.surface.end();p++) {
			load_md3_surface(this,frame,&*p,i);
		}

		Matrix4x4 trans;
		int j;
		for(j=0;j<3;j++) {
			trans.m[j+4*0]=upper.tag[i*upper.header->num_tags+tagWeapon].axis[j+3*0];
			trans.m[j+4*1]=upper.tag[i*upper.header->num_tags+tagWeapon].axis[j+3*1];
			trans.m[j+4*2]=upper.tag[i*upper.header->num_tags+tagWeapon].axis[j+3*2];
			trans.m[j+4*3]=upper.tag[i*upper.header->num_tags+tagWeapon].origin[j];
		}
		upperTransform.push_back(trans);
	}
	return 0;
}

void load_md3_surface(TextureHolder *entity,Frame *frame,MD3FileSurface *surf,int frameNo)
{
	int i;
	Mesh *mesh=new Mesh();

	frame->mesh.push_back(mesh);
	mesh->polyCount=surf->surface->num_triangles;
	mesh->poly=(Tri *)memalign(16,mesh->polyCount*sizeof(Tri));
	//mesh->poly=(Tri *)malloc(mesh->polyCount*sizeof(Tri));
	mesh->vertCount=surf->surface->num_verts;
	mesh->vert=(Vertex3D *)memalign(16,mesh->vertCount*sizeof(Vertex3D));
	//mesh->vert=(Vertex3D *)malloc(mesh->vertCount*sizeof(Vertex3D));
	if(mesh->vert==0 || mesh->poly==0) {
		printf("load_md3_surface: Problem with memory allocation\n");
		if(mesh->vert) {
			free(mesh->vert);
			mesh->vert=0;
		}
		if(mesh->poly) {
			free(mesh->poly);
			mesh->poly=0;
		}
		return;
	}
	Material mat;
	mat.textureName=surf->shader->name;
	if(entity->findTexture(mat.textureName.c_str())) {
		mat.texture=entity->findTexture(mat.textureName.c_str());
	} else {
		printf("Loading image %s\n",mat.textureName.c_str());
		char buffer[256];
		strcpy(buffer,mat.textureName.c_str());
		char *s=strstr(buffer,".tga");
		if(s) strcpy(s,".PNG");
		Image *image=loadImage(buffer); //mat.textureName.c_str());
		if(image) {
			mat.texture=image;
			entity->addTexture(mat.textureName.c_str(),image);
		}
		if(!image) {
			printf("Couldn't load image %s\n",buffer);
		}
	}
	mesh->mat.push_back(mat);
	// Now fill in the vertices and the polygons
	for(i=0;i<surf->surface->num_triangles;i++) {
		mesh->poly[i].v[0]=surf->triangle[i].indexes[0];
		mesh->poly[i].v[1]=surf->triangle[i].indexes[1];
		mesh->poly[i].v[2]=surf->triangle[i].indexes[2];
	}
	// And verticies.
	if(frameNo>surf->surface->num_frames) {
		printf("Frame number overflow (%d not %d)!\n",frameNo,surf->surface->num_frames);
		frameNo=0;
	}
	for(i=0;i<surf->surface->num_verts;i++) {
		float v[6];
		surf->xyzn[i+surf->surface->num_verts*frameNo].getPos(v);
		/*
		if(!axis) {
		*/
			mesh->vert[i].x=v[0]; //-local_origin[0];
			mesh->vert[i].y=v[1]; //-local_origin[1];
			mesh->vert[i].z=v[2]; //-local_origin[2];
			mesh->vert[i].nx=v[3];
			mesh->vert[i].ny=v[4];
			mesh->vert[i].nz=v[5];
			if(v[0]<mesh->boundingRect[0] || i==0) mesh->boundingRect[0]=v[0];
			if(v[1]<mesh->boundingRect[1] || i==0) mesh->boundingRect[1]=v[1];
			if(v[2]<mesh->boundingRect[2] || i==0) mesh->boundingRect[2]=v[2];
			if(v[0]>mesh->boundingRect[3] || i==0) mesh->boundingRect[3]=v[0];
			if(v[1]>mesh->boundingRect[4] || i==0) mesh->boundingRect[4]=v[1];
			if(v[2]>mesh->boundingRect[5] || i==0) mesh->boundingRect[5]=v[2];
		/*
		} else {
			mesh->vert[i].x=v[0]*axis[0]+v[1]*axis[3]+v[2]*axis[6];
			mesh->vert[i].y=v[0]*axis[1]+v[1]*axis[4]+v[2]*axis[7];
			mesh->vert[i].z=v[0]*axis[2]+v[1]*axis[5]+v[2]*axis[8];
			
			mesh->vert[i].nx=v[3]*axis[0]+v[4]*axis[3]+v[5]*axis[6];
			mesh->vert[i].ny=v[3]*axis[1]+v[4]*axis[4]+v[5]*axis[7];
			mesh->vert[i].nz=v[3]*axis[2]+v[4]*axis[5]+v[5]*axis[8];
			if(origin) {
				mesh->vert[i].x+=origin[0];
				mesh->vert[i].y+=origin[1];
				mesh->vert[i].z+=origin[2];
			}
		}
		*/
		mesh->vert[i].u=surf->texcoord[i].st[0];
		mesh->vert[i].v=surf->texcoord[i].st[1];
//		mesh->vert[i].color=0xffffffff;
	}
	mesh->boundingSphere[0]=(mesh->boundingRect[0]+mesh->boundingRect[3])/2;
	mesh->boundingSphere[1]=(mesh->boundingRect[1]+mesh->boundingRect[4])/2;
	mesh->boundingSphere[2]=(mesh->boundingRect[2]+mesh->boundingRect[5])/2;
	mesh->boundingSphere[3]=sqrtf((mesh->boundingRect[3]-mesh->boundingRect[0])*(mesh->boundingRect[3]-mesh->boundingRect[0])+(mesh->boundingRect[4]-mesh->boundingRect[1])*(mesh->boundingRect[4]-mesh->boundingRect[1])+(mesh->boundingRect[5]-mesh->boundingRect[2])*(mesh->boundingRect[5]-mesh->boundingRect[2]))/2;
}

AnimationTracker::AnimationTracker()
{
	// Defaults
	weight=0.0;
	active=BOTH_DEAD1;	// Default has to be a both one.
	queued=BOTH_DEAD1;	// Usually this animation doesn't twitch
	lastFrame=0;
	nextFrame=0;
	rate=1000/25;
	startTime=0;
	delay=0;
}

Matrix4x4::Matrix4x4() {
	int j;
	for(j=0;j<15;j++) m[j]=0.0f;
	m[0]=m[5]=m[10]=m[15]=1.0f;
}
