#ifndef MODEMENU_H
#define MODEMENU_H

// Menu mode menus.
#include "overlay.h"

// Sorry, I don't have time to refactor today. :-)
#define Displayable GunnarHero
class Displayable;
enum MenuModeType { MMT_NONE,MMT_MAIN, MMT_CREDITS,MMT_HIGHSCORE,
	MMT_CONTROLS,MMT_PAUSE, MMT_SETTINGS, MMT_SUMMARY, MMT_NEWHIGH,
	MMT_SELECTSONG };

class MenuMode
{
public:
	MenuMode();
	MenuMode(Displayable *d);
	~MenuMode() { }

	void setDisplayable(Displayable *d) { parent=d; }
	/// Update the state of any animations, etc.
	void update(unsigned long milliseconds);
	/** Controls the activation mode of the menu.  Deactivating 
	 * should remove itself from the message and overlay lists
	 * for the display
	 */
	void activate(bool active);
	/**
	 * Receives a button event.
	 * 
	 * @param id the button id of the button (0=triangle, etc).
	 * @param down true if the button is pressed, and false if the button was released.
	 */
	void buttonEvent(int id,int down,int milliseconds);
	void setMenuModeType(enum MenuModeType menuModeType);
		// Gets the menu mode.
		enum MenuModeType getMenuModeType();
protected:
		// The main menu handler.
		void selectMain();
		// Selects a menu item
		void selectPause();
		// Selects a menu item
		void selectSettings();
		// Selects a menu item
		void selectControls();
		/**
		 * Handle selection of menu item.
		 */
		void selectNewHigh();
	std::string username;
	/// The current type.  Change with setMenuModeType() only.
	enum MenuModeType type;
	/// 2D Graphics overlays.
	std::deque<Overlay> overlay;
	/// Messages that you can cursor to
	std::deque<Message> message;
	/// Messages that aren't selectable
	std::deque<Message> staticMessage;
	/// Images
	Image *bg;
	/// pointer to something in the local object message list	
	std::deque<Message>::iterator current;
	
	Displayable *parent;
		/**
		 * Select the song, or the next one.
		 */
		void selectSelectSong();
};

#endif
