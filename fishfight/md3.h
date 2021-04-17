#ifndef MD3_H
#define MD3_H
#include<math.h>
#include<vector>

class MD3Header {
public:
	int ident;	// "IDP3"
	int version;	// usually 15.
	char name[64];
	int flags;
	int num_frames;	// number of frame objects, up to 1024
	int num_tags;	// number of tag objects, up to 16
	int num_surfaces;	// number of surface objects, up to 32
	int num_skins;	// not used
	int ofs_frames;	// offset (from start) of the frames objects
	int ofs_tags;	// offset (from start) of the tag objects
	int ofs_surfaces;	// offset (from start) of the surfaces
	int ofs_eof;	// note no offset for skin objects.
};
class MD3Frame {
public:
	float min_bounds[3];	// First corner
	float max_bounds[3];	// Second corner of bound box.
	float local_origin[3];	// Local origin (usually 0,0,0)
	float radius;		// Bounding sphere
	char name[16];
};
class MD3Tag {
public:
	char name[64];		// Name of Tag object.
	float origin[3];	// Coordinates of the tag object.
	float axis[9];		// 3x3 matrix transform of the tag object

};
class MD3Surface {
public:
	int ident;	// Magic number "IDP3"
	char name[64];	// Name of the surface
	int flags;	// Flags
	int num_frames;	// Number of frames (matches header)
	int num_shaders;	// Number of shader object for this surface (max 256)
	int num_verts;	// how many vertexes for this surface (max 4096)
	int num_triangles;	// number of triangles objects for this surface (max 8192)
	int ofs_triangles;	// Offset in surface of the list of trinagles
	int ofs_shaders;	// Offset in surface of the shaders
	int ofs_st;		// Offset in the surface of the st (Texture object s-t verts)
	int ofs_xyznormal;	// Offset in surface of xyz-normals
	int ofs_end;	// Offset of the end of the surface
}; 
class MD3Shader {
public:
	char name[64];	// Pathname of the shader in the PK3 file
	int shader_index;	// index of the shader in the file
};
class MD3Triangle {
public:
	int indexes[3];	// Offsets in the vertex objects for trinagles.
};
class MD3TexCoord {
public:
	float st[2];	// uv texture coordinates
};
class MD3Vertex {
public:
	short x,y,z;	// scaled 1.0/64
	short normal;	// encoded log and latitude, with 0, -32767 for extremes
	// lat = (normal & 255) * (2 * PI) /255
	// long = ((normal >> 8) &255) * (2*PI) /255;
	// nx=cos(lat)*sin(long)
	// ny=sin(lat)*sin(long)
	// nz=cos(long)
	void getPos(float *pos);
	void getPos3(float *pos);
	void getNormal(float *normal);
};
class MD3FileSurface {
public:
	MD3Surface *surface;
	MD3Shader *shader;
	MD3Triangle *triangle;	// num_triangles entries
	MD3TexCoord *texcoord;	// num_verts entries
	MD3Vertex *xyzn;	// num_frames * num_verts emtries

	MD3FileSurface();
	int load(char *buffer,int length);
	void dump(FILE *file);
};
class MD3File {
public:
	MD3Header *header;
	MD3Frame * frame;	// Array of frames (num_frames entries)
	MD3Tag * tag;	// Array of tags (num_tagx * num_frames entries)
	std::vector<MD3FileSurface> surface;
	MD3File();
	~MD3File();
	int load(const char *filename);
	void dump(FILE *file);
	void dump(const char *filename);
protected:
	char *buffer;		// Original imported file
	int length;		// Size of the buffer
};
#endif
