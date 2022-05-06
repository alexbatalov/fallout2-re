#ifndef NEVS_H
#define NEVS_H

#define NEVS_COUNT (40)

typedef struct Nevs {
    int field_0;
    char field_4[32];
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    void (*field_38)();
} Nevs;

extern Nevs* off_6391C8;

extern int dword_6391CC;

Nevs* nevs_alloc();
void nevs_close();
void nevs_removeprogramreferences(int a1);
void nevs_initonce();
Nevs* nevs_find(const char* a1);
int nevs_addevent(const char* a1, int a2, int a3, int a4);
int nevs_clearevent(const char* a1);
int nevs_signal(const char* a1);
void nevs_update();

#endif /* NEVS_H */
