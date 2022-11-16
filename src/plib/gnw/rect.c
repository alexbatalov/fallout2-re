#include "plib/gnw/rect.h"

#include <stdlib.h>

#include "plib/gnw/memory.h"

// 0x51DEF4
static RectPtr rlist = NULL;

// 0x4C6900
void GNW_rect_exit()
{
    RectPtr temp;

    while (rlist != NULL) {
        temp = rlist->next;
        mem_free(rlist);
        rlist = temp;
    }
}

// 0x4C6924
void rect_clip_list(RectPtr* rectListNodePtr, Rect* rect)
{
    Rect v1;
    rectCopy(&v1, rect);

    // NOTE: Original code is slightly different.
    while (*rectListNodePtr != NULL) {
        RectPtr rectListNode = *rectListNodePtr;
        if (v1.lrx >= rectListNode->rect.ulx
            && v1.lry >= rectListNode->rect.uly
            && v1.ulx <= rectListNode->rect.lrx
            && v1.uly <= rectListNode->rect.lry) {
            Rect v2;
            rectCopy(&v2, &(rectListNode->rect));

            *rectListNodePtr = rectListNode->next;

            rectListNode->next = rlist;
            rlist = rectListNode;

            if (v2.uly < v1.uly) {
                RectPtr newRectListNode = rect_malloc();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.lry = v1.uly - 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);

                v2.uly = v1.uly;
            }

            if (v2.lry > v1.lry) {
                RectPtr newRectListNode = rect_malloc();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.uly = v1.lry + 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);

                v2.lry = v1.lry;
            }

            if (v2.ulx < v1.ulx) {
                RectPtr newRectListNode = rect_malloc();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.lrx = v1.ulx - 1;
                newRectListNode->next = *rectListNodePtr;

                *rectListNodePtr = newRectListNode;
                rectListNodePtr = &(newRectListNode->next);
            }

            if (v2.lrx > v1.lrx) {
                RectPtr newRectListNode = rect_malloc();
                if (newRectListNode == NULL) {
                    return;
                }

                rectCopy(&(newRectListNode->rect), &v2);
                newRectListNode->rect.ulx = v1.lrx + 1;
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
RectPtr rect_malloc()
{
    RectPtr temp;
    int i;

    if (rlist == NULL) {
        for (i = 0; i < 10; i++) {
            temp = (RectPtr)mem_malloc(sizeof(*temp));
            if (temp == NULL) {
                break;
            }

            temp->next = rlist;
            rlist = temp;
        }
    }

    if (rlist == NULL) {
        return NULL;
    }

    temp = rlist;
    rlist = rlist->next;

    return temp;
}

// 0x4C6C04
void rect_free(RectPtr ptr)
{
    ptr->next = rlist;
    rlist = ptr;
}

// Calculates a union of two source rectangles and places it into result
// rectangle.
//
// 0x4C6C18
void rect_min_bound(const Rect* r1, const Rect* r2, Rect* min_bound)
{
    min_bound->ulx = min(r1->ulx, r2->ulx);
    min_bound->uly = min(r1->uly, r2->uly);
    min_bound->lrx = max(r1->lrx, r2->lrx);
    min_bound->lry = max(r1->lry, r2->lry);
}

// Calculates intersection of two source rectangles and places it into third
// rectangle and returns 0. If two source rectangles do not have intersection
// it returns -1 and resulting rectangle is a copy of r1.
//
// 0x4C6C68
int rect_inside_bound(const Rect* r1, const Rect* bound, Rect* r2)
{
    r2->ulx = r1->ulx;
    r2->uly = r1->uly;
    r2->lrx = r1->lrx;
    r2->lry = r1->lry;

    if (r1->ulx <= bound->lrx && bound->ulx <= r1->lrx && bound->lry >= r1->uly && bound->uly <= r1->lry) {
        if (bound->ulx > r1->ulx) {
            r2->ulx = bound->ulx;
        }

        if (bound->lrx < r1->lrx) {
            r2->lrx = bound->lrx;
        }

        if (bound->uly > r1->uly) {
            r2->uly = bound->uly;
        }

        if (bound->lry < r1->lry) {
            r2->lry = bound->lry;
        }

        return 0;
    }

    return -1;
}
