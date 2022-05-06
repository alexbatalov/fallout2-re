#ifndef SELFRUN_H
#define SELFRUN_H

extern int dword_51C8D8;

int selfrun_get_list(char*** fileListPtr, int* fileListLengthPtr);
int selfrun_free_list(char*** fileListPtr);
void selfrun_playback_callback();

#endif /* SELFRUN_H */
