//#include<pspdebug.h>

#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<string.h>
#include<math.h>
#include<zlib.h>

#include<stack>

#include"textureholder.h"
#include"mesh.h"
#ifdef SUPPORT_MD3
#include"md3.h"
#include"md3animation.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//#define printf pspDebugScreenPrintf

enum { M_NONE,
	M_FRAME,M_FRAMETRANSFORMMATRIX,
	M_MESH,M_MESHVERT,M_MESHPOLY,M_MESHPOLYVERT,
	M_MESHTEXTURE,M_MESHTEXTUREVERT,M_MESHNORMAL,M_MESHNORMALVERT,
	M_MESHNORMALINDEX,M_MESHNORMALINDEXVERT,
	M_MESHMATERIALLIST,M_MESHMATERIALCOUNT,M_MESHMATERIALINDEX,
	M_MESHMATERIAL,M_MESHMATERIALDIFFUSE,M_MESHMATERIALPOWER,
	M_MESHMATERIALSPECULAR,M_MESHMATERIALEMISSION,M_MESHMATERIALTEXTURE,
	M_TEXTUREFILENAME };

//enum { ME_SUCCESS, ME_FILENOTFOUND, ME_BADFILE };
int mesherror=0;

void free_entity(Entity *scene)
{
	printf("Using delete entity.\n");
	delete scene;		// Really an entity.
	printf("Done delete entity\n");
}

#if SUPPORT_MD3
Entity *load_md3(const char *pathname)
{
	// Slurp in the MD3
	FILE *file;
	char filename[256];

	strcpy(filename,pathname);
	strcat(filename,"head.md3");
	MD3File head;
	head.load(filename);

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
	MD3AnimationFile md3anim;
	md3anim.load(filename);

	Entity *entity=new Entity();
	int i;
	int tagTorso=0;
	Frame *frame;
	float *lowerAxis=0;
	float *lowerOrigin=0;
	for(i=0;i<lower.header->num_tags;i++) {
		if(strcmp(lower.tag[i].name,"tag_torso")==0) {
			tagTorso=i;
		}
	}
	
	for(i=0;i<lower.header->num_frames;i++) {
		std::vector<MD3FileSurface>::iterator p;
		// Now create 1 Frame per animation frame
		frame=new Frame();
		entity->frame.push_back(frame);

		for(p=lower.surface.begin();p!=lower.surface.end();p++) {
			load_md3_surface(entity,frame,&*p,i);
		}
		// Now grab the first triangle from lower, and use that to attach upper to it.
//		md3_linkage(linkOrigin,linkAxis,lowerOrigin,lowerAxis,&*lower.surface.begin(),i);
		int j=i % upper.header->num_frames;

		lowerAxis=lower.tag[i*lower.header->num_tags+tagTorso].axis;
		lowerOrigin=lower.tag[i*lower.header->num_tags+tagTorso].origin;

		if(j<upper.header->num_frames) {
			for(p=upper.surface.begin();p!=upper.surface.end();p++) {
				//load_md3_surface(entity,frame,&*p,j,upper.frame[j].local_origin,lowerOrigin,lowerAxis);
				load_md3_surface(entity,frame,&*p,j);
			}
		}
	}
printf("Parsed file correctly\n");
	return entity;
}
#endif

std::string pathPrefix(const char *filename)
{
	char *path=strdup(filename);
	std::string result;
	char *sb=strrchr(path,'\\');
	char *sf=strrchr(path,'/');
	if(sb) sb[1]=0;
	if(sf) sf[1]=0;
	result=path;
	free(path);
	return result;
}

Entity *load_xof(const char *filename)
{
	//FILE *file;
	gzFile file;
	int depth;
	char buf[256];
	int mode;
	int count=0;
	int nvCount=0;
	int uvCount=0;
	Entity *scene=new Entity();
	Frame *frame=0;
	std::stack<Frame *> frameStack;
	Mesh *mesh=0;
	struct MeshNormal { float nx,ny,nz; } *meshNormal=0;
	int meshNormalCount=0;
	float minx=0,miny=0,minz=0,maxx=0,maxy=0,maxz=0;
	std::map<std::string,std::string> materialMap;
	std::string materialName;

	mesherror=ME_SUCCESS;

	file=gzopen(filename,"r");
	if(!file) {
		printf("Could not open %s\n",filename);
		mesherror=ME_FILENOTFOUND;
		return 0;
	}
	depth=0;
	buf[255]=0;
	mode=M_NONE;
	while( gzgets(file,buf,255)) {
//printf("%s",buf);		
		if( strchr(buf,'{') ) {
			depth++;
			//printf("Depth >> %d\n",depth);

		}
		if( strstr(buf,"Frame ") && strchr(buf,'{')) {
			Frame *f=new Frame();
			char *s=strstr(buf,"Frame ");
			char name[256];
			strcpy(name,s+6);
			s=strchr(name,' ');
			if(s) s[0]=0;
			f->name=name;
			//printf("Frame depth == %d\n",depth);
			//printf("Stack depth in:%d\n",frameStack.size());
			if(depth==1) {
				scene->frame.push_back(f);
				frame=f;
				while(frameStack.empty()==false)
					frameStack.pop();
				frameStack.push(f);
			} else if(frame) {
				if(frameStack.size()<(unsigned)depth) {
					frame->frame.push_back(f);
					frameStack.push(f);
				} else {
					while(frameStack.size()>=(unsigned)depth) 
						frameStack.pop();
					frame=frameStack.top();
					frameStack.push(f);
					frame->frame.push_back(f);
				}
				frame=f;
			}
			//printf("Stack depth out:%d\n",frameStack.size());
			mode=M_FRAME;
		}
		if( strstr(buf,"FrameTransformMatrix ") && strstr(buf,"{")) {
			count=0;
			mode=M_FRAMETRANSFORMMATRIX;
		}
		if( strstr(buf,"Mesh " ) && strstr(buf," {")) {
			mode=M_MESH;
			count=0;
			nvCount=0;
			uvCount=0;
			
			if(!frame) {	// Assert we have a frame
				frame=new Frame;
				scene->frame.push_back(frame);
			}
			mesh=new Mesh();
			frame->mesh.push_back(mesh);
			printf("Set Mesh to %08x\n",mesh);
		}
		if( strstr(buf,"MeshNormals " ) && strstr(buf,"{")) {
			mode=M_MESHNORMAL;
		}
		if( strstr(buf,"MeshTextureCoords " ) && strstr(buf,"{")) {
			mode=M_MESHTEXTURE;
		}
		if( strstr(buf,"Material ") && strstr(buf," {")) {
			char *s=strstr(buf,"Material ");
			char matName[256];
			strcpy(matName,s+9);
			if(strstr(matName," {")) strstr(matName," {")[0]=0;
			materialName=matName;
			printf("Found material %s\n",matName);
		}
		if( strstr(buf,"MeshMaterialList ") && strstr(buf," {")) {
			mode=M_MESHMATERIALLIST;
		}
		if( mode==M_MESHMATERIALLIST && strstr(buf,"{ ") && strstr(buf," }") ) {
			// Grab texture from named material.
			char *s=strstr(buf,"{ ");
			char texName[256];
			strcpy(texName,s+2);
			if(strstr(texName," }")) strstr(texName," }")[0]=0;
//printf("Searching for material '%s'\n",texName);
			// Now look up material name in material name list.
			if(materialMap.find((std::string)texName)!=materialMap.end()) {
printf("Reusing material '%s'\n",texName);

				std::string fname=materialMap.find((std::string)texName)->second;
				Material mat;
				mat.textureName=fname;
#ifdef FF_TEXTURE_ENABLE
				if(scene->findTexture(mat.textureName.c_str())!=0) {
					mat.texture=scene->findTexture(mat.textureName.c_str());
					printf("Found loaded image %s\n",mat.textureName.c_str());
				} else {
					// Here we attempt to load in the texture
					//printf("Loading image '%s'\n",fname);
					Image *image=loadImage(fname.c_str());
					if(!image) image=loadImage((pathPrefix(filename)+fname).c_str());
					if(image) {
						printf("Adding image %08x\n",image);
						scene->addTexture(mat.textureName.c_str(),image);
						mat.texture=image;
					}
				}
#endif
				if(mesh) {
					mesh->mat.push_back(mat);
				}
			}
		}
		if( strstr(buf,"TextureFilename ") && strstr(buf,"{")) {
			mode=M_TEXTUREFILENAME;
		}
		if( mode==M_TEXTUREFILENAME && strstr(buf,"\"") ) {
			char *s=strchr(buf,'"');
			char fname[256]="";
			if(s) {
				s++;
				strcpy(fname,s);
				if((s=strchr(fname,'"'))) {
					s[0]=0;
				}
			}
			mode=M_NONE;
			Material mat;
			mat.textureName=fname;
#ifdef FF_TEXTURE_ENABLE
			if(scene->findTexture(mat.textureName.c_str())!=0) {
				mat.texture=scene->findTexture(mat.textureName.c_str());
				printf("Found loaded image %s\n",mat.textureName.c_str());
			} else {
				// Here we attempt to load in the texture
				//printf("Loading image '%s'\n",fname);
				Image *image=loadImage(fname);
				if(!image) image=loadImage((pathPrefix(filename)+fname).c_str());
				if(image) {
					printf("Adding image %08x\n",image);
					scene->addTexture(mat.textureName.c_str(),image);
					mat.texture=image;
				}
			}
#endif
			if(mesh) {
				mesh->mat.push_back(mat);
			}
			if(materialName.size()>0 && materialMap.find(materialName)==materialMap.end()) {
				printf("Added material mapping: %s to %s\n", materialName.c_str(),fname);

				materialMap[materialName]=fname;
			}
		}
		//if( strstr(buf," Material ") && strstr(buf, "{")) {
		//	mode=M_MESHMATERIAL;
			// Should copy the material name from this line
		//}
		if( strstr(buf,"}") ) {
			depth--;
			//printf("Depth << %d\n",depth);
		}
		if(mode==M_MESH) {
			if(sscanf(buf,"%d",&count) && count>0) {
				mode=M_MESHVERT;
				//printf("Loading in %d vertexes\n",count);
				mesh->vert=(struct Vertex3D *)memalign(16,sizeof(struct Vertex3D)*count);
				//mesh->vert=(struct Vertex3D *)malloc(sizeof(struct Vertex3D)*count);
				memset(mesh->vert,0,sizeof(struct Vertex3D)*count);
			}
		} else if(mode==M_MESHVERT && count>0) {
			float x=0,y=0,z=0;
			int rc;
			rc=sscanf(buf,"%f;%f;%f;",&x,&y,&z);
			if(mesh->vertCount==0) {
				minx=x; miny=y; minz=z;
				maxx=x; maxy=y; maxz=z;
			} else {
				if(x<minx) minx=x;
				if(y<miny) miny=y;
				if(z<minz) minz=z;
				if(x>maxx) minx=x;
				if(y>maxy) maxy=y;
				if(z>maxz) maxz=z;
			}

			mesh->vert[mesh->vertCount].x=x;
			mesh->vert[mesh->vertCount].y=y;
			mesh->vert[mesh->vertCount].z=z;
#ifdef FF_VERTEXCOLOR_ENABLE
			mesh->vert[mesh->vertCount].color=0xffffffff;
#endif

			mesh->vertCount++;
			count--;
			if(count==0) {
				mode=M_MESHPOLY;
				//printf("Vert bound box: ( %.3f,%.3f,%.3f ) - ( %.3f,%.3f,%.3f )\n",minx,miny,minz,maxx,maxy,maxz);
				mesh->boundingRect[0]=minx;
				mesh->boundingRect[1]=miny;
				mesh->boundingRect[2]=minz;
				mesh->boundingRect[3]=maxx;
				mesh->boundingRect[4]=maxy;
				mesh->boundingRect[5]=maxz;
				mesh->boundingSphere[0]=(minx+maxx)/2;
				mesh->boundingSphere[1]=(miny+maxy)/2;
				mesh->boundingSphere[2]=(minz+maxz)/2;
				mesh->boundingSphere[3]=sqrtf((maxx-minx)*(maxx-minx)+(maxy-miny)*(maxy-miny)+(maxz-minz)*(maxz-minz))/2;
			}
		} else if(mode==M_MESHPOLY) {
			if(sscanf(buf,"%d",&count) && count>0) {
				//printf("Loading in %d polys\n",count);
				mode=M_MESHPOLYVERT;
				mesh->poly=(struct Tri *)memalign(16,sizeof(struct Tri)*count*2);
				//mesh->poly=(struct Tri *)malloc(sizeof(struct Tri)*count*2);
				memset(mesh->poly,0,sizeof(struct Tri)*count*2);
			}
		} else if(mode==M_MESHPOLYVERT && count>0) {
			int corners=0,v[4];
			v[0]=v[1]=v[2]=v[3]=0;
			sscanf(buf,"%d",&corners);
			if(corners==4) {
				int count=sscanf(buf,"%d; %d, %d, %d, %d;",&corners,v+0,v+1,v+2,v+3);
				if(count<5) sscanf(buf,"%d; %d; %d; %d; %d;",&corners,v+0,v+1,v+2,v+3);
				mesh->poly[mesh->polyCount].v[0]=v[0];
				mesh->poly[mesh->polyCount].v[1]=v[1];
				mesh->poly[mesh->polyCount++].v[2]=v[2];
				mesh->poly[mesh->polyCount].v[0]=v[0];
				mesh->poly[mesh->polyCount].v[1]=v[2];
				mesh->poly[mesh->polyCount++].v[2]=v[3];
			} else if(corners==3) {
				int count=sscanf(buf,"%d; %d, %d, %d;",&corners,v+0,v+1,v+2);
				if(count<4) sscanf(buf,"%d; %d; %d; %d;",&corners,v+0,v+1,v+2);
				mesh->poly[mesh->polyCount].v[0]=v[0];
				mesh->poly[mesh->polyCount].v[1]=v[1];
				mesh->poly[mesh->polyCount++].v[2]=v[2];
			}
			count--;
			if(count==0) {
				mode=M_NONE;
				printf("Actual poly count: %d\n",mesh->polyCount);
			}
		} else if(mode==M_MESHNORMAL) {
			if(sscanf(buf,"%d",&count) && count>0) {
				mode=M_MESHNORMALVERT;
				printf("Loading in %d vertex normals\n",count);
				if(meshNormal) delete [] meshNormal;
				meshNormal=new MeshNormal[count];
				meshNormalCount=count;
				if(mesh->vertCount<count) {
					printf("Invalid file.  Upperbound of verts set to %d\n",mesh->vertCount);
					mesherror=ME_BADFILE;
					count=mesh->vertCount;
				}
			}
			//printf("nvCount = %d\n",nvCount);
		} else if(mode==M_MESHNORMALVERT && count>0) {
			float x=0,y=0,z=0;
			int rc;
			rc=sscanf(buf,"%f;%f;%f;",&x,&y,&z);
			meshNormal[nvCount].nx=x;
			meshNormal[nvCount].ny=y;
			meshNormal[nvCount].nz=z;
			
			nvCount++;
			count--;
			if(count==0) mode=M_MESHNORMALINDEX;
		} else if(mode==M_MESHNORMALINDEX && meshNormal) {
			sscanf(buf,"%d",&count);
			mode=M_MESHNORMALINDEXVERT;
			if(count>mesh->polyCount) {
				printf("Bad normal vertex count %d -- exceeds polys %d\n",count,mesh->polyCount);
				mesherror=ME_BADFILE;
				count=mesh->polyCount;
			}
		} else if(mode==M_MESHNORMALINDEXVERT && count>0) {
			int v[4];
			int corners=0;
			v[0]=v[1]=v[2]=v[3]=0;
			sscanf(buf,"%d",&corners);
			if(corners==4) {
				int count=sscanf(buf,"%d; %d, %d, %d, %d;",&corners,v+0,v+1,v+2,v+3);
				if(count<5) sscanf(buf,"%d; %d; %d; %d; %d;",&corners,v+0,v+1,v+2,v+3);
			} else if(corners==3) {
				int count=sscanf(buf,"%d; %d, %d, %d;",&corners,v+0,v+1,v+2);
				if(count<4) sscanf(buf,"%d; %d; %d; %d;",&corners,v+0,v+1,v+2);
			}
			int i;
			int idx=mesh->polyCount-count;
			// Doesn't support quads properly.
			for(i=0;i<3;i++) {
				int vt=mesh->poly[idx].v[i];
				mesh->vert[vt].nx=meshNormal[v[i]].nx;
				mesh->vert[vt].ny=meshNormal[v[i]].ny;
				mesh->vert[vt].nz=meshNormal[v[i]].nz;
			}
			count--;
			if(count==0) {
				delete [] meshNormal;
				meshNormal=0;
				meshNormalCount=0;
			}
		} else if(mode==M_MESHTEXTURE) {
			if(sscanf(buf,"%d",&count) && count>0) {
				mode=M_MESHTEXTUREVERT;
				printf("Loading in %d vertex texture coordinates\n",count);
				if(mesh->vertCount<count) {
					printf("Invalid file.  Upperbound of verts set to %d\n",mesh->vertCount);
					mesherror=ME_BADFILE;
					count=mesh->vertCount;
				}
			}
			//printf("uvCount = %d\n",uvCount);
		} else if(mode==M_MESHTEXTUREVERT && count>0) {
#ifdef FF_TEXTURE_ENABLE
			float u=0,v=0;
			int rc;
			rc=sscanf(buf,"%f;%f;",&u,&v);
			mesh->vert[uvCount].u=u;
			mesh->vert[uvCount].v=v;
#endif
			
			uvCount++;
			count--;
			if(count==0) {
				mode=M_NONE;
				//printf("uvCount = %d\n",uvCount);
			}
		} else if(mode==M_FRAMETRANSFORMMATRIX && count<4) {
			float x=0.0f,y=0.0f,z=0.0f,w=0.0f,t[16];
			int i;
			for(i=0;i<16;i++) t[i]=0.0f;
			if(count==0) {
				int num=sscanf(buf,"%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",t+0,t+1,t+2,t+3,t+4,t+5,t+6,t+7,t+8,t+9,t+10,t+11,t+12,t+13,t+14,t+15);
				if(num>15) {
					for(i=0;i<16;i++) {
						frame->transform[i]=t[i];
						//printf("%.2f ",t[i]);
						//if((i%4)==3) printf("\n");
					}
					count=4;
				}
			}
			if(count<4 && sscanf(buf,"%f,%f,%f,%f",&x,&y,&z,&w)>0) {
				frame->transform[count*4]=x;
				frame->transform[count*4+1]=y;
				frame->transform[count*4+2]=z;
				frame->transform[count*4+3]=w;
				count++;
			}
			if(count==4) mode=M_NONE;
		}
	}
	printf("Final parser state: %d\n",mode);
	gzclose(file);
#ifdef _DEBUG
	static int dumpId=1;
	char fbuf[256];
	sprintf(fbuf,"dump%02d.msh6",dumpId++);
	export_tmsh6(fbuf,mesh);
	printf("Dumped %d\n",dumpId-1);
#endif
	
	if(mode!=M_NONE) {
		mesherror=ME_BADFILE;
		return 0;
	}
	return scene;
}

Entity::Entity() {
printf("Constructor entity\n");
}
Entity::~Entity() {
//printf("Freeing entity\n");
	std::map<std::string,Image *>::iterator p;
	for(p=texture.begin();p!=texture.end();p++) {
		Image *image=p->second;
		freeImage(image);
	}
	std::vector<Frame *>::iterator q;
	for(q=frame.begin();q!=frame.end();q++) {
		Frame *f=*q;
		delete f;
	}
}

Mesh::Mesh() {
	polyCount=0;
	poly=0;
	polyMaterial=0;
	vertCount=0;
	vert=0;
	int i;
	for(i=0;i<6;i++) boundingRect[i]=0;
	for(i=0;i<4;i++) boundingSphere[i]=1;
}
Mesh::~Mesh() {
//printf("Freeing mesh\n");
	if(poly) free(poly);
	poly=0;
	if(polyMaterial) free(polyMaterial);
	polyMaterial=0;
	if(vert) free(vert);
	vert=0;
}
Mesh& Mesh::operator =(const Mesh &obj) {
	mat=obj.mat;
	polyMaterial=0;
	poly=0;
	polyCount=obj.polyCount;
	if(polyCount>0) {
		poly=(Tri *)malloc(polyCount*sizeof(Tri));
		memcpy(poly,obj.poly,polyCount*sizeof(Tri));
	}
	vert=0;
	vertCount=obj.vertCount;
	if(vertCount>0) {
		vert=(Vertex3D *)malloc(vertCount*sizeof(Vertex3D));
		memcpy(vert,obj.vert,vertCount*sizeof(Vertex3D));
	}
	return *this;
}
Mesh::Mesh(const Mesh &obj) {
	*this=obj;	// Using the copy constructor
}

Frame::Frame() {
	int i;
	for(i=0;i<16;i++) transform[i]=0.0f;
	// Initialize as identity matrix
	transform[0]=transform[5]=transform[10]=transform[15]=1.0f;
	mesh.reserve(256);
}
Frame::~Frame() {
//printf("Freeing frame\n");
	std::vector<Frame *>::iterator p;
	for(p=frame.begin();p!=frame.end();p++) {
		Frame *f;
		f=*p;
		delete f;
	}
	std::vector<Mesh *>::iterator q;
	for(q=mesh.begin();q!=mesh.end();q++) {
		Mesh *m;
		m=*q;
		delete m;
	}
}
Frame::Frame(const Frame&obj) {
	*this=obj;
}
Frame& Frame::operator =(const Frame&obj) {
	int i;
	mesh=obj.mesh;
	name=obj.name;
	for(i=0;i<15;i++) {
		transform[i]=obj.transform[i];
	}
	// Now copy all of the contained frames into this list
#if 0	
	std::vector<Frame *>::iterator p;
	printf("Thinking of you\n");
	p=obj.frame.begin();
	printf("No, really.\n");
	p++;
	
	for(p=obj.frame.begin();p!=obj.frame.end();p++) {
		Frame *in=*p;
		Frame *f=new Frame();
		*f=*in;
		frame.push_back(f);
	}
#endif
	return *this;
}

Material::Material() {
	difuse[0]=difuse[1]=difuse[2]=difuse[3]=1.0f;
	power=0.80f;
	specular[0]=specular[1]=specular[2]=1.0f;
	emissive[0]=emissive[1]=emissive[2]=0.0f;
	texture=0;
	first=count=0;
}
Material::~Material() {
	texture=0;		// Deallocated elsewhere
}
Material::Material(const Material &obj) {
	*this=obj;
}
Material& Material::operator =(const Material &obj) {
	difuse[0]=obj.difuse[0];
	difuse[1]=obj.difuse[1];
	difuse[2]=obj.difuse[2];
	difuse[3]=obj.difuse[3];
	power=obj.power;
	specular[0]=obj.specular[0];
	specular[1]=obj.specular[1];
	specular[2]=obj.specular[2];
	texture=obj.texture;
	textureName=obj.textureName;
	first=obj.first;
	count=obj.count;

	return *this;
}

#ifdef _DEBUG
int export_tmsh6(const char *filename,struct Mesh *mesh) {
	FILE *file;
	int count;

	/* Dump result */
	file=fopen(filename,"w");
	fprintf(file,"%d\n%d\n",mesh->vertCount,mesh->polyCount);
	for(count=0;count<mesh->vertCount;count++) {
		fprintf(file,"%f %f ",mesh->vert[count].u,mesh->vert[count].v);
		fprintf(file,"%f %f %f ",mesh->vert[count].x,mesh->vert[count].y,mesh->vert[count].z);
		fprintf(file,"%f %f %f\n",mesh->vert[count].nx,mesh->vert[count].ny,mesh->vert[count].nz);
	}
	for(count=0;count<mesh->polyCount;count++) {
		fprintf(file,"%d %d %d\n",mesh->poly[count].v[0],mesh->poly[count].v[1],mesh->poly[count].v[2]);
	}
	if(mesh->mat.size()>0) {
		fprintf(file,"%s\n",mesh->mat.begin()->textureName.c_str());
	}
	printf("Saved sample file %s\n",filename);
	fclose(file);
	return 0;
}	
#endif

#if 1
/*
 * Switches a texture for a different one.
 * 
 * \param name the filename of the original texture.
 * \param newImage the new image to use.
 * \returns the old texture.
 */
Image * Entity::swapTexture(const char *name,Image *newImage)
{
	std::vector<Frame *>::iterator p;
	
	for(p=frame.begin();p!=frame.end();p++) {
		Frame *f=*p;
		swapTexture(f,name,newImage);
	}
	
	return 0;
}

/*
 * Goes through all of a frame, and does sub-frames and sub-meshes.
 * 
 * \param frame the frame to search through
 * \param filename the original image name
 * \param newImage the replacement image
 * 
 * \seealso swapTexture
 */
void Entity::swapTexture(Frame *frame, const char *filename,Image *newImage)
{
	std::vector<Frame *>::iterator p;
	std::vector<Mesh *>::iterator q;
	std::vector<Material>::iterator r;
	
	for(p=frame->frame.begin();p!=frame->frame.end();p++) {
		Frame *f=*p;
		swapTexture(f,filename,newImage);
	}
	for(q=frame->mesh.begin();q!=frame->mesh.end();q++) {
		Mesh *m=*q;
		for(r=m->mat.begin();r!=m->mat.end();r++) {
			Material *mat=&*r;
			if(mat->textureName.compare(filename)==0) {
				//printf("%s texture name\n",mat->textureName.c_str());
				mat->texture=newImage;
			}
		}
	}
}
#endif
