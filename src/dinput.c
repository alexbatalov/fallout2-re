#include <initguid.h>

#include "dinput.h"

// NOTE: There is no such define in DirectX SDK. I've taken it from Wine at
// https://github.com/wine-mirror/wine/blob/master/dlls/dinput/data_formats.c.
#define DIDFT_OPTIONAL 0x80000000

// NOTE: This define is different in the newer DirectX. Check DirectX SDK 3.0
// at https://github.com/masonmc/dxsdk3/blob/master/sdk/inc/dinput.h.
#undef DIDFT_ANYINSTANCE
#define DIDFT_ANYINSTANCE 0x0000FF00

// 0x4FCE90
static const DIOBJECTDATAFORMAT dfDIMouse[] = {
    { &GUID_XAxis, DIMOFS_X, DIDFT_ANYINSTANCE | DIDFT_AXIS, 0 },
    { &GUID_YAxis, DIMOFS_Y, DIDFT_ANYINSTANCE | DIDFT_AXIS, 0 },
    { &GUID_ZAxis, DIMOFS_Z, DIDFT_OPTIONAL | DIDFT_ANYINSTANCE | DIDFT_AXIS, 0 },
    { NULL, DIMOFS_BUTTON0, DIDFT_ANYINSTANCE | DIDFT_BUTTON, 0 },
    { NULL, DIMOFS_BUTTON1, DIDFT_ANYINSTANCE | DIDFT_BUTTON, 0 },
    { NULL, DIMOFS_BUTTON2, DIDFT_OPTIONAL | DIDFT_ANYINSTANCE | DIDFT_BUTTON, 0 },
    { NULL, DIMOFS_BUTTON3, DIDFT_OPTIONAL | DIDFT_ANYINSTANCE | DIDFT_BUTTON, 0 },
};

// 0x4FCF00
const DIDATAFORMAT c_dfDIMouse = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    sizeof(DIMOUSESTATE),
    sizeof(dfDIMouse) / sizeof(dfDIMouse[0]),
    (LPDIOBJECTDATAFORMAT)dfDIMouse
};

// 0x4FCF20
static const DIOBJECTDATAFORMAT dfDIKeyboard[] = {
    { &GUID_Key, 0, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(0), 0 },
    { &GUID_Key, 1, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(1), 0 },
    { &GUID_Key, 2, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(2), 0 },
    { &GUID_Key, 3, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(3), 0 },
    { &GUID_Key, 4, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(4), 0 },
    { &GUID_Key, 5, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(5), 0 },
    { &GUID_Key, 6, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(6), 0 },
    { &GUID_Key, 7, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(7), 0 },
    { &GUID_Key, 8, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(8), 0 },
    { &GUID_Key, 9, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(9), 0 },
    { &GUID_Key, 10, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(10), 0 },
    { &GUID_Key, 11, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(11), 0 },
    { &GUID_Key, 12, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(12), 0 },
    { &GUID_Key, 13, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(13), 0 },
    { &GUID_Key, 14, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(14), 0 },
    { &GUID_Key, 15, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(15), 0 },
    { &GUID_Key, 16, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(16), 0 },
    { &GUID_Key, 17, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(17), 0 },
    { &GUID_Key, 18, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(18), 0 },
    { &GUID_Key, 19, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(19), 0 },
    { &GUID_Key, 20, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(20), 0 },
    { &GUID_Key, 21, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(21), 0 },
    { &GUID_Key, 22, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(22), 0 },
    { &GUID_Key, 23, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(23), 0 },
    { &GUID_Key, 24, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(24), 0 },
    { &GUID_Key, 25, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(25), 0 },
    { &GUID_Key, 26, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(26), 0 },
    { &GUID_Key, 27, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(27), 0 },
    { &GUID_Key, 28, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(28), 0 },
    { &GUID_Key, 29, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(29), 0 },
    { &GUID_Key, 30, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(30), 0 },
    { &GUID_Key, 31, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(31), 0 },
    { &GUID_Key, 32, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(32), 0 },
    { &GUID_Key, 33, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(33), 0 },
    { &GUID_Key, 34, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(34), 0 },
    { &GUID_Key, 35, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(35), 0 },
    { &GUID_Key, 36, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(36), 0 },
    { &GUID_Key, 37, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(37), 0 },
    { &GUID_Key, 38, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(38), 0 },
    { &GUID_Key, 39, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(39), 0 },
    { &GUID_Key, 40, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(40), 0 },
    { &GUID_Key, 41, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(41), 0 },
    { &GUID_Key, 42, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(42), 0 },
    { &GUID_Key, 43, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(43), 0 },
    { &GUID_Key, 44, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(44), 0 },
    { &GUID_Key, 45, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(45), 0 },
    { &GUID_Key, 46, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(46), 0 },
    { &GUID_Key, 47, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(47), 0 },
    { &GUID_Key, 48, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(48), 0 },
    { &GUID_Key, 49, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(49), 0 },
    { &GUID_Key, 50, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(50), 0 },
    { &GUID_Key, 51, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(51), 0 },
    { &GUID_Key, 52, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(52), 0 },
    { &GUID_Key, 53, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(53), 0 },
    { &GUID_Key, 54, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(54), 0 },
    { &GUID_Key, 55, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(55), 0 },
    { &GUID_Key, 56, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(56), 0 },
    { &GUID_Key, 57, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(57), 0 },
    { &GUID_Key, 58, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(58), 0 },
    { &GUID_Key, 59, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(59), 0 },
    { &GUID_Key, 60, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(60), 0 },
    { &GUID_Key, 61, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(61), 0 },
    { &GUID_Key, 62, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(62), 0 },
    { &GUID_Key, 63, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(63), 0 },
    { &GUID_Key, 64, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(64), 0 },
    { &GUID_Key, 65, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(65), 0 },
    { &GUID_Key, 66, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(66), 0 },
    { &GUID_Key, 67, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(67), 0 },
    { &GUID_Key, 68, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(68), 0 },
    { &GUID_Key, 69, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(69), 0 },
    { &GUID_Key, 70, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(70), 0 },
    { &GUID_Key, 71, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(71), 0 },
    { &GUID_Key, 72, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(72), 0 },
    { &GUID_Key, 73, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(73), 0 },
    { &GUID_Key, 74, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(74), 0 },
    { &GUID_Key, 75, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(75), 0 },
    { &GUID_Key, 76, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(76), 0 },
    { &GUID_Key, 77, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(77), 0 },
    { &GUID_Key, 78, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(78), 0 },
    { &GUID_Key, 79, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(79), 0 },
    { &GUID_Key, 80, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(80), 0 },
    { &GUID_Key, 81, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(81), 0 },
    { &GUID_Key, 82, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(82), 0 },
    { &GUID_Key, 83, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(83), 0 },
    { &GUID_Key, 84, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(84), 0 },
    { &GUID_Key, 85, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(85), 0 },
    { &GUID_Key, 86, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(86), 0 },
    { &GUID_Key, 87, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(87), 0 },
    { &GUID_Key, 88, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(88), 0 },
    { &GUID_Key, 89, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(89), 0 },
    { &GUID_Key, 90, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(90), 0 },
    { &GUID_Key, 91, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(91), 0 },
    { &GUID_Key, 92, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(92), 0 },
    { &GUID_Key, 93, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(93), 0 },
    { &GUID_Key, 94, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(94), 0 },
    { &GUID_Key, 95, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(95), 0 },
    { &GUID_Key, 96, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(96), 0 },
    { &GUID_Key, 97, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(97), 0 },
    { &GUID_Key, 98, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(98), 0 },
    { &GUID_Key, 99, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(99), 0 },
    { &GUID_Key, 100, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(100), 0 },
    { &GUID_Key, 101, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(101), 0 },
    { &GUID_Key, 102, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(102), 0 },
    { &GUID_Key, 103, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(103), 0 },
    { &GUID_Key, 104, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(104), 0 },
    { &GUID_Key, 105, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(105), 0 },
    { &GUID_Key, 106, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(106), 0 },
    { &GUID_Key, 107, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(107), 0 },
    { &GUID_Key, 108, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(108), 0 },
    { &GUID_Key, 109, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(109), 0 },
    { &GUID_Key, 110, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(110), 0 },
    { &GUID_Key, 111, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(111), 0 },
    { &GUID_Key, 112, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(112), 0 },
    { &GUID_Key, 113, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(113), 0 },
    { &GUID_Key, 114, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(114), 0 },
    { &GUID_Key, 115, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(115), 0 },
    { &GUID_Key, 116, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(116), 0 },
    { &GUID_Key, 117, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(117), 0 },
    { &GUID_Key, 118, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(118), 0 },
    { &GUID_Key, 119, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(119), 0 },
    { &GUID_Key, 120, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(120), 0 },
    { &GUID_Key, 121, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(121), 0 },
    { &GUID_Key, 122, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(122), 0 },
    { &GUID_Key, 123, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(123), 0 },
    { &GUID_Key, 124, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(124), 0 },
    { &GUID_Key, 125, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(125), 0 },
    { &GUID_Key, 126, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(126), 0 },
    { &GUID_Key, 127, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(127), 0 },
    { &GUID_Key, 128, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(128), 0 },
    { &GUID_Key, 129, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(129), 0 },
    { &GUID_Key, 130, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(130), 0 },
    { &GUID_Key, 131, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(131), 0 },
    { &GUID_Key, 132, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(132), 0 },
    { &GUID_Key, 133, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(133), 0 },
    { &GUID_Key, 134, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(134), 0 },
    { &GUID_Key, 135, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(135), 0 },
    { &GUID_Key, 136, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(136), 0 },
    { &GUID_Key, 137, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(137), 0 },
    { &GUID_Key, 138, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(138), 0 },
    { &GUID_Key, 139, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(139), 0 },
    { &GUID_Key, 140, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(140), 0 },
    { &GUID_Key, 141, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(141), 0 },
    { &GUID_Key, 142, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(142), 0 },
    { &GUID_Key, 143, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(143), 0 },
    { &GUID_Key, 144, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(144), 0 },
    { &GUID_Key, 145, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(145), 0 },
    { &GUID_Key, 146, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(146), 0 },
    { &GUID_Key, 147, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(147), 0 },
    { &GUID_Key, 148, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(148), 0 },
    { &GUID_Key, 149, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(149), 0 },
    { &GUID_Key, 150, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(150), 0 },
    { &GUID_Key, 151, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(151), 0 },
    { &GUID_Key, 152, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(152), 0 },
    { &GUID_Key, 153, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(153), 0 },
    { &GUID_Key, 154, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(154), 0 },
    { &GUID_Key, 155, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(155), 0 },
    { &GUID_Key, 156, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(156), 0 },
    { &GUID_Key, 157, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(157), 0 },
    { &GUID_Key, 158, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(158), 0 },
    { &GUID_Key, 159, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(159), 0 },
    { &GUID_Key, 160, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(160), 0 },
    { &GUID_Key, 161, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(161), 0 },
    { &GUID_Key, 162, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(162), 0 },
    { &GUID_Key, 163, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(163), 0 },
    { &GUID_Key, 164, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(164), 0 },
    { &GUID_Key, 165, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(165), 0 },
    { &GUID_Key, 166, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(166), 0 },
    { &GUID_Key, 167, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(167), 0 },
    { &GUID_Key, 168, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(168), 0 },
    { &GUID_Key, 169, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(169), 0 },
    { &GUID_Key, 170, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(170), 0 },
    { &GUID_Key, 171, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(171), 0 },
    { &GUID_Key, 172, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(172), 0 },
    { &GUID_Key, 173, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(173), 0 },
    { &GUID_Key, 174, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(174), 0 },
    { &GUID_Key, 175, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(175), 0 },
    { &GUID_Key, 176, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(176), 0 },
    { &GUID_Key, 177, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(177), 0 },
    { &GUID_Key, 178, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(178), 0 },
    { &GUID_Key, 179, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(179), 0 },
    { &GUID_Key, 180, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(180), 0 },
    { &GUID_Key, 181, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(181), 0 },
    { &GUID_Key, 182, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(182), 0 },
    { &GUID_Key, 183, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(183), 0 },
    { &GUID_Key, 184, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(184), 0 },
    { &GUID_Key, 185, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(185), 0 },
    { &GUID_Key, 186, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(186), 0 },
    { &GUID_Key, 187, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(187), 0 },
    { &GUID_Key, 188, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(188), 0 },
    { &GUID_Key, 189, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(189), 0 },
    { &GUID_Key, 190, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(190), 0 },
    { &GUID_Key, 191, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(191), 0 },
    { &GUID_Key, 192, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(192), 0 },
    { &GUID_Key, 193, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(193), 0 },
    { &GUID_Key, 194, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(194), 0 },
    { &GUID_Key, 195, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(195), 0 },
    { &GUID_Key, 196, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(196), 0 },
    { &GUID_Key, 197, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(197), 0 },
    { &GUID_Key, 198, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(198), 0 },
    { &GUID_Key, 199, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(199), 0 },
    { &GUID_Key, 200, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(200), 0 },
    { &GUID_Key, 201, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(201), 0 },
    { &GUID_Key, 202, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(202), 0 },
    { &GUID_Key, 203, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(203), 0 },
    { &GUID_Key, 204, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(204), 0 },
    { &GUID_Key, 205, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(205), 0 },
    { &GUID_Key, 206, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(206), 0 },
    { &GUID_Key, 207, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(207), 0 },
    { &GUID_Key, 208, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(208), 0 },
    { &GUID_Key, 209, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(209), 0 },
    { &GUID_Key, 210, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(210), 0 },
    { &GUID_Key, 211, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(211), 0 },
    { &GUID_Key, 212, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(212), 0 },
    { &GUID_Key, 213, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(213), 0 },
    { &GUID_Key, 214, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(214), 0 },
    { &GUID_Key, 215, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(215), 0 },
    { &GUID_Key, 216, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(216), 0 },
    { &GUID_Key, 217, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(217), 0 },
    { &GUID_Key, 218, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(218), 0 },
    { &GUID_Key, 219, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(219), 0 },
    { &GUID_Key, 220, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(220), 0 },
    { &GUID_Key, 221, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(221), 0 },
    { &GUID_Key, 222, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(222), 0 },
    { &GUID_Key, 223, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(223), 0 },
    { &GUID_Key, 224, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(224), 0 },
    { &GUID_Key, 225, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(225), 0 },
    { &GUID_Key, 226, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(226), 0 },
    { &GUID_Key, 227, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(227), 0 },
    { &GUID_Key, 228, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(228), 0 },
    { &GUID_Key, 229, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(229), 0 },
    { &GUID_Key, 230, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(230), 0 },
    { &GUID_Key, 231, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(231), 0 },
    { &GUID_Key, 232, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(232), 0 },
    { &GUID_Key, 233, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(233), 0 },
    { &GUID_Key, 234, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(234), 0 },
    { &GUID_Key, 235, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(235), 0 },
    { &GUID_Key, 236, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(236), 0 },
    { &GUID_Key, 237, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(237), 0 },
    { &GUID_Key, 238, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(238), 0 },
    { &GUID_Key, 239, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(239), 0 },
    { &GUID_Key, 240, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(240), 0 },
    { &GUID_Key, 241, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(241), 0 },
    { &GUID_Key, 242, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(242), 0 },
    { &GUID_Key, 243, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(243), 0 },
    { &GUID_Key, 244, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(244), 0 },
    { &GUID_Key, 245, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(245), 0 },
    { &GUID_Key, 246, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(246), 0 },
    { &GUID_Key, 247, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(247), 0 },
    { &GUID_Key, 248, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(248), 0 },
    { &GUID_Key, 249, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(249), 0 },
    { &GUID_Key, 250, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(250), 0 },
    { &GUID_Key, 251, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(251), 0 },
    { &GUID_Key, 252, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(252), 0 },
    { &GUID_Key, 253, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(253), 0 },
    { &GUID_Key, 254, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(254), 0 },
    { &GUID_Key, 255, DIDFT_OPTIONAL | DIDFT_BUTTON | DIDFT_MAKEINSTANCE(255), 0 },
};

// 0x4FDF20
const DIDATAFORMAT c_dfDIKeyboard = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    256,
    sizeof(dfDIKeyboard) / sizeof(dfDIKeyboard[0]),
    (LPDIOBJECTDATAFORMAT)dfDIKeyboard
};

// 0x51E458
LPDIRECTINPUTA gDirectInput = NULL;

// 0x51E45C
LPDIRECTINPUTDEVICEA gMouseDevice = NULL;

// 0x51E460
LPDIRECTINPUTDEVICEA gKeyboardDevice = NULL;

// 0x51E464
int gKeyboardDeviceDataIndex = 0;

// 0x51E468
int gKeyboardDeviceDataLength = 0;

// 0x6B2560
DIDEVICEOBJECTDATA gKeyboardDeviceData[KEYBOARD_DEVICE_DATA_CAPACITY];

// 0x4E0400
bool directInputInit()
{
    if (gDirectInput != NULL) {
        return false;
    }

    HRESULT hr = gDirectInputCreateAProc(gInstance, DIRECTINPUT_VERSION, &gDirectInput, NULL);
    if (hr != DI_OK) {
        goto err;
    }

    if (!mouseDeviceInit()) {
        goto err;
    }

    if (!keyboardDeviceInit()) {
        goto err;
    }

    return true;

err:

    keyboardDeviceFree();
    mouseDeviceFree();

    if (gDirectInput != NULL) {
        IDirectInput_Release(gDirectInput);
        gDirectInput = NULL;
    }

    return false;
}

// 0x4E0478
void directInputFree()
{
    // NOTE: Uninline.
    keyboardDeviceFree();

    // NOTE: Uninline.
    mouseDeviceFree();

    if (gDirectInput != NULL) {
        IDirectInput_Release(gDirectInput);
        gDirectInput = NULL;
    }
}

// 0x4E04E8
bool mouseDeviceAcquire()
{
    if (gMouseDevice == NULL) {
        return false;
    }

    HRESULT hr = IDirectInputDevice_Acquire(gMouseDevice);
    if (hr != DI_OK && hr != S_FALSE) {
        return false;
    }

    return true;
}

// 0x4E0514
bool mouseDeviceUnacquire()
{
    if (gMouseDevice == NULL) {
        return false;
    }

    HRESULT hr = IDirectInputDevice_Unacquire(gMouseDevice);
    if (hr != DI_OK) {
        return false;
    }

    return true;
}

// 0x4E053C
bool mouseDeviceGetData(MouseData* mouseState)
{
    if (gMouseDevice == NULL) {
        return false;
    }

    if (!mouseDeviceAcquire()) {
        return false;
    }

    DIMOUSESTATE dims;
    HRESULT hr = IDirectInputDevice_GetDeviceState(gMouseDevice, sizeof(dims), &dims);
    if (hr != DI_OK) {
        return false;
    }

    mouseState->x = dims.lX;
    mouseState->y = dims.lY;
    mouseState->buttons[0] = (dims.rgbButtons[0] & 0x80) != 0;
    mouseState->buttons[1] = (dims.rgbButtons[1] & 0x80) != 0;

    return true;
}

// 0x4E05A8
bool keyboardDeviceAcquire()
{
    if (gKeyboardDevice == NULL) {
        return false;
    }

    HRESULT hr = IDirectInputDevice_Acquire(gKeyboardDevice);
    if (hr != DI_OK && hr != S_FALSE) {
        return false;
    }

    return true;
}

// 0x4E05D4
bool keyboardDeviceUnacquire()
{
    if (gKeyboardDevice == NULL) {
        return false;
    }

    HRESULT hr = IDirectInputDevice_Unacquire(gKeyboardDevice);
    if (hr != DI_OK) {
        return false;
    }

    return true;
}

// 0x4E05FC
bool keyboardDeviceReset()
{
    if (gKeyboardDevice == NULL) {
        return false;
    }

    if (!keyboardDeviceAcquire()) {
        return false;
    }

    DWORD items = -1;
    HRESULT hr = IDirectInputDevice_GetDeviceData(gKeyboardDevice, sizeof(DIDEVICEOBJECTDATA), NULL, &items, 0);
    if (hr != DI_OK && hr != DI_BUFFEROVERFLOW) {
        return false;
    }

    return true;
}

// 0x4E0650
bool keyboardDeviceGetData(KeyboardData* keyboardData)
{
    if (gKeyboardDevice == NULL) {
        return false;
    }

    if (!keyboardDeviceAcquire()) {
        return false;
    }

    if (gKeyboardDeviceDataIndex >= gKeyboardDeviceDataLength) {
        DWORD items = KEYBOARD_DEVICE_DATA_CAPACITY;
        HRESULT hr = IDirectInputDevice_GetDeviceData(gKeyboardDevice, sizeof(DIDEVICEOBJECTDATA), gKeyboardDeviceData, &items, 0);
        if (hr == DI_OK || hr == DI_BUFFEROVERFLOW) {
            gKeyboardDeviceDataLength = items;
            gKeyboardDeviceDataIndex = 0;
        }
    }

    if (gKeyboardDeviceDataIndex < gKeyboardDeviceDataLength) {
        DIDEVICEOBJECTDATA* entry = &(gKeyboardDeviceData[gKeyboardDeviceDataIndex]);
        keyboardData->key = entry->dwOfs & 0xFF;
        keyboardData->down = (entry->dwData & 0x80) != 0;
        gKeyboardDeviceDataIndex++;

        return true;
    }

    return false;
}

// 0x4E070C
bool mouseDeviceInit()
{
    HRESULT hr;

    hr = IDirectInput_CreateDevice(gDirectInput, &GUID_SysMouse, &gMouseDevice, NULL);
    if (hr != DI_OK) {
        goto err;
    }

    hr = IDirectInputDevice_SetCooperativeLevel(gMouseDevice, gProgramWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if (hr != DI_OK) {
        goto err;
    }

    hr = IDirectInputDevice_SetDataFormat(gMouseDevice, &c_dfDIMouse);
    if (hr != DI_OK) {
        goto err;
    }

    return true;

err:

    // NOTE: Uninline.
    mouseDeviceFree();

    return false;
}

// 0x4E078C
void mouseDeviceFree()
{
    if (gMouseDevice != NULL) {
        IDirectInputDevice_Unacquire(gMouseDevice);
        IDirectInputDevice_Release(gMouseDevice);
        gMouseDevice = NULL;
    }
}

// 0x4E07B8
bool keyboardDeviceInit()
{
    HRESULT hr;

    hr = IDirectInput_CreateDevice(gDirectInput, &GUID_SysKeyboard, &gKeyboardDevice, NULL);
    if (hr != DI_OK) {
        goto err;
    }

    hr = IDirectInputDevice_SetCooperativeLevel(gKeyboardDevice, gProgramWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    if (hr != DI_OK) {
        goto err;
    }

    hr = IDirectInputDevice_SetDataFormat(gKeyboardDevice, &c_dfDIKeyboard);
    if (hr != DI_OK) {
        goto err;
    }

    DIPROPDWORD dipdw;
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = KEYBOARD_DEVICE_DATA_CAPACITY;

    hr = IDirectInputDevice_SetProperty(gKeyboardDevice, DIPROP_BUFFERSIZE, &(dipdw.diph));
    if (hr != DI_OK) {
        goto err;
    }

    return true;

err:

    // NOTE: Uninline.
    keyboardDeviceFree();

    return false;
}

// 0x4E0874
void keyboardDeviceFree()
{
    if (gKeyboardDevice != NULL) {
        IDirectInputDevice_Unacquire(gKeyboardDevice);
        IDirectInputDevice_Release(gKeyboardDevice);
        gKeyboardDevice = NULL;
    }
}
