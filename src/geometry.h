#ifndef GEOMETRY_H
#define GEOMETRY_H

typedef struct Point {
    int x;
    int y;
} Point;

typedef struct Size {
    int width;
    int height;
} Size;

typedef struct Rect {
    int left;
    int top;
    int right;
    int bottom;
} Rect;

typedef struct RectListNode {
    Rect rect;
    struct RectListNode* next;
} RectListNode;

extern RectListNode* off_51DEF4;

void GNW_rect_exit();
void rect_clip_list(RectListNode** rectListNodePtr, Rect* rect);
RectListNode* rect_malloc();
void rect_free(RectListNode* entry);
void rectUnion(const Rect* s1, const Rect* s2, Rect* r);
int rectIntersection(const Rect* a1, const Rect* a2, Rect* a3);

static inline void rectCopy(Rect* dest, const Rect* src)
{
    dest->left = src->left;
    dest->top = src->top;
    dest->right = src->right;
    dest->bottom = src->bottom;
}

static inline int rectGetWidth(const Rect* rect)
{
    return rect->right - rect->left + 1;
}

static inline int rectGetHeight(const Rect* rect)
{
    return rect->bottom - rect->top + 1;
}

static inline void rectOffset(Rect* rect, int dx, int dy)
{
    rect->left += dx;
    rect->top += dy;
    rect->right += dx;
    rect->bottom += dy;
}

#endif /* GEOMETRY_H */
