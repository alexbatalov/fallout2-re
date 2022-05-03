#include "geometry.h"

#include "memory.h"

#include <stdlib.h>

// 0x51DEF4
RectListNode* off_51DEF4 = NULL;

// 0x4C6900
void sub_4C6900()
{
    while (off_51DEF4 != NULL) {
        RectListNode* next = off_51DEF4->next;
        internal_free(off_51DEF4);
        off_51DEF4 = next;
    }
}

// 0x4C6924
void sub_4C6924(RectListNode** rectListNodePtr, Rect* rect)
{
    Rect v1;
    rectCopy(&v1, rect);

    // NOTE: Original code is slightly different.
    while (*rectListNodePtr != NULL) {
        RectListNode* rectListNode = *rectListNodePtr;
        if (v1.right >= rectListNode->rect.left
            && v1.bottom >= rectListNode->rect.top
            && v1.left <= rectListNode->rect.right
            && v1.top <= rectListNode->rect.bottom) {
            Rect v2;
            rectCopy(&v2, &(rectListNode->rect));

            *rectListNodePtr = rectListNode->next;

            rectListNode->next = off_51DEF4;
            off_51DEF4 = rectListNode;

            if (v2.top < v1.top) {
                RectListNode* newRectListNode = sub_4C6BB8();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.bottom = v1.top - 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);

                v2.top = v1.top;
            }

            if (v2.bottom > v1.bottom) {
                RectListNode* newRectListNode = sub_4C6BB8();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.top = v1.bottom + 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);

                v2.bottom = v1.bottom;
            }

            if (v2.left < v1.left) {
                RectListNode* newRectListNode = sub_4C6BB8();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.right = v1.left - 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);
            }

            if (v2.right > v1.right) {
                RectListNode* newRectListNode = sub_4C6BB8();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.left = v1.right + 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);
            }
        } else {
            rectListNodePtr = &(rectListNode->next);
        }
    }
}

// 0x4C6BB8
RectListNode* sub_4C6BB8()
{
    if (off_51DEF4 == NULL) {
        for (int index = 0; index < 10; index++) {
            RectListNode* rectListNode = internal_malloc(sizeof(*rectListNode));
            if (rectListNode == NULL) {
                break;
            }

            // NOTE: Uninline.
            sub_4C6C04(rectListNode);
        }
    }

    if (off_51DEF4 == NULL) {
        return NULL;
    }

    RectListNode* rectListNode = off_51DEF4;
    off_51DEF4 = off_51DEF4->next;

    return rectListNode;
}

// 0x4C6C04
void sub_4C6C04(RectListNode* rectListNode)
{
    rectListNode->next = off_51DEF4;
    off_51DEF4 = rectListNode;
}

// Calculates a union of two source rectangles and places it into result
// rectangle.
//
// 0x4C6C18
void rectUnion(const Rect* s1, const Rect* s2, Rect* r)
{
    r->left = min(s1->left, s2->left);
    r->top = min(s1->top, s2->top);
    r->right = max(s1->right, s2->right);
    r->bottom = max(s1->bottom, s2->bottom);
}

// Calculates intersection of two source rectangles and places it into third
// rectangle and returns 0. If two source rectangles do not have intersection
// it returns -1 and resulting rectangle is a copy of s1.
//
// 0x4C6C68
int rectIntersection(const Rect* s1, const Rect* s2, Rect* r)
{
    r->left = s1->left;
    r->top = s1->top;
    r->right = s1->right;
    r->bottom = s1->bottom;

    if (s1->left <= s2->right && s2->left <= s1->right && s2->bottom >= s1->top && s2->top <= s1->bottom) {
        if (s2->left > s1->left) {
            r->left = s2->left;
        }

        if (s2->right < s1->right) {
            r->right = s2->right;
        }

        if (s2->top > s1->top) {
            r->top = s2->top;
        }

        if (s2->bottom < s1->bottom) {
            r->bottom = s2->bottom;
        }

        return 0;
    }

    return -1;
}
