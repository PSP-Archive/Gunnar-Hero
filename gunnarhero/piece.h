#ifndef PIECE_H
#define PIECE_H
#include <vector>
#include <string>

class Entity;
class Image;

enum PieceType { PT_NONE, PT_ROOM, PT_BOARD, PT_GRID, PT_KEY,
	PT_BUTTONORANGE,PT_BUTTONGREEN,PT_BUTTONRED,PT_BUTTONBLUE,PT_BUTTONYELLOW,
	PT_BARORANGE,PT_BARGREEN, PT_BARRED, PT_BARBLUE, PT_BARYELLOW };
enum PieceAction { PA_NONE, PA_MOVETO, PA_TIMEOUT };
class Piece
{
public:
	Piece();
	virtual ~Piece() {}
	void setModel(Entity *entity,enum PieceType type);
	Entity *getModel();
	float *getTransform();
	void setPos(float x,float y,float z);
	void setMoveTo(float x,float y,float z,float seconds);
	virtual void update(long elapsed);
	/**
	 * Returns the amount of time left to move.
	 */
	long getMoveToTimeLeft();
		/**
		 * Discover if the type of this piece is a button.
		 */
		bool isButton();

	enum PieceAction action;
	enum PieceType type;
	bool transparent;
		/**
		 * Check to see if this function is a sustain bar.
		 */
		bool isBar();
		bool isHidden();
protected:
	float transform[16];
	float moveTo[3];
	long moveToTimeLeft;
	Entity *model;
};

class SpritePiece : public Piece
{
public:
	SpritePiece();
	virtual ~SpritePiece();
	virtual void update(long elapsed);
	std::vector<Image *> animFrames;
	std::string animFilename;
	unsigned int currentFrame;
	long animDuration;
	long animTimeLeft;
};

class Camera
{
public:
	Camera();
	~Camera();
	/// Where the cam is
	float from[3];
	/// What we are looking at
	float to[3];
	/// Must be >0
	float near;
	/// Must be >near
	float far;
	/// Field of view
	float fov;
	/** takes a ASE file with a camera node, and a line path to follow
	* \param filename the file to load.
	*/
	void loadCameraPath(const char *filename);
	/** takes a ASE file with a camera node, and a line path to follow
	* \param filename the file to load.
	* \param orient do orientation scheme
	*/
	void loadCameraPath(const char *filename,bool orient);
	/** update the next camera location.
	* \param elapsed the amount of time since the last frame
	*/
	void update(unsigned long elapsed);
	/** Play back the path */
	void doPath();
		/**
		 * Checks to see if the camera path is still active, or if it is at the end of the path.
		 * 
		 * \returns true if the camera is still active.
		 */
		bool cameraActive();
protected:
	/// the length of the path in seconds x 1000
	long duration;
	/// the number of keypoints in the array.
	int keypointCount;
	/// triples of keypoints.
	float *keypoint;
	/// where we are on the path. in seconds x 1000 -- 0 means still.
	long pathToTimeLeft;
};

#endif
