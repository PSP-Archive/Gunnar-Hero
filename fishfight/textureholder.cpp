#include "textureholder.h"

Image *TextureHolder::findTexture(const char *filename)
{
	if(texture.find(filename)!=texture.end()) {
		return texture.find(filename)->second;
	} else {
		return 0;
	}
}

void TextureHolder::addTexture(const char *filename,Image *image)
{
	texture[filename]=image;
}

Image *TextureHolder::swapTexture(const char *filename,Image *newImage)
{
	Image *oldImage=findTexture(filename);
	printf("swap looking for '%s'.\n",filename);
	if(oldImage) {
		printf("swap found a matching image.\n");
//#if 0
		std::map<std::string,Image *>::iterator p;
		p=texture.find(filename);
		texture.erase(p);
		addTexture(filename,newImage);
//#else
		return 0;
//#endif
	} else {
		std::map<std::string,Image *>::iterator p;
		for(p=texture.begin();p!=texture.end();p++) {
			printf("candidates were: %s\n",p->first.c_str());
		}
	}
	return oldImage;
}
