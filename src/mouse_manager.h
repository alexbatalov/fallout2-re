#ifndef MOUSE_MANAGER_H
#define MOUSE_MANAGER_H

extern char* (*off_5195A8)(char*);
extern int (*off_5195AC)();
extern int (*off_5195B0)();

char* _defaultNameMangler(char* a1);
int _defaultRateCallback();
int _defaultTimeCallback();
void _mousemgrSetNameMangler(char* (*func)(char*));
void _initMousemgr();
void _mouseHide();
void _mouseShow();

#endif /* MOUSE_MANAGER_H */
