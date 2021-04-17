#ifndef MESH_H
#define MESH_H
/*
MD3 file format data structures
based on documentation by PhaethonH (phaethon@linux.ucla.edu)
*/
#include<string>
#include<vector>
#include<vector>
#include<map>

#include"loadpng.h"
#ifdef ENABLE_MD3
#include"md3animation.h"
#endif
#include "textureholder.h"

enum { ME_SUCCESS, ME_FILENOTFOUND, ME_BADFILE };
extern int mesherror;

struct Vertex3D {
#ifdef FF_TEXTURE_ENABLE
	float u,v;
#endif
#ifdef FF_VERTEXCOLOR_ENABLE
	unsigned int color;
#endif
	float nx,ny,nz;
	float x,y,z;
};
struct Tri {
	unsigned short int v[3];
};
class Material {
public:
	float difuse[4];	// RGBA
	float power;		// Specular power
	float specular[3];	// Specular base color (RGB)
	float emissive[3];	// RGB
	std::string textureName;	// Filename of texture
	Image *texture;
	// How it is used
	int first;		// In the polygon section
	int count;		// Number of shapes of this material
	Material();
	~Material();
	Material(const Material &obj);
	Material& operator=(const Material &obj);
};
class Mesh {
public:
	int polyCount;
	struct Tri *poly;
	short int *polyMaterial;	// Listed in the mat vector
	int vertCount;
	struct Vertex3D *vert;
	std::vector<Material> mat;
	float boundingSphere[4];	// x,y,z,radius
	float boundingRect[6];	// min x,y,z; max x,y,z
	Mesh();
	~Mesh();
	Mesh(const Mesh &obj);
	Mesh& operator=(const Mesh &obj);
};
class Frame {
public:
	std::string name;		// Optional name
	float transform[16];	// Transform matrix for the meshes
	std::vector<Mesh *> mesh;
	std::vector<Frame *> frame;	// Child frames
	Frame();
	~Frame();
	Frame(const Frame &obj);
	Frame& operator=(const Frame &obj);
};

class Entity : public TextureHolder {
public:
	std::vector<Frame *> frame;		// Array of static meshes
	//std::map<std::string,Image *> texture; in base class
	Entity();
	~Entity();
#if 1
		/**
		 * Switches a texture for a different one.
		 * 
		 * \param name the filename of the original texture.
		 * \param newImage the new image to use.
		 * \returns the old texture.
		 */
		virtual Image * swapTexture(const char *name,Image *newImage);
	protected:
		/**
		 * Goes through all of a frame, and does sub-frames and sub-meshes.
		 * 
		 * \param frame the frame to search through
		 * \param filename the original image name
		 * \param newImage the replacement image
		 * 
		 * \seealso swapTexture
		 */
		void swapTexture(Frame *frame, const char *filename,Image *newImage);
#endif
};

Entity *load_xof(const char *filename);
Entity *load_md3(const char *filename);
void free_entity(Entity *scene);
#ifdef _DEBUG
int export_tmsh6(const char *filename,struct Mesh *mesh);
#endif
#endif // MESH_H
