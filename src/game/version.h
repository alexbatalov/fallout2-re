#ifndef FALLOUT_GAME_VERSION_H_
#define FALLOUT_GAME_VERSION_H_

// The size of buffer for version string.
#define VERSION_MAX 32

#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_RELEASE 'R'
#define VERSION_BUILD_TIME "Dec 11 1998 16:54:30"

void getverstr(char* dest);

#endif /* FALLOUT_GAME_VERSION_H_ */
