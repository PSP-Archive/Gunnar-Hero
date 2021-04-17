#ifndef GUNNARHERO_H
#define GUNNARHERO_H
#include <string>
#include <deque>
#include <vector>
#include "piece.h"
#include "mesh.h"
#include "song.h"
#include "overlay.h"
#include "menumode.h"

enum {
	FF_TRIANGLE,
	FF_SQUARE,
	FF_CIRCLE,
	FF_CROSS,
	FF_LEFT,
	FF_UP,
	FF_RIGHT,
	FF_DOWN,
	FF_LTRIGGER,
	FF_RTRIGGER,
	FF_SELECT,
	FF_START,
	FF_HOLD,
	FF_MAX		// Last one.
};

enum {
	KMAP_NONE,
	KMAP_ORANGE,
	KMAP_GREEN,
	KMAP_RED,
	KMAP_BLUE,
	KMAP_YELLOW,
	KMAP_STRUM,
	KMAP_PAUSE,
	KMAP_SCREENSHOT,
	KMAP_MAX,
};

enum {
S_INTRO,
S_LEVELSTART,
S_WINNER,
S_COMPUTERWINNER,
S_SELECT,
S_BADMOVE,
S_MOVEARROW,
S_MAX
};

/** To be implemented by game engine. Plays an appropriate sound.
 * \param id the S_ enumatated value.
 */
void playSound(int id);
/** To be implemented by game engine. Saves the screen.
 * \param fname the screen to save
*/
void saveScreenshot(const char *fname);

enum BoardPiece {
	BP_EMPTY,
	BP_GRID,
	BP_BUTTON,
	
	BP_CROSS,
	BP_CIRCLE,
	BP_SQUARE,
	BP_TRIANGLE,
};

enum GameMode {
	GM_TITLE,
	GM_MODEMENU,
	GM_PLAYING,
	GM_QUIT
};

class Entity;
/**
 * This is the main game, which is event/timer driven.
 */
class GunnarHero
{
	public:
		// class constructor
		GunnarHero();
		// class destructor
		~GunnarHero();
		void switchTheme();
		/**
		 * New game is called to initialize the game engine.  It resets
		 * the score, health, level, loads the initial level, and resets 
		 * the NPCs.
		 */
		void newGame();
		/**
		 * Updates the state of all entities in the currently active level.
		 * 
		 * @param milliseconds milliseconds since the game/level/machine started.
		 */
		void updateLevel(unsigned long milliseconds);
		std::deque<Piece *> piece;
		std::deque<Overlay *> overlay;
		std::deque<Message *> message;
		/**
		 * Loads in the level specified, and initializes all things needed.
		 * 
		 * @param level the level number to load (0 based)
		 * @returns 0 on success, negative on error.
		 */
		int load(int level);
		/**
		 * A keyboard event was received.
		 * 
		 * @param ch ASCII character or special keycode.
		 * @param down true if pressed, or false if released.
		 */
		void keyEvent(int ch,int down);
		/**
		 * Receives a button event.
		 * 
		 * @param id the button id of the button (0=triangle, etc).
		 * @param down true if the button is pressed, and false if the button was released.
		 */
		void buttonEvent(int id,int down,int milliseconds);
		/**
		* the analog stick changes once per frame
		*
		* @param lx the x value from 0 to 255
		* @param ly the y value from 0 to 255
		* @param milliseconds the time since whenever.
		*/
		void analogEvent(int lx,int ly,int milliseconds);
		/** if we should be showing the pause menu */
		bool pauseMenu;
		/** Mode of the game engine */
		enum GameMode mode;
		/** arrow column */
		int getActiveItem() { return activeItem; }
		/// The filename for the background image.
		std::string backgroundFilename;
		/// The filename for the buttons texture.
		std::string buttonsFilename;
		/// The filename for the grid texture.
		std::string gridFilename;
		/**
		 * Adds Pieces to the scene description to animate the whole song with a 5 second lead in.
		 * Removes any current Pieces of the type PT_BUTTON or PT_GRID.
		 */
		void animateSong();
		/**
		 * Strum the chord.  Apply points, or play an screech, depending.
		 * 
		 * \param down strum direction.
		 */
		void strum(bool down);
		/** gets the chord key specified. 
		* \param i the index of the chord key we want to find the status of
		*/
		int getChord(int i);
		Camera cameraBoard;
		Camera cameraStage;
		/**
		 * erases all of the overlays currently loaded.
		 * Note: it does not erase the images that they refer to.  Those need to be managed seperately.
		 */
		void eraseOverlay();
		/**
		 * Sets the game mode of the game music.  Pausing and unpausing the music as appropriates.
		 * 
		 * \param mode the new mode to set.  May have side effects 
		 * such as pausing and unpausing the game play.
		 */
		void setMode(enum GameMode mode);
		int defaultKeymap[FF_MAX];
		int keymap[FF_MAX];
		/// Sound effects are disabled.
		bool muteSfx;
		/// Music is disabled.
		bool muteMusic;
		FastFont blue10;
		FastFont blue16;
		FastFont gray10;
		FastFont gray16;
		int getScore();
		int getHighScore();
		/**
		 * Saves the high score in the high score list.
		 * 
		 * \param username
		 */
		void saveHighScore(const char *username);
		/**
		 * Sets the pause mode.  Stops the animation, and pauses any music.
		 * 
		 * \param isPaused whether the mode should now be paused
		 */
		void setPauseMode(bool isPaused);
		/**
		 * Figures out the next song number, and points to it.
		 */
		void nextSong();
		/**
		 * Returns the active song
		 * 
		 * \returns the active song.
		 */
		Song * getActiveSong();
		/**
		 * Sets up the overlays for the game HUD.
		 * Used to recover from pause mode, and for intial setup.
		 */
		void setOverlays();
	protected:
		int highscore;
		MenuMode menuMode;
		Overlay jukeboxOverlay;
		Message msgScore;
		Message msgMult;
		Message msgMp3Time;
		Image imgScorebox;
		/// the song that is active
		Song *activeSong;
		/// this points to the active song.
		std::vector<Song *>::iterator songP;
		/// the list of all imported songs.
		std::vector<Song *> songLibrary;
		// The 5 keys, and their states.
		bool chord[5];
        /** list of the themes. */
        std::vector<std::string> theme;
		/** activeItem is the menu item selected */
		int activeItem;
        /** the active theme index */
        unsigned int themeNo;
		/** rotate the piece around the right way.
		 */
		void orientPiece(Entity *modelPiece);
		/** calculate the board relative position of a piece.
		 * \param column in bp squares
		 * \param row in bp squares
		 * \param x output of calc
		 * \param y output of calc
		 */
		void calcPiecePos(float row,int column,float &x,float &y,float &z);
		/** Model of room */
		Entity *modelRoom;
		/** Model of board */
		Entity *modelBoard;
		/** Model of grid piece */
		Entity *modelGrid;
		/** Model good */
		Entity *modelGood;
		/** Model of button piece */
		Entity *modelButton[5];
		/** Model of keys */
		Entity *modelKeys[5];
		/** Model of a sustain bar */
		Entity *modelBar[5];
		/* the texture image to use when the key is unpressed */
		Image *openKeyImage[5];
		/** the texture image to use when the key is pressed */
		Image *closedKeyImage[5];
		/**
		 * The current displayed score.  This is updated by this class, 
		 * and  displayed in the HUD.
		 */
		int score;
		/**
		 * Last parameter to updateLevel.
		 */
		unsigned long lastTick;
		/** is the song waiting to start? */
		bool songRequested;
		/**
		 * Converts from an enum to a chord column.
		 * 
		 * \param type the type of the piece to decode.
		 * \returns the column number.
		 */
		int typeToChord(enum PieceType type);
	protected:
};

#endif
