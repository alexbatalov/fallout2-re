#ifndef SELFRUN_H
#define SELFRUN_H

extern int dword_51C8D8;

int _selfrun_get_list(char*** fileListPtr, int* fileListLengthPtr);
int _selfrun_free_list(char*** fileListPtr);
void _selfrun_playback_callback();

#endif /* SELFRUN_H */
