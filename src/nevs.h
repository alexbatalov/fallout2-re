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

Nevs* sub_488340();
void sub_4883AC();
void sub_4883D4(int a1);
void sub_488418();
Nevs* sub_48846C(const char* a1);
int sub_4884C8(const char* a1, int a2, int a3, int a4);
int sub_48859C(const char* a1);
int sub_48862C(const char* a1);
void sub_4886AC();

#endif /* NEVS_H */
