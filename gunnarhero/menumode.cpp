#include "gunnarhero.h"
#include "mp3player.h"

static char *kmapNames[]={
	"None",
	"Orange",
	"Green",
	"Red",
	"Blue",
	"Yellow",
	"Strum",
	"Pause",
	"Screen shot",
};

MenuMode::MenuMode()
{
	type=MMT_NONE;
	parent=0;
	bg=0;
}

MenuMode::MenuMode(Displayable *p)
{
	type=MMT_NONE;
	parent=p;
	bg=0;
}

/// Update the state of any animations, etc.
void MenuMode::update(unsigned long milliseconds)
{
	
	
}

/** Controls the activation mode of the menu.  Deactivating 
 * should remove itself from the message and overlay lists
 * for the display
 */
void MenuMode::activate(bool active)
{
	/// overlay pointer
	//printf("=%d\n",parent->overlay.size());
	parent->overlay.erase(parent->overlay.begin(),parent->overlay.end());
	parent->message.erase(parent->message.begin(),parent->message.end());
	if(active==false) {
		current=message.end();
		return;
	} else {
		// Add in the approprate ones for us.
		std::deque<Overlay>::iterator p;
		
		for(p=overlay.begin();p!=overlay.end();p++) {
			Overlay *over=&*p;
			//printf("Added overlay %08x\n",over);
			parent->overlay.push_back(over);
		}
		std::deque<Message>::iterator q;
		
		for(q=message.begin();q!=message.end();q++) {
			Message *msg=&*q;
			//printf("Added message %08x\n",msg);
			parent->message.push_back(msg);
		}
		current=message.begin();

		for(q=staticMessage.begin();q!=staticMessage.end();q++) {
			Message *msg=&*q;
			//printf("Added message %08x\n",msg);
			parent->message.push_back(msg);
		}
	}
}

static char *menuName[]={
	"None","Main Menu", "Credits","High Score",
	"Controls","Pause Menu", "Settings", "Round Summary", "New Hight Score",
	"Select Song" };
	

class MenuItem {
public:
	char *label;
	bool selectable;
};

static MenuItem menumain[]={
	{"Select Song",true},
	{"Settings",true},
	{"High Scores",true},
	{"Credits",true},
	{"Exit Game",true}};

static MenuItem menucontrols[]={
	{"Triangle: ",true},
	{"Square: ",true},
	{"Circle: ",true},
	{"Cross: ",true},
	{"Left: ",true},
	{"Up: ",true},
	{"Right: ",true},
	{"Down: ",true},
	{"L-trigger: ",true},
	{"R-trigger: ",true},
	{"Select: ",true},
	{"Start: ",true},
	{"Hold: ",true},
	{"Default",true},
	{"Return to Menu",true} };

static MenuItem menuhighscore[]={
	{"",false},
	{"",false},
	{"",false},
	{"",false},
	{"",false},
	{"",false},
	{"More",true},
	{"Return to Menu",true}};
static MenuItem menupause[]={
	{"Continue",true},
	{"Settings",true},
	{"Exit Song",true},
	{"Return to Menu",true} };
static MenuItem menusettings[]={
	{"Controls",true},
	{"Sound effects: ",true},
	{"Music: ",true},
	{"Return to menu",true} };
static MenuItem menusummary[]={
	{"Title: ",false},
//	{"Hits: ",false},
//	{"Misses: ",false},
//	{"Bad notes: ",false},
	{"Score: ",false},
	{"",false},
	{"Continue",true}};
static MenuItem menunewhigh[]={ 
	{"New high score",false},
	{"Name: ",false},
	{"O to advance, [] to erase",false},
	{ "Triangle to save name",false} };
static char *newhighgrid="ABCDEFGHIJKLMNOPQRSTUVWXYZ!_* 0123456789?.";
static MenuItem menucredits[]={ 
	{"A teamsushi.org production",false},
	{"Programming: hardhat",false},
	{"Models: meyitzo, hardhat, PSPdemon",false},
	{"Characters and default skin: Wilhel1812",false},
	{"Sound effects: meanrabbit.com",false},
	{"Music/Levels: the people of freetar.net",false},
	{"Testing: everyone at pspsmm.com, wally, zion, ...",false},
	{"Return to menu",true}};
static MenuItem menuselectsong[]={
	{"Title: ",false},
	{"Artist: ",false},
	{"Difficulty: ",false},
	{"Next song",true},
	{"Select song",true} };
static MenuItem menuerror[]={
	{"An error has occured.",false},
	{"",false},
	{"",false},
	{"Sorry for the inconvenience.",false},
	{"Quit Game",true} };

void MenuMode::setMenuModeType(enum MenuModeType menuModeType)
{
	activate(false);
	if(overlay.size()>0) overlay.erase(overlay.begin(),overlay.end());
	if(message.size()>0) message.erase(message.begin(),message.end());
	if(staticMessage.size()>0) staticMessage.erase(staticMessage.begin(),staticMessage.end());

	type=menuModeType;
	if(menuModeType==MMT_NONE) {
		parent->setMode(GM_PLAYING);
		return;
	}

	int items=0;
	MenuItem *list=0;
	switch(menuModeType) {
	case MMT_MAIN:
		list=menumain;
		items=sizeof(menumain)/sizeof(MenuItem);
		break;
	case MMT_HIGHSCORE:
		list=menuhighscore;
		items=sizeof(menuhighscore)/sizeof(MenuItem);
		break;
	case MMT_CONTROLS:
		list=menucontrols;
		items=sizeof(menucontrols)/sizeof(MenuItem);
		break;
	case MMT_PAUSE:
		list=menupause;
		items=sizeof(menupause)/sizeof(MenuItem);
		break;
	case MMT_SETTINGS:
		list=menusettings;
		items=sizeof(menusettings)/sizeof(MenuItem);
		break;
	case MMT_SUMMARY:
		list=menusummary;
		items=sizeof(menusummary)/sizeof(MenuItem);
		break;
	case MMT_NEWHIGH:
		list=menunewhigh;
		items=sizeof(menunewhigh)/sizeof(MenuItem);
		break;
	case MMT_CREDITS:
		list=menucredits;
		items=sizeof(menucredits)/sizeof(MenuItem);
		break;
	case MMT_SELECTSONG:
		list=menuselectsong;
		items=sizeof(menuselectsong)/sizeof(MenuItem);
		break;
	case MMT_ERROR:
		list=menuerror;
		items=sizeof(menuerror)/sizeof(MenuItem);
		break;
	case MMT_NONE:
		// Actually can't happen.
		return;
		break;
	}

	int top=45;
	int left=130;
	int fontHeight=20;
	int columns=(items*fontHeight+199)/200;
//	printf("Columns: %d\n",columns);
	int columnWidth=150;
	if(columns==0) columns=1;
	top+=(200-items*fontHeight/columns)/2;
	if(type==MMT_NEWHIGH) {
		top=40;
	}
	// Now populate the items in the list:
	int index=0;
	unsigned int i;
	for(i=0;i<(unsigned int)items;i++) {
		//printf("Item %d (%d)\n",i,index);
		//printf("Got: %s\n",list[i].label);
		if(list[i].selectable) {
			message.push_back(Message(index++,&parent->gray10,
				left+columnWidth*(i/((items+1)/columns)),
				top+fontHeight*(i%((items+1)/columns)),list[i].label));
			if(type==MMT_CONTROLS && index<=FF_MAX) {
				message.back().message+=kmapNames[parent->keymap[index-1]];
			}
			if(type==MMT_SETTINGS && index==2) {
				message.back().message=!parent->muteSfx?"Sound Effects: On":"Sound Effects: Off";
			} else if(type==MMT_SETTINGS && index==3) {
				message.back().message=!parent->muteMusic?"Music: On":"Music: Off";
			}
		} else {
			staticMessage.push_back(Message(&parent->gray10,
				left+columnWidth*(i/((items+1)/columns)),
				top+fontHeight*(i%((items+1)/columns)),list[i].label));
		}
	}
	if(type==MMT_NEWHIGH) {
		for(i=0;i<strlen(newhighgrid);i++) {
			char buf[2]=" ";
			buf[0]=newhighgrid[i];
			message.push_back(Message(index++,&parent->gray10,left+(i/6)*25,
				top+(staticMessage.size())*fontHeight+fontHeight*(i%6),list[i].label));
		}
	}
	if(type==MMT_SELECTSONG) {
		std::deque<Message>::iterator p;
		for(i=0,p=staticMessage.begin();p!=staticMessage.end();i++,p++) {
			if(i>2) break;
			p->message=menuselectsong[i].label;
			switch(i) {
			case 0:
				p->message+=parent->getActiveSong()->title;
				break;
			case 1:
				p->message+=parent->getActiveSong()->artist;
				break;
			case 2:
				p->message+=parent->getActiveSong()->difficulty;
				break;
			}
		}
	}
	if(type==MMT_SUMMARY) {
		std::deque<Message>::iterator p;
		for(i=0,p=staticMessage.begin();p!=staticMessage.end();i++,p++) {
			if(i>1) break;
			p->message=menusummary[i].label;
			switch(i) {
			case 0:
				p->message+=parent->getActiveSong()->title;
				break;
			case 1:
				{
				char buf[64];
				sprintf(buf,"%06d",parent->getScore());
				p->message+=buf;
				}
				break;
			}
		}
	}
	if(type==MMT_ERROR) {
		std::deque<Message>::iterator p;
		for(i=0,p=staticMessage.begin();p!=staticMessage.end();i++,p++) {
			if(i==1) p->message=error1;
			if(i==2) p->message=error2;
		}
	}

	if(message.size()>0) {
		current=message.begin();
		current->font=&parent->blue10;
	}
	if(!bg) bg=loadImage("data/menubg.png");
	if(bg) {
		overlay.push_back(Overlay());
		overlay.back().image=bg;
		//printf("Size of overlays: %d\n",overlay.size());
	}
	staticMessage.push_back(Message(&parent->gray16,150,36,menuName[(int)type]));
		
	activate(true);
}

/**
 * Receives a button event.
 * 
 * @param id the button id of the button (0=triangle, etc).
 * @param down true if the button is pressed, and false if the button was released.
 */
void MenuMode::buttonEvent(int id,int down,int milliseconds)
{
	if(down) return;
	if(message.size()==0) return;	// All is lost?
	switch(id) {
	case FF_UP:
		if(current!=message.end()) {
			current->font=&parent->gray10;
		}
		if(current==message.begin()) {
			current=message.end();
		}
		current--;
		current->font=&parent->blue10;
		playSound(S_MOVEARROW);
		break;
	case FF_DOWN:
		if(current!=message.end()) {
			current->font=&parent->gray10;
			current++;
		}
		if(current==message.end()) {
			current=message.begin();
		}
		current->font=&parent->blue10;
		playSound(S_MOVEARROW);
		break;
	case FF_TRIANGLE:
		// Quit this menu.
		switch(type) {
		case MMT_NONE:
			break;
		case MMT_NEWHIGH:
			parent->saveHighScore(username.c_str());
			// fall through
		case MMT_MAIN:
		case MMT_SELECTSONG:
		case MMT_CREDITS:
		case MMT_HIGHSCORE:
			setMenuModeType(MMT_MAIN);
			break;
		case MMT_PAUSE:
			setMenuModeType(MMT_NONE);
			// resume game.
			break;
		case MMT_CONTROLS:
			setMenuModeType(MMT_SETTINGS);
			break;
		case MMT_SETTINGS:
			if(!parent->pauseMenu) {
				setMenuModeType(MMT_MAIN);
			} else {
				setMenuModeType(MMT_PAUSE);
			}
			break;
		case MMT_SUMMARY:
			if(parent->getScore()>parent->getHighScore() ) {
				//setMenuModeType(MMT_NEWHIGH);
				setMenuModeType(MMT_MAIN);
			} else{
				setMenuModeType(MMT_MAIN);
			}
			break;
		}
		break;
	case FF_LEFT:
		if(type==MMT_NEWHIGH) {
			playSound(S_MOVEARROW);
			if(current!=message.end()) {
				current->font=&parent->gray10;
			} else {
				current=message.begin();
			}
			// skip back 6 items
			int i;
			for(i=0;i<6;i++) {
				if(current!=message.begin()) {
					current--;
				} else {
					current=message.end();
					current--;
				}
			}
			current->font=&parent->blue10;
		}		
		break;
	case FF_RIGHT:
		if(type==MMT_NEWHIGH) {
			playSound(S_MOVEARROW);
			if(current!=message.end()) {
				current->font=&parent->gray10;
			} else {
				current=message.begin();
			}
			// skip forward 6 items.
			int i;
			for(i=0;i<6;i++) {
				if(current!=message.end()) {
					current++;
				} else {
					current=message.begin();
				}
			}
			current->font=&parent->blue10;
			break;
		}
		// fall through
	case FF_CROSS:
	case FF_START:
		// Select the item
		switch(type) {
		case MMT_MAIN:
			selectMain();
			break;
		case MMT_CREDITS:
		case MMT_HIGHSCORE:
			setMenuModeType(MMT_MAIN);
			break;
		case MMT_CONTROLS:
			selectControls();
			break;
		case MMT_PAUSE:
			selectPause();
			break;
		case MMT_SETTINGS:
			selectSettings();
			break;
		case MMT_SUMMARY:
			if(parent->getScore()>parent->getHighScore() ) {
				setMenuModeType(MMT_NEWHIGH);
			} else{
				setMenuModeType(MMT_MAIN);
			}
			break;
		case MMT_NEWHIGH:
			selectNewHigh();
			break;
		case MMT_SELECTSONG:
			selectSelectSong();
			break;
		case MMT_NONE:
			// Shouldn't be here.
			break;
		}
		playSound(S_SELECT);
		break;
	}
}


void MenuMode::selectMain()
{
	switch(current->id) {
	case 0:	// Select Song
		setMenuModeType(MMT_SELECTSONG);
		break;
	case 1: // Settings
		setMenuModeType(MMT_SETTINGS);
		break;
	case 2: // High Score
		setMenuModeType(MMT_HIGHSCORE);
		break;
	case 3: // Credits
		setMenuModeType(MMT_CREDITS);
		break;
	case 4: // Exit game
		parent->setMode(GM_QUIT);
		break;
	}
}

void MenuMode::selectPause()
{
	switch(current->id) {
	case 0:	// Continue
		setMenuModeType(MMT_NONE);
		break;
	case 1: // Settings
		setMenuModeType(MMT_SETTINGS);
		break;
	case 2: // Exit Song
		setMenuModeType(MMT_MAIN);
		parent->setPauseMode(false);
		parent->setMode(GM_MODEMENU);
		MP3_Stop();
		MP3_FreeTune();
		break;
	case 3: // Exit game
		parent->setMode(GM_QUIT);
	}
}

void MenuMode::selectControls()
{
	int i;
	switch(current->id) {
	default:
		// key remapping
		parent->keymap[current->id]++;
		if(parent->keymap[current->id]>=KMAP_MAX) {
			parent->keymap[current->id]=0;
		}
		// Update the label.
		current->message=(std::string)menucontrols[current->id].label+
			kmapNames[parent->keymap[current->id]];
		break;
	case 13: // default
		// Make the key map the default.
		for(i=0;i<FF_MAX;i++) {
			parent->keymap[i]=parent->defaultKeymap[i];
		}
		// And update the labels:
		{
			std::deque<Message>::iterator p;
			for(p=message.begin();p!=message.end();p++) {
				if(p->id<FF_MAX) {
					p->message=(std::string)menucontrols[p->id].label+
						kmapNames[parent->keymap[p->id]];
				}
			}
		}
		break;
	case 14: // Return to menu
		FILE *file=fopen("data/kmap","wb");
		if(file) {
			fwrite(parent->keymap,FF_MAX,sizeof(int),file);
			fclose(file);
		}
		setMenuModeType(MMT_SETTINGS);
	}
}

void MenuMode::selectSettings()
{
	switch(current->id) {
	case 0:
		setMenuModeType(MMT_CONTROLS);
		break;
	case 1:
		parent->muteSfx=!parent->muteSfx;
		current->message=!parent->muteSfx?"Sound Effects: On":"Sound Effects: Off";
		break;
	case 2:
		parent->muteMusic=!parent->muteMusic;
		current->message=!parent->muteMusic?"Music: On":"Music: Off";
		break;
	case 3:
		if(parent->pauseMenu) setMenuModeType(MMT_PAUSE);
		else setMenuModeType(MMT_MAIN);
	}
}

/*
 * Handle selection of menu item.
 */
void MenuMode::selectNewHigh()
{
	/* TODO (#1#): Implement MenuMode::selectNewHigh() */
}


// Gets the menu mode.
enum MenuModeType MenuMode::getMenuModeType()
{
	return type;
}

/*
 * Select the song, or the next one.
 */
void MenuMode::selectSelectSong()
{
	std::deque<Message>::iterator p;
	int i;
	
	switch(current->id) {
	case 0:
		// select the next song
		parent->nextSong();
		// Update the display

		for(i=0,p=staticMessage.begin();p!=staticMessage.end();i++,p++) {
			if(i>2) break;
			p->message=menuselectsong[i].label;
			switch(i) {
			case 0:
				p->message+=parent->getActiveSong()->title;
				break;
			case 1:
				p->message+=parent->getActiveSong()->artist;
				break;
			case 2:
				p->message+=parent->getActiveSong()->difficulty;
				break;
			}
		}
		break;
	case 1:
		// select the correct song
		setMenuModeType(MMT_NONE);
		break;
	}
}
