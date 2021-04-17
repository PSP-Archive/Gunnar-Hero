#ifndef OVERLAY_H
#define OVERLAY_H
/// Overlay for the game, to move more game logic into the portable part of the game.
#include <string>

class Image;

class Overlay
{
public:
	/// see the OVER_ defines above
	bool subimage;	// true means use the ox,oy x ow,oh
	/// the horizonal position of the left in pixels
	int x;
	/// the vertical position of the top in pixels
	int y;
	/// position of the subimage
	int ox;
	/// position of the subimage
	int oy;
	/// position of the subimage
	int ow;
	/// position of the subimage
	int oh;
	/// the image to display
	Image *image;
	Overlay();
};

class FastFont
{
public:
	Image *texture;
	int height;
	struct Glyph {
		bool active;
		int x,y,w,h;
		int kern_left,kern_right,kern_bottom;
	} glyph[256];
	FastFont();
	int load(const char *filename);
};

class Message
{
public:
	Message();
	Message(FastFont *f,int posx,int posy,const char *msg);
	Message(FastFont *f,int posx,int posy,std::string &msg);
	Message(int i,FastFont *f,int posx,int posy,const char *msg);
	void set(FastFont *f,int posx,int posy,const char *msg);
	enum Alignment { ALIGN_LEFT, ALIGN_CENTRE, ALIGN_RIGHT } align;
	FastFont *font;
	/// the identifer of the message.
	int id;
	int x,y;
	std::string message;
};

#endif
