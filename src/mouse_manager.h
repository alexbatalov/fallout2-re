#ifndef MOUSE_MANAGER_H
#define MOUSE_MANAGER_H

extern char* (*off_5195A8)(char*);
extern int (*off_5195AC)();
extern int (*off_5195B0)();

char* defaultNameMangler(char* a1);
int defaultRateCallback();
int defaultTimeCallback();
void mousemgrSetNameMangler(char* (*func)(char*));
void initMousemgr();
void mouseHide();
void mouseShow();

#endif /* MOUSE_MANAGER_H */
