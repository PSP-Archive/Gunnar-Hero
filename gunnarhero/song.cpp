/** Song implementation of the song class.
*/
#include "tinyxml.h"
#include "song.h"

/*
 * Parse in the level file, and load it in.. 
 * 
 * \param level the level to load up.
 */
void Song::parse(const char *fname)
{
#if 0
	char path[256];
	char filename[256];
	char line[256];
	FILE *file;
	sprintf(path,"song%d/",level);
	strcpy(filename,path);
	strcat(filename,"score.txt");
	
	// Clear out the stuff that is in the level file.
	backgroundFilename="";
	buttonsFilename="";
	gridFilename="";
	title="Untitled";
	author="Anonymous";
	difficulty="Unknown";
	secondsPerBeat=1.0;
	soundMap.clear();
	riffs.clear();
	
	// Open the file
	file=fopen(filename,"r");
	if(file==0) return;	// Failed.
	// Now load the level.
	enum SectionMode { SECTION_NONE,SECTION_INTRO,SECTION_GFX,SECTION_SOUNDS,SECTION_SONG } mode;
	mode=SECTION_NONE;
	while( fgets(line,255,file)!=0) {
		char *name=0,*value=0;
		line[255]=0;
		if(strchr(line,'\n')) strchr(line,'\n')[0]=0;	// Remove trailing CR/LF
		if(strchr(line,'\r')) strchr(line,'\r')[0]=0;	// Remove trailing CR/LF
		if(line[0]!=0 && line[strlen(line)-1]=='\r') line[strlen(line)-1]=0;
		if(strchr(line,'=')) {
			name=line;
            while(name[0]==' ' || name[0]=='\t') name++;	// Remove leading spaces
			value=strchr(line,'=')+1;
		}
		if(strchr(line,':')) {
			 if(strcmp(line,"intro:")==0) { mode=SECTION_INTRO; continue; }
			 if(strcmp(line,"gfx:")==0) { mode=SECTION_GFX; continue; }
			 if(strcmp(line,"sounds:")==0) { mode=SECTION_SOUNDS; continue; }
			 if(strcmp(line,"song:")==0) { mode=SECTION_SONG; continue; }
		}
		switch(mode) {
		case SECTION_INTRO:
			if(name==0) break;
			value[-1]=0;
            while(value[0]==' ' || value[0]=='\t') value++;	// Remove leading spaces
			if(strcmp(name,"title")==0) title=value;
			if(strcmp(name,"author")==0) author=value;
			if(strcmp(name,"difficulty")==0) difficulty=value;
			if(strcmp(name,"secondsperbeat")==0) secondsPerBeat=atof(value);
			if(secondsPerBeat==0) secondsPerBeat=1.0;
			break;
		case SECTION_GFX:
			if(name==0) break;
			value[-1]=0;
            while(value[0]==' ' || value[0]=='\t') value++;	// Remove leading spaces
			if(strcmp(name,"background")==0) backgroundFilename=(std::string)path+value;
			if(strcmp(name,"buttons")==0) buttonsFilename=(std::string)path+value;
			if(strcmp(name,"grid")==0) gridFilename=(std::string)path+value;
			break;
		case SECTION_SOUNDS:
			if(name==0) break;
			value[-1]=0;
			while(value[0]==' ' || value[0]=='\t') value++;	// Remove leading spaces
			soundMap[name[0]]=(std::string)path+value;	// More error checking?
			break;
		case SECTION_SONG:
			{
				int i,pos;
				pos=0;
				RiffBeat notes;
				notes.time=notes.size()*secondsPerBeat;
				notes.duration=0;
				for(i=0;i<4;i++) {
					while(line[pos]==' ' || line[pos]=='\t') pos++;
					if(line[pos]!='.') {
						notes.column=i;
						riffs.push_back(notes);
					}
					if(line[pos]!=0) pos++;
				}
			}
			break;
		}
	}
	fclose(file);
#endif
}

/*
 * Load in the XML Freetar format.
 * 
 * \param level the enumerated song.
 */
void Song::parsexml(const char *fname)
{
	TiXmlDocument doc;

	title="Untitled";
	artist="Anonymous";
	album="None";
	difficulty="Unknown";
	year="xxxx";
	beatsPerSecond=5.0;
	allowableErrorTime=0.25;
	length=0.0;
	musicFilename="";
	riffs.clear();

	doc.LoadFile(fname);
	// Now walk the file to constuct the level.
	TiXmlHandle handle(&doc);
	TiXmlElement *block;
	TiXmlElement *ele;
	TiXmlNode *node;
	TiXmlNode *text;
	// Parse the header
	block=handle.FirstChild("Song").FirstChild("Properties").ToElement();
	if(block) {
		node=block->FirstChild("Difficulty");
		if(node && (text=node->FirstChild())&& text->ToText()) difficulty=text->Value();
		node=block->FirstChild("Title");
		if(node && (text=node->FirstChild())&& text->ToText()) title=text->Value();
		node=block->FirstChild("Artist");
		if(node && (text=node->FirstChild())&& text->ToText()) artist=text->Value();
		node=block->FirstChild("Album");
		if(node && (text=node->FirstChild()) && text->ToText()) album=text->Value();
		node=block->FirstChild("Year");
		if(node && (text=node->FirstChild()) && text->ToText()) year=text->Value();
		node=block->FirstChild("MusicFileName");
		if(node && (text=node->FirstChild()) && text->ToText()) musicFilename=text->Value();
		node=block->FirstChild("BeatsPerSecond");
		if(node && (text=node->FirstChild()) && text->ToText()) sscanf(text->Value(),"%f",&beatsPerSecond);
		node=block->FirstChild("Length");
		if(node && (text=node->FirstChild()) && text->ToText()) sscanf(text->Value(),"%f",&length);
	}
	block=handle.FirstChild("Song").FirstChild("Data").ToElement();
	if(block) {
		for(ele=block->FirstChildElement("Note"); ele; ele=ele->NextSiblingElement("Note")) {
			RiffBeat rb={0,0,0};
			ele->Attribute("duration",&rb.duration);
			if(ele->Attribute("track",&rb.column) &&
			ele->Attribute("time",&rb.time)) {
				riffs.push_back(rb);
			}
		}
	}
}

#ifdef UNITTEST_SONG
int main(int argc,char *argv[])
{
	Song song;
	char *fname="test.sng";
	if(argc>2) fname=argv[1];
	printf("Parsing song file '%s'\n",fname);
	song.parsexml(fname);
	printf("Complete.\n");
}
#endif
