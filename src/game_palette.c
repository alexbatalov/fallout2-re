#include "game_palette.h"

#include "color.h"

// 0x44EBC0
int sub_44EBC0(int a1)
{
    // TODO: Some strange bit arithmetic.
    int v1 = sub_4C72E0(a1);
    int r = (v1 & 0x7C00) >> 10;
    int g = (v1 & 0x3E0) >> 5;
    int b = (v1 & 0x1F);

    int result = g;
    if (r > result) {
        result = r;
    }

    result = result & 0xFF;
    if (result <= b) {
        result = b;
    }

    return result;
}
