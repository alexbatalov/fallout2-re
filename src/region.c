#include "region.h"

#include "debug.h"
#include "memory_manager.h"

#include <limits.h>
#include <string.h>

static_assert(sizeof(Region) == 140, "wrong size");

char byte_50D394[] = "<null>";

// 0x4A2D78
Region* regionCreate(int initialCapacity)
{
    Region* region = internal_malloc_safe(sizeof(*region), __FILE__, __LINE__); // "..\int\REGION.C", 142
    memset(region, 0, sizeof(*region));

    if (initialCapacity != 0) {
        region->points = internal_malloc_safe(sizeof(*region->points) * (initialCapacity + 1), __FILE__, __LINE__); // "..\int\REGION.C", 147
        region->pointsCapacity = initialCapacity + 1;
    } else {
        region->points = NULL;
        region->pointsCapacity = 0;
    }

    region->name[0] = '\0';
    region->field_74 = 0;
    region->field_28 = INT_MIN;
    region->field_30 = INT_MAX;
    region->field_54 = 0;
    region->field_5C = 0;
    region->field_64 = 0;
    region->field_68 = 0;
    region->field_6C = 0;
    region->field_70 = 0;
    region->field_60 = 0;
    region->field_78 = 0;
    region->field_7C = 0;
    region->field_80 = 0;
    region->field_84 = 0;
    region->pointsLength = 0;
    region->field_24 = 0;
    region->field_2C = 0;
    region->field_50 = 0;
    region->field_4C = 0;
    region->field_48 = 0;
    region->field_58 = 0;

    return region;
}

// regionAddPoint
// 0x4A2E68
void regionAddPoint(Region* region, int x, int y)
{
    if (region == NULL) {
        debugPrint("regionAddPoint(): null region ptr\n");
        return;
    }

    if (region->points != NULL) {
        if (region->pointsCapacity - 1 == region->pointsLength) {
            region->points = internal_realloc_safe(region->points, sizeof(*region->points) * (region->pointsCapacity + 1), __FILE__, __LINE__); // "..\int\REGION.C", 190
            region->pointsCapacity++;
        }
    } else {
        region->pointsCapacity = 2;
        region->pointsLength = 0;
        region->points = internal_malloc_safe(sizeof(*region->points) * 2, __FILE__, __LINE__); // "..\int\REGION.C", 185
    }

    int pointIndex = region->pointsLength;
    region->pointsLength++;

    Point* point = &(region->points[pointIndex]);
    point->x = x;
    point->y = y;

    Point* end = &(region->points[pointIndex + 1]);
    end->x = region->points->x;
    end->y = region->points->y;
}

// regionDelete
// 0x4A2F0C
void regionDelete(Region* region)
{
    if (region == NULL) {
        debugPrint("regionDelete(): null region ptr\n");
        return;
    }

    if (region->points != NULL) {
        internal_free_safe(region->points, __FILE__, __LINE__); // "..\int\REGION.C", 206
    }

    internal_free_safe(region, __FILE__, __LINE__); // "..\int\REGION.C", 207
}

// regionAddName
// 0x4A2F54
void regionSetName(Region* region, char* name)
{
    if (region == NULL) {
        debugPrint("regionAddName(): null region ptr\n");
        return;
    }

    if (name == NULL) {
        region->name[0] = '\0';
        return;
    }

    strncpy(region->name, name, REGION_NAME_LENGTH - 1);
}

// regionGetName
// 0x4A2F80
char* regionGetName(Region* region)
{
    if (region == NULL) {
        debugPrint("regionGetName(): null region ptr\n");
        return byte_50D394;
    }

    return region->name;
}

// regionGetUserData
// 0x4A2F98
void* regionGetUserData(Region* region)
{
    if (region == NULL) {
        debugPrint("regionGetUserData(): null region ptr\n");
        return NULL;
    }

    return region->userData;
}

// regionSetUserData
// 0x4A2FB4
void regionSetUserData(Region* region, void* data)
{
    if (region == NULL) {
        debugPrint("regionSetUserData(): null region ptr\n");
        return;
    }

    region->userData = data;
}
