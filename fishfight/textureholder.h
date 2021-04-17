#ifndef TEXTUREHOLDER_H
#define TEXTUREHOLDER_H
#include <map>
#include <string>

class Image;

class TextureHolder {
public:
	virtual ~TextureHolder() {}
	Image *findTexture(const char *filename);
	void addTexture(const char *filename,Image *image);
	virtual Image *swapTexture(const char *filename,Image *newImage);
	std::map<std::string,Image *>::iterator textureBegin() { return texture.begin(); }
	std::map<std::string,Image *>::iterator textureEnd() { return texture.end(); }
protected:
	std::map<std::string,Image *> texture;
};

#endif // TEXTUREHOLDER_H
