#ifndef MD3ANIMATION_H
#define MD3ANIMATION_H
#include <vector>
#include <map>
#include <string>
#include "textureholder.h"

class Image;
class MD3FileSurface;
class Frame;

enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,
	TORSO_ATTACK,
	TORSO_ATTACK2,	// No weapon
	TORSO_DROP,
	TORSO_RAISE,
	TORSO_STAND,
	TORSO_STAND2,	// No weapon

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,
	LEGS_JUMP,
	LEGS_LAND,
	LEGS_JUMPB,	// With looping part
	LEGS_LANDB,
	LEGS_IDLE,
	LEGS_IDLECR,
	LEGS_TURN,

	BOTH_MAX
};

class MD3Animation {
public:
	int firstFrame;
	int numFrames;
	int loopingFrames;
	int framesPerSecond;
};

class MD3AnimationFile {
public:
	char sex;	// Handy for deciding what sound profile to use.
	MD3Animation anim[BOTH_MAX];
	MD3AnimationFile();
	int load(const char *filename);
};

class AnimationTracker {
public:
	int active;	// What animation we're playing right now
	int queued;	// What animation to play next -- same as active for looping
	int lastFrame;	// What frame we showed last, handy for interpolation.
	int nextFrame;	// What to show
	float weight;	// Between 0 and 1, where 1 means 100% nextFrame
	int startTime;	// To calculate what frame we're at since the last loop began
	int delay;	// Milliseconds to go until next frame
	int rate;	// Number of miliseconds per frame.
	AnimationTracker();	// Constructor
};
class Matrix4x4 {
public:
	float m[16];
	Matrix4x4();	// To identity
};

class MD3Entity : public TextureHolder {
public:
	std::vector<Frame *> upperFrame;
	std::vector<Frame *> lowerFrame;
	std::vector<Matrix4x4> lowerTransform;	// Connect body to legs
	std::vector<Matrix4x4> upperTransform;	// Weapon to body
	MD3AnimationFile md3anim;

	// Methods
	int load(const char *filename);
};
MD3Entity *load_animated_md3(const char *pathname);
void load_md3_surface(TextureHolder *entity,Frame *frame,MD3FileSurface *surf,int frameNo);

#endif
