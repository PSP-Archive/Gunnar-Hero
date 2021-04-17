/** A riff beat is the think that is flying towards you.
*/
#include <string>
#include <vector>

class RiffBeat {
public:
	double time;	// In seconds.
	int column;	// 0 to 4 are all valid values.
	double duration; // In seconds.
};

/** song an object to track a song file.
*/
class Song
{
public:
		/** Level file */
		std::vector<RiffBeat> riffs;
		/// Music filename
		std::string musicFilename;
		/// The level's title
		std::string title;
		/// The song's author
		std::string artist;
		/// The song's album
		std::string album;
		/// The song's year
		std::string year;
		/// The difficulty of the level
		std::string difficulty;
		/// Speed
		float beatsPerSecond;
		/**
		 * The amount of time that the player can be off, and still score points.
		 */
		float allowableErrorTime;
		/**
		 * The length in seconds.
		 */
		float length;
		/**
		 * Parse in the level file, and load it in.. 
		 * 
		 * \param fname the level to load up.
		 */
		void parse(const char *fname);
		/**
		 * Load in the XML Freetar format.
		 * 
		 * \param level the enumerated song.
		 */
		void parsexml(const char *fname);
};
