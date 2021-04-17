#ifdef __cplusplus
extern "C" {
#endif

/** playMJPEG is a function which sets up the graphics mode, and plays a 
 * MJPEG.  A MJPEG is really a series of JPEG files concatinated together 
 * in one big file.  The images should be 240x136 in size, and the result is
 * upscaled to 480x272.
 * \param filename the relative or absolute path to the MJPEG file.
 * \param fps how many frames per second to display (up to 60).  Supported values are: 60, 30, 20, 15, 12, 10, 9, 6, 4, 3, 2, 1.  Other values will be rounded.
 * \returns -2 on success or -1 if the file is not found.
 */
int playMJPEG(const char *filename,int fps);

#ifdef __cplusplus
}
#endif

