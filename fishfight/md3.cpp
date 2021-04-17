//#include<pspdebug.h>

#include<stdio.h>

#include "md3.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//#define printf pspDebugScreenPrintf

MD3File::MD3File()
{
	buffer=0;
	length=0;
	header=0;
}

int MD3File::load(const char *filename)
{
printf("Reading file %s\n",filename);
	FILE *file=fopen(filename,"rb");
	if(!file) {
		printf("Could not find file %s\n",filename);
		return -1;	// File not found
	}
	fseek(file,0,SEEK_END);
	length=ftell(file);
	fseek(file,0,SEEK_SET);
	if(length>0) {
		buffer=(char *)malloc(length);
		printf("Allocated buffer %08x\n",buffer);
		if(buffer) fread(buffer,length,1,file);
		if(!buffer) {
			printf("Memory allocation failed.\n");
			return -6;
		}
	}
	fclose(file);
	// Now walk the tree and build up the list of structures
	header=(MD3Header *)buffer;
	if(header->ident!=('I' | 'D'<<8 | 'P'<<16 | '3'<<24)) {
		return -2;	// Wrong magic
	}
	if(header->version!=15) {
		return -3;	// Unsupported version
	}
	if(header->num_frames>1024 || header->num_frames<1 ||
	header->num_tags>16 || header->num_tags<0 ||
	header->num_surfaces>32 || header->num_surfaces<0) {
		return -4;	// Header out of range
	}
	if(header->ofs_frames<0 || header->ofs_frames+header->num_frames*sizeof(MD3Frame)>length ||
	header->ofs_surfaces<0 || header->ofs_surfaces+header->num_surfaces*sizeof(MD3Surface)>length ||
	header->ofs_tags<0 || header->ofs_tags+header->num_tags*sizeof(MD3Tag)>length) {
		return -5;	// File header corrupt
	}
	// Now set up contained pointers.
	frame=(MD3Frame *)(buffer+header->ofs_frames);
	tag=(MD3Tag *)(buffer+header->ofs_tags);
	int base=header->ofs_surfaces;
	int i;
	for(i=0;i<header->num_surfaces;i++) {
		MD3FileSurface s;
		int rc=s.load(buffer+base,length-base);
		if(rc<0) return rc;
		base+=rc;
		surface.push_back(s);
	}
printf("done loading %s\n",filename);
	return 0;
}

int MD3FileSurface::load(char *buffer,int length)
{
	// Now walk the tree and build up the list of structures
	surface=(MD3Surface *)buffer;
	if(surface->ident!=('I' | 'D'<<8 | 'P'<<16 | '3'<<24)) {
		return -12;	// Wrong magic
	}
	if(surface->num_shaders>256 || surface->num_shaders<0 ||
	surface->num_verts>4096 || surface->num_verts<0 ||
	surface->num_triangles>8192 || surface->num_triangles<0) {
		return -14;	// Header out of range
	}
	if(surface->ofs_triangles<0 || surface->ofs_triangles+surface->num_triangles*sizeof(MD3Triangle)>length ||
	surface->ofs_shaders<0 || surface->ofs_shaders+surface->num_shaders*sizeof(MD3Shader)>length ||
	surface->ofs_st<0 || surface->ofs_st+surface->num_verts*sizeof(MD3TexCoord)>length ||
	surface->ofs_xyznormal<0 || surface->ofs_xyznormal+surface->num_frames*surface->num_verts*sizeof(MD3Vertex)>length) {
		return -15; // Header corrupt
	}
	shader=(MD3Shader *)(buffer+surface->ofs_shaders);
	triangle=(MD3Triangle *)(buffer+surface->ofs_triangles);
	texcoord=(MD3TexCoord *)(buffer+surface->ofs_st);
	xyzn=(MD3Vertex *)(buffer+surface->ofs_xyznormal);

	return surface->ofs_end;
}

MD3File::~MD3File() {
	//printf("Thinking about freeing buffer %08x\n",buffer);
	if(buffer) free(buffer);
	return;
}

MD3FileSurface::MD3FileSurface() {
	surface=0;
}

void MD3File::dump(const char *filename)
{
	FILE *file=fopen(filename,"w");
	if(file) {
		dump(file);
	}
	fclose(file);
}

void MD3File::dump(FILE *file)
{
#ifdef _DEBUG
	if(!header) return;
	fprintf(file,"Name: %s\n,Frames: %d\nSkins: %d\nSurfaces: %d\nTags: %d\n",
	header->name,header->num_frames,header->num_skins,header->num_surfaces,
	header->num_tags);
	
	int i;
	for(i=0;i<header->num_frames;i++) {
		fprintf(file,"Frame %d:\n\t'%s'\n\torigin=(%.4f, %.4f, %.4f)\n"
		"\tmin=(%.4f,%.4f,%.4f)\n"
		"\tmax=(%.4f,%.4f,%.4f)\n"
		"\tradius=%.4f\n",
		i,frame[i].name,
		frame[i].local_origin[0],frame[i].local_origin[1],frame[i].local_origin[2],
		frame[i].min_bounds[0],frame[i].min_bounds[1],frame[i].min_bounds[2],
		frame[i].radius);
	}
	for(i=0;i<header->num_tags*header->num_frames;i++) {
		fprintf(file,"Tag %d:\n\t'%s'\n\torigin=(%.4f,%.4f,%.4f)\n",
		i,tag[i].name,tag[i].origin[0],tag[i].origin[1],tag[i].origin[2]);
		fprintf(file,"\taxis=[%.4f,%.4f,%.4f]\n",
		tag[i].axis[0],tag[i].axis[1],tag[i].axis[2]);
		fprintf(file,"\t     [%.4f,%.4f,%.4f]\n",
		tag[i].axis[3],tag[i].axis[4],tag[i].axis[5]);
		fprintf(file,"\t     [%.4f,%.4f,%.4f]\n",
		tag[i].axis[6],tag[i].axis[7],tag[i].axis[8]);
	}
	std::vector<MD3FileSurface>::iterator p;
	i=0;
	for(p=surface.begin();p!=surface.end();p++) {
		fprintf(file,"Surface %d\n",i++);
		p->dump(file);
	}
#endif
}

void MD3FileSurface::dump(FILE *file)
{
#ifdef _DEBUG
	fprintf(file,"\t'%s'\n\tFrames: %d\n\tShaders: %d\n\tTriangles: %d\n\tVerts: %d\n",
	surface->name,surface->num_frames,surface->num_shaders,
	surface->num_triangles,surface->num_verts);
	
	int i;
	for(i=0;i<surface->num_shaders;i++) {
		fprintf(file,"\t\tShader %d\n\t\t\t'%s'\n\t\t\tindex: %d\n",
		i,shader[i].name,shader[i].shader_index);
	}
	for(i=0;i<surface->num_triangles;i++) {
		fprintf(file,"\t\tTriangle %d: {%d,%d,%d}\n",i,
		triangle[i].indexes[0],
		triangle[i].indexes[1],
		triangle[i].indexes[2]);
	}
	for(i=0;i<surface->num_verts;i++) {
		fprintf(file,"\t\tTexture coord %d: (%.4f,%.4f)\n",
		i,texcoord[i].st[0],texcoord[i].st[1]);
	}
	for(i=0;i<surface->num_frames;i++) {
		int j;
		fprintf(file,"\tFrame %d\n",i);
		for(j=0;j<surface->num_verts;j++) {
			fprintf(file,"\t\t%d: %d,%d,%d,%d\n",j,
			xyzn[i*surface->num_verts+j].x,
			xyzn[i*surface->num_verts+j].y,
			xyzn[i*surface->num_verts+j].z,
			xyzn[i*surface->num_verts+j].normal);
		}
	}
#endif
}

void MD3Vertex::getPos3(float *pos)
{
	pos[0]=x/64.0f;
	pos[1]=y/64.0f;
	pos[2]=z/64.0f;
}

void MD3Vertex::getNormal(float *pos)
{
	float lat=(normal & 255) * (2 * (float)M_PI) /255;
	float lng=((normal >> 8) &255) * (2*(float)M_PI) /255;
	// Maybe should do a table lookup for these values:
	pos[0]=cosf(lat)*sinf(lng);
	pos[1]=sinf(lat)*sinf(lng);
	pos[2]=cosf(lng);
}

void MD3Vertex::getPos(float *pos)
{
	getNormal(pos+3);
	getPos3(pos);
}
