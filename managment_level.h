#pragma once

#include "coordinator.h"
#include "prop_dir.h"

/*
 * The function declerations of this file are used by protocols pertaining
 * to levels. A procedure is required for extracting and formating level 
 * data from level (".lvl") files.
 */

__forceinline LRESULT initLevel();

__forceinline void freeTilemap();

/*
 * The "initLevel" function translates level data to tilemap data. This
 * extraction requires the allocation of memory for saving tilemap
 * information. This tilemap's address is saved in the "gLevel" struct for 
 * global access. Deallocation of this memory is performed in the cleanup
 * procedure in the "WinMain" function. A spawnpoint for the player is
 * also identified and saved in said struct.
 */

__forceinline LRESULT initLevel() {
    
    FILE* pFile = fopen(DIR_FIRST_LEVEL, "rb");
    if (pFile == NULL) {
        panic("Entry level file was not found.");
        return ERROR_FILE_NOT_FOUND;
    }
    // This variable stores the second part of the 12-bit value used to
    // describe the width of the level in tiles.
    BYTE levelWidthPieceLow;
    const UINT levelColumns = ((fgetc(pFile) << 4) 
        + ((levelWidthPieceLow = fgetc(pFile)) >> 4));
    const UINT levelTileRawBytes = levelColumns * COLUMN_SIZE;
    const UINT levelTileAlignedBytes = levelTileRawBytes / 8
        + ((levelTileRawBytes % 8) != 0);
    // Save the boundary that the player character cannot pass beyond for the
    // number of columns of the level read from the level file.
    gLevel.width = levelColumns * TILE_SIZE;
    BYTE* pRawLevelBytes = malloc(levelTileAlignedBytes);
    if (pRawLevelBytes == NULL) {
        panic("Temporary raw level data memory allocation failed.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    // At this point, the first two bytes of the level file were read.
    fread(pRawLevelBytes, levelTileAlignedBytes, 1, pFile);
    fclose(pFile);
    // The size of the allocated memory for the tilemap corresponds to the
    // number of bits used in the level file used to describe the level
    // layout.
    BYTE* pTilemap = (
        gLevel.pTilemap = malloc(levelColumns * COLUMN_SIZE));
    if (pTilemap == NULL) {
        panic("Tilemap memory allocation failed.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    // The variables below are used to determine at which point the sequence
    // consisting of a non-air tile immediately followed by a solid tile is
    // encountered.
    BYTE prev, cur;
    // The last four bits from the lower magnitude portion of the number of
    // tile columns in the level extracted from the file must be added to
    // tilemap.
    pTilemap[0] = (levelWidthPieceLow >> 3) & 1;
    pTilemap[1] = (levelWidthPieceLow >> 2) & 1;
    pTilemap[2] = (levelWidthPieceLow >> 1) & 1;
    // The tile below is required to determine if the fourth and fifth tile
    // of the tilemap constitute a valid spawnpoint for the player character.
    pTilemap[3] = (prev = (levelWidthPieceLow & 1));
    // Analyze only the four least significant bits of the second part of the
    // column count value to determine if this part can potentially describe
    // the spawnpoint coordinates of the player character. These bits are
    // skipped in the translation of the level file's tile layout data to
    // to corresponding tilemap.
    levelWidthPieceLow &= 0x0F;
    // The iterator variables will be set to their maximum values if the
    // player character spawn point is found within the first four bits.
    UINT32 bytes = 0;
    UINT8 bits;
    switch(levelWidthPieceLow) {
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        
        gLevel.posPlayerSpawn.x = 0;
        gLevel.posPlayerSpawn.y = 1 * TILE_SIZE;
        break;
        
        case 0x04: case 0x05: case 0x0C: case 0x0D:
        
        gLevel.posPlayerSpawn.x = 0;
        gLevel.posPlayerSpawn.y = 2 * TILE_SIZE;
        break;
        
        case 0x02: case 0x06: case 0x0E:
        
        gLevel.posPlayerSpawn.x = 0;
        gLevel.posPlayerSpawn.y = 3 * TILE_SIZE;
        break;
        
        default:
        // Not only does the code below convert every bit of the level file's
        // contents into byte values in order of their respective addresses, 
        // but it also reccords the position of the tile in the 
        // memory-allocated array of tiles that is a non-air tile immediately
        // followed by an air tile. This arrangment of tiles determines the 
        // spawnpoint of the player character.
        for (; bytes < levelTileAlignedBytes; bytes++) {
            for (bits = 0; bits < 8; bits++) {
                // The summand of four is included to consider the four 
                // last bits added from the second byte of the files that 
                // are part of the tilemap description of the level. These 
                // bits were already saved.
                // It is also important to metion that the tiles in the 
                // level file are arranged, from lowest to highest 
                // address, bottom to top, left to right.
                cur = (pRawLevelBytes[bytes] >> (7 - bits)) & 1;
                if (prev != tileAir && cur == tileAir) {
                    UINT16 tileIndex = (bytes * 8) + bits + 4;
                    gLevel.posPlayerSpawn.x = (tileIndex / COLUMN_SIZE)
                        * TILE_SIZE;
                    gLevel.posPlayerSpawn.y = (tileIndex % COLUMN_SIZE)
                        * TILE_SIZE;
                    pTilemap[tileIndex] = cur;
                    // Increment to prepare for the next loop.
                    bits++;
                    goto spawnFound;
                }
                pTilemap[(bytes * 8) + bits + 4] = cur;
                prev = cur;
            } 
        }
        break;
    }
    // The counterpart of the loop above without checking for the player
    // spawnpoint is found below. The outer loop iterating variable "bytes" does
    // not need to be initialized since the "goto" statment makes the second
    // nested loop resume the progress of the first nested loop. In addition,
    // this variable should not be ever initialized at this stage to consider
    // the case where no player character spawnpoint can be located. This 
    // case suggests that there no need to iterate through the tilemap again.
    for (; bytes < levelTileAlignedBytes; bytes++) {
        for (; bits < 8; bits++) {
            spawnFound:
            
            pTilemap[(bytes * 8) + bits + 4] = 
                (pRawLevelBytes[bytes] >> (7 - bits)) & 1;
        }
        bits = 0;
    }
    
    free(pRawLevelBytes);
    return ERROR_SUCCESS;
}

__forceinline void freeTilemap() {
    
    free(gLevel.pTilemap);
    return;
}