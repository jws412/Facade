#pragma once

enum {
    tileAir,
    tileStone,
    TILE_VARIETY
};

// Avoid changing the name of the enumeration entries below. The litteral
// enumeration item names are used in string preprocessing.
enum {
    player,
    bug,
    CHARACTER_VARIETY,
    idNull = 255,
};

#define MICROSEC_PER_UPDATE_LOGIC 16667
#define MICROSEC_PER_UPDATE_SLEEP MICROSEC_PER_UPDATE_LOGIC * 2000 / 2381
#define MINIMUM_TIME_RESOLUTION 3

#define BACKBUFFER_WIDTH 384
#define BACKBUFFER_HEIGHT 216
#define TILE_SIZE 16
#define COLUMN_SIZE (UINT8) (BACKBUFFER_HEIGHT / TILE_SIZE)

#define DEBUG_CHAR_HEIGHT 14
#define DEBUG_CHAR_WIDTH 11
#define DEBUG_METRICS_LINE_SIZE 6
#define MAX_DEBUG_MESSAGE_SIZE (BACKBUFFER_WIDTH / DEBUG_CHAR_WIDTH)
#define MAX_DEBUG_MESSAGE_NUMBER (BACKBUFFER_HEIGHT / DEBUG_CHAR_HEIGHT - DEBUG_METRICS_LINE_SIZE)

#define MAX_CHARACTER_NUM 8

#define panic(str) MessageBox(NULL, str, "An unexpected error has occured.", MB_ICONEXCLAMATION | MB_OK); PostQuitMessage(0);

#define TRUE 1
#define FALSE 0

/*
 * The file section below defines all structs required to be defined
 * by dispersed files. This file in general serves as a coordinator
 * between separate files.
 */
 
// The struct established below is referenced for any instance of some
// two-dimensional delimitations or a two-dimensional area.
typedef struct {
    UINT16 width;
    UINT16 height;
} sDimensions;

// The struct defined below is referenced in any object created in this
// application.
typedef struct {
    UINT16 x;
    UINT16 y;
} sPosition;

// The struct below is intended to describe vertical and horizontal
// velocities of objects bearing these motions.
typedef struct {
    INT8 x;
    INT8 y;
} sVelocity;

// The struct below is intended to describe the width and height of a
// character's collision box and dimensions of its graphics.
typedef struct {
    UINT8 width;
    UINT8 height;
} sCollision;

// The struct below defines the pixel color format used for displaying
// pixels on the backbuffer.
typedef union {
    struct {
        UINT8 blue;
        UINT8 green;
        UINT8 red;
        UINT8 alpha;
    };
    UINT32 whole;
} sPixel;

// All entities in this application whose behavior is as a function of time
// or of other entities of this type are referred to as "characters." Any
// character instance either has specific attributes belonging to itself,
// such as its position, or attributes that are shared among all instances
// of characters that share the same ID, such as its graphics.

// The struct below is intended to save information about each mold in 
// this program. This struct is comprised of a collision width and
// height, as well as a maximum horizontal speed and a number of unique
// animation frames that the pixel data associated to the mold is intended 
// to describe.
typedef struct {
    union {
        struct {
            sCollision collision;
            UINT8 maxSpeedX;
            UINT8 frames;
        };
        UINT32 moldInfo;
    };
    sPixel* pPixelData;
} sMold;

// The struct below aims to have all information required for rendering a
// particular character.
typedef struct { 
    sPosition pos;
    sVelocity velocity;
    UINT8 id;
    INT8 animState;
} sCharacter;

// The "sCharacterArray" struct stores a pointer to memory intended to store
// "sCharacter" instances linearly. This struct also features the "instances"
// member, which describes the number of said instances.
typedef struct {
    sCharacter* pCharacter;
    UINT16 instances;
} sCharacterArray;

// The "sBitmap" struct is used to store information regarding a bitmap.
// It indicates the address of the bitmap's allocated memory.
typedef struct {
    HBITMAP bitmapHandle;
    void* pPixelData;
    UINT32 memorySize;
} sBitmap;

// The struct defined below is referenced once create a mediator that
// allows the logic of the application to communicate with the rendering
// process of this same application.
typedef struct {
    CHAR* pMessages[MAX_DEBUG_MESSAGE_NUMBER];
    UINT8 messageSizes[MAX_DEBUG_MESSAGE_NUMBER];
} sRenderInfo;

// The struct defined below is used to save information about the level
// that is currently being loaded for the player character to traverse.
typedef struct {
    sPosition posPlayerSpawn;
    UINT16 width;
    BYTE* pTilemap;
} sLevelInfo;

/*
 * The following function definitions apply for functions intended to have
 * global use in any part of the application.
 */

 __cdecl void debugPrintf(const CHAR* string, ...);

/*
 * The variables below are used by dispersed files and used to share
 * information between each other.
 */
 
// The array below is used to store each unique character mold. The index
// of any mold corresponds to its ID, which is shared in any ID to character
// mapping. The mold information of any character can be obtained by fetching
// the value in the array below givn said character's id.
sMold gCharacterMolds[CHARACTER_VARIETY];

// The struct instance "gMutableCharacterArray" stores all references to characters
// loaded in the current level.
sCharacterArray gMutableCharacterArray;
// The array labeled "gTile" is intended to store a pointer to the tile
// texture altas.
sPixel gTileAtlas[TILE_VARIETY][TILE_SIZE * TILE_SIZE];
// The "gBackbuffer" variable stores the address of the backbuffer for
// rendering any graphic. This backbuffer's pixel data is then rendered
// on the application window's pixel data.
sBitmap gBackbuffer;
// The variable below stores the character struct of the player. Given that
// the playe character is the most frequently access character, less
// element fetching from arrays can be achieved.
sCharacter gPlayer = {0};

sDimensions gWindowDimensions;
sLevelInfo gLevel;
BOOLEAN gIsDebug = FALSE;

// Variable used to pass information to rendering protocol. The message
// buffers of the struct defined below are allocated their respective
// memories near the beginning of the entry point of this application.
// This variable must be accessible from a global scale given that any part
// of this program may change data in it.
sRenderInfo gRenderInfo = {0};

// Variable only used in the function below.
CHAR* pOldestMessage;

__cdecl void debugPrintf(const CHAR* restrict string, ...) {    
    // Save the message pointer to be removed, which is also the oldest one.
    pOldestMessage = gRenderInfo.pMessages[(MAX_DEBUG_MESSAGE_NUMBER - 1)];
    
    for (INT8 i = (MAX_DEBUG_MESSAGE_NUMBER - 1); i > 0; i--) {
        // Shift message pointers in the array of addresses to these string
        // messages.
        gRenderInfo.pMessages[i] = gRenderInfo.pMessages[i - 1];
        // Shift message sizes.
        gRenderInfo.messageSizes[i] = gRenderInfo.messageSizes[i - 1];
    }
    gRenderInfo.pMessages[0] = pOldestMessage;
    // It is essential to not make this oldest address point to any datum
    // after it was used since said information can be unallocated. 
    pOldestMessage = NULL;
    
    va_list args;
    va_start(args, string);
    // Write the formatted string directly to the string buffer of the
    // struct interpreted by the rendering process.
    gRenderInfo.messageSizes[0] = vsprintf(
        gRenderInfo.pMessages[0],
        string, 
        args);
    va_end(args);
    
    return;
}