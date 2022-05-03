#include "graph_lib.h"

#include "debug.h"
#include "memory.h"

#include <string.h>

// 0x596E90
int* off_596E90;

// 0x596E94
int dword_596E94;

// 0x596E98
int dword_596E98;

// 0x596E9C
int* off_596E9C;

// 0x596EA0
int* off_596EA0;

// 0x596EA4
unsigned char* off_596EA4;

// 0x596EA8
int dword_596EA8;

// 0x596EAC
int dword_596EAC;

// 0x44F250
int graphCompress(unsigned char* a1, unsigned char* a2, int a3)
{
    off_596E90 = NULL;
    off_596E9C = NULL;
    off_596EA0 = NULL;
    off_596EA4 = NULL;

    // NOTE: Original code is slightly different, it uses deep nesting or a
    // bunch of gotos.
    off_596EA0 = internal_malloc(sizeof(*off_596EA0) * 4104);
    off_596E9C = internal_malloc(sizeof(*off_596E9C) * 4376);
    off_596E90 = internal_malloc(sizeof(*off_596E90) * 4104);
    off_596EA4 = internal_malloc(sizeof(*off_596EA4) * 4122);

    if (off_596EA0 == NULL || off_596E9C == NULL || off_596E90 == NULL || off_596EA4 == NULL) {
        debugPrint("\nGRAPHLIB: Error allocating compression buffers!\n");

        if (off_596E90 != NULL) {
            internal_free(off_596E90);
        }

        if (off_596E9C != NULL) {
            internal_free(off_596E9C);
        }
        if (off_596EA0 != NULL) {
            internal_free(off_596EA0);
        }
        if (off_596EA4 != NULL) {
            internal_free(off_596EA4);
        }

        return -1;
    }

    sub_44F5F0();

    memset(off_596EA4, ' ', 4078);

    int count = 0;
    int v30 = 0;
    for (int index = 4078; index < 4096; index++) {
        off_596EA4[index] = *a1++;
        int v8 = v30++;
        if (v8 > a3) {
            break;
        }
        count++;
    }

    dword_596E98 = count;

    for (int index = 4077; index > 4059; index--) {
        sub_44F63C(index);
    }

    sub_44F63C(4078);

    unsigned char v29[32];
    v29[1] = 0;

    int v3 = 4078;
    int v4 = 0;
    int v10 = 0;
    int v36 = 1;
    unsigned char v41 = 1;
    int rc = 0;
    while (count != 0) {
        if (count < dword_596E94) {
            dword_596E94 = count;
        }

        int v11 = v36 + 1;
        if (dword_596E94 > 2) {
            v29[v36 + 1] = dword_596EAC;
            v29[v36 + 2] = ((dword_596E94 - 3) | (dword_596EAC >> 4) & 0xF0);
            v36 = v11 + 1;
        } else {
            dword_596E94 = 1;
            v29[1] |= v41;
            int v13 = v36++;
            v29[v13 + 1] = off_596EA4[v3];
        }

        v41 *= 2;

        if (v41 == 0) {
            v11 = 0;
            if (v36 != 0) {
                for (;;) {
                    v4++;
                    *a2++ = v29[v11 + 1];
                    if (v4 > a3) {
                        rc = -1;
                        break;
                    }

                    v11++;
                    if (v11 >= v36) {
                        break;
                    }
                }

                if (rc == -1) {
                    break;
                }
            }

            dword_596EA8 += v36;
            v29[1] = 0;
            v36 = 1;
            v41 = 1;
        }

        int v16;
        int v38 = dword_596E94;
        for (v16 = 0; v16 < v38; v16++) {
            unsigned char v34 = *a1++;
            int v17 = v30++;
            if (v17 >= a3) {
                break;
            }

            sub_44F7EC(v10);

            unsigned char* v19 = off_596EA4 + v10;
            off_596EA4[v10] = v34;

            if (v10 < 17) {
                v19[4096] = v34;
            }

            v3 = (v3 + 1) & 0xFFF;
            v10 = (v10 + 1) & 0xFFF;
            sub_44F63C(v3);
        }

        for (; v16 < v38; v16++) {
            sub_44F7EC(v10);
            v3 = (v3 + 1) & 0xFFF;
            v10 = (v10 + 1) & 0xFFF;
            if (--count != 0) {
                sub_44F63C(v3);
            }
        }
    }

    if (rc != -1) {
        for (int v23 = 0; v23 < v36; v23++) {
            v4++;
            v10++;
            *a2++ = v29[v23 + 1];
            if (v10 > a3) {
                rc = -1;
                break;
            }
        }

        dword_596EA8 += v36;
    }

    internal_free(off_596EA0);
    internal_free(off_596E9C);
    internal_free(off_596E90);
    internal_free(off_596EA4);

    if (rc == -1) {
        v4 = -1;
    }

    return v4;
}

// 0x44F5F0
void sub_44F5F0()
{
    for (int index = 4097; index < 4353; index++) {
        off_596E9C[index] = 4096;
    }

    for (int index = 0; index < 4096; index++) {
        off_596E90[index] = 4096;
    }
}

// 0x44F63C
void sub_44F63C(int a1)
{
    off_596EA0[a1] = 4096;
    off_596E9C[a1] = 4096;
    dword_596E94 = 0;

    unsigned char* v2 = off_596EA4 + a1;

    int v21 = 4097 + off_596EA4[a1];
    int v5 = 1;
    for (;;) {
        int v6 = v21;
        if (v5 < 0) {
            if (off_596EA0[v6] == 4096) {
                off_596EA0[v6] = a1;
                off_596E90[a1] = v21;
                return;
            }
            v21 = off_596EA0[v6];
        } else {
            if (off_596E9C[v6] == 4096) {
                off_596E9C[v6] = a1;
                off_596E90[a1] = v21;
                return;
            }
            v21 = off_596E9C[v6];
        }

        int v9;
        unsigned char* v10 = v2 + 1;
        int v11 = v21 + 1;
        for (v9 = 1; v9 < 18; v9++) {
            v5 = *v10 - off_596EA4[v11];
            if (v5 != 0) {
                break;
            }
            v10++;
            v11++;
        }

        if (v9 > dword_596E94) {
            dword_596E94 = v9;
            dword_596EAC = v21;
            if (v9 >= 18) {
                break;
            }
        }
    }

    off_596E90[a1] = off_596E90[v21];
    off_596EA0[a1] = off_596EA0[v21];
    off_596E9C[a1] = off_596E9C[v21];

    off_596E90[off_596EA0[v21]] = a1;
    off_596E90[off_596E9C[v21]] = a1;

    if (off_596E9C[off_596E90[v21]] == v21) {
        off_596E9C[off_596E90[v21]] = a1;
    } else {
        off_596EA0[off_596E90[v21]] = a1;
    }

    off_596E90[v21] = 4096;
}

// 0x44F7EC
void sub_44F7EC(int a1)
{
    if (off_596E90[a1] != 4096) {
        int v5;
        if (off_596E9C[a1] == 4096) {
            v5 = off_596EA0[a1];
        } else {
            if (off_596EA0[a1] == 4096) {
                v5 = off_596E9C[a1];
            } else {
                v5 = off_596EA0[a1];

                if (off_596E9C[v5] != 4096) {
                    do {
                        v5 = off_596E9C[v5];
                    } while (off_596E9C[v5] != 4096);

                    off_596E9C[off_596E90[v5]] = off_596EA0[v5];
                    off_596E90[off_596EA0[v5]] = off_596E90[v5];
                    off_596EA0[v5] = off_596EA0[a1];
                    off_596E90[off_596EA0[a1]] = v5;
                }

                off_596E9C[v5] = off_596E9C[a1];
                off_596E90[off_596E9C[a1]] = v5;
            }
        }

        off_596E90[v5] = off_596E90[a1];

        if (off_596E9C[off_596E90[a1]] == a1) {
            off_596E9C[off_596E90[a1]] = v5;
        } else {
            off_596EA0[off_596E90[a1]] = v5;
        }
        off_596E90[a1] = 4096;
    }
}

// 0x44F92C
int graphDecompress(unsigned char* src, unsigned char* dest, int length)
{
    off_596EA4 = internal_malloc(sizeof(*off_596EA4) * 4122);
    if (off_596EA4 == NULL) {
        debugPrint("\nGRAPHLIB: Error allocating decompression buffer!\n");
        return -1;
    }

    int v8 = 4078;
    memset(off_596EA4, ' ', v8);

    int v21 = 0;
    int index = 0;
    while (index < length) {
        v21 >>= 1;
        if ((v21 & 0x100) == 0) {
            v21 = *src++;
            v21 |= 0xFF00;
        }

        if ((v21 & 0x01) == 0) {
            int v10 = *src++;
            int v11 = *src++;

            v10 |= (v11 & 0xF0) << 4;
            v11 &= 0x0F;
            v11 += 2;

            for (int v16 = 0; v16 <= v11; v16++) {
                int v17 = (v10 + v16) & 0xFFF;

                unsigned char ch = off_596EA4[v17];
                off_596EA4[v8] = ch;
                *dest++ = ch;

                v8 = (v8 + 1) & 0xFFF;

                index++;
                if (index >= length) {
                    break;
                }
            }
        } else {
            unsigned char ch = *src++;
            off_596EA4[v8] = ch;
            *dest++ = ch;

            v8 = (v8 + 1) & 0xFFF;

            index++;
        }
    }

    internal_free(off_596EA4);

    return 0;
}
