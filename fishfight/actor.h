// Class automatically generated by Dev-C++ New Class wizard

#ifndef ACTOR_H
#define ACTOR_H

#include "md3animation.h" // inheriting class's header file

enum {
	WALK_IDLE,	// Waiting patiently
	WALK_FD,	// Walking forward (plus turning if needed)
	WALK_BK,	// Walking backwards
	WALK_TURN,	// Turning in place
	RUN_FD,		// Running.
	SWIM_FD,	// Treading water
};

/**
 * The field of non-player characters.  These characters implement
 * a particular behaviour based upon the triggers in the level.
 */
class Actor
{
	public:
		// class constructor
		Actor();
		// class destructor
		~Actor();
		void setAnimation(int value,int now);	// TORSO only upper, LEGS only lower, otherwise affects both and is not queued.
		void setEntity(MD3Entity * x); // sets the value of entity
		MD3Entity * getEntity(); // returns the value of entity
		/**
		 * @returns the lower frame for display.
		 */
		Frame *getCurrLowerFrame();
		/**
		 * @returns the upper frame for display.
		 */
		Frame *getCurrUpperFrame();
		/**
		 * @returns 4x4 matrix with the legs to torso linkage
		 */
		float *getCurrLowerTransform();
		/**
		 * @returns 4x4 matrix with the torso to weapon linkage.
		 */
		float * getCurrUpperTransform();
		/**
		 * This is a percentage health remaining.  This is updated by this class
		 * and displayed by the HUD.
		 */
		int health;
		/// Facing direction in radians.
		float angle;
		/// Turning rate/second
		float turnSpeed;
		/// How fast this character walks/second.
		float walkSpeed;
		/**
		* Updates the animation frame, and then applies the movement 
		*/
		void updateAnimation(int now);
		/**
		 * Current movement mode.  
		 * @param mode The valid values are:
		 * WALK_IDLE, WALK_FD, WALK_BK, WALK_TURN, RUN_FD, SWIM_FD.
		 */
		void setMovement(int mode);
		/**
		 * The turning direction.  
		 * @param dir Value -1 left, 0 for straight, 1 for right.
		 */
		void setTurn(int dir);
		/// Set the position of the actor.
		void setXYZ(float x,float y,float z);
		/// Get the transform matrix for display purposes
		float *getTransform();
		/**
		 * Get movement mode
		 * 
		 * @returns movement mode, like WALK_IDLE, WALK_FD, WALK_BK, etc.
		 */
		int getMovement();
		void setAttack(int x); // sets the value of attackMode
		int getAttack(); // returns the value of attackMode
		/// 4x4 matrix of where the actor is located
		Matrix4x4 pos;
		/**
		 * This retreives the textures used by the actor.
		 * 
		 * @returns TextureHolder that contains the list of textures.
		 */
		TextureHolder * getTextures();
	private:
		/**
		* Current movement mode.  The valid values are:
		* WALK_IDLE, WALK_FD, WALK_BK, WALK_TURN, RUN_FD, SWIM_FD
		*/
		int movementMode;
		/**
		* Current turning mode: 0=straight, -1=left, 1=right
		*/
		int turnMode;
		/**
		 * This points to the character that should appear.  There can 
		 * independent users of a single MD3Entity.
		 */
		MD3Entity * entity;
private:
	void setTracker(AnimationTracker &tracker,int value,int startTime);
	void updateTracker(AnimationTracker &tracker,int elapsed);
	AnimationTracker upperTracker,lowerTracker;		
		/**
		 * Attack mode is for dealing damage.
		 */
		int attackMode;
};

#endif // ACTOR_H
