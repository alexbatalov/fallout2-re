#ifndef FALLOUT_GAME_OPTIONS_H_
#define FALLOUT_GAME_OPTIONS_H_

#include <stdbool.h>

#include "plib/db/db.h"

int do_options();
int do_optionsFunc(int initialKeyCode);
int PauseWindow(bool a1);
int init_options_menu();
int save_options(File* stream);
int load_options(File* stream);
void IncGamma();
void DecGamma();

#endif /* FALLOUT_GAME_OPTIONS_H_ */
