#ifndef SELFRUN_H
#define SELFRUN_H

typedef enum SelfrunState {
    SELFRUN_STATE_TURNED_OFF,
    SELFRUN_STATE_PLAYING,
    SELFRUN_STATE_RECORDING,
} SelfrunState;

typedef struct SelfrunHeader {
    char field_0[13];
    char field_D[13];
    int field_1C;
} SelfrunData;

static_assert(sizeof(SelfrunData) == 32, "wrong size");

extern int gSelfrunState;

int selfrunInitFileList(char*** fileListPtr, int* fileListLengthPtr);
int selfrunFreeFileList(char*** fileListPtr);
int selfrunPreparePlayback(const char* fileName, SelfrunData* selfrunHeader);
void selfrunPlaybackLoop(SelfrunData* selfrunData);
int selfrunPrepareRecording(const char* a1, char* a2, SelfrunData* selfrunData);
void selfrunRecordingLoop(SelfrunData* selfrunData);
void selfrunPlaybackCompleted(int reason);
int selfrunReadData(const char* path, SelfrunData* selfrunData);
int selfrunWriteData(const char* path, SelfrunData* selfrunData);

#endif /* SELFRUN_H */
