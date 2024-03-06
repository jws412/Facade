#pragma once

#include "coordinator.h"
#include "read.h"
#include "decode.h"
#include "prop_dir.h"

#define BLOCK_CHARACTER_MACROS
#define BLOCK_RENDER_MACROS

/*
 * This document features function prototypes intended for managing tile
 * rendering.
 */

__forceinline LRESULT initTilePixelData(
        UINT8* const restrict pColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData);

/*
 * The "initTilePixelData" function outlines the procedure required 
 * initialize graphic tile data. This initialization procedure includes
 * allocating memory for each unique tile graphic and their respective
 * initialization.
 */

__forceinline LRESULT initTilePixelData(
        UINT8* const restrict pColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData) {
    
    FILE* pFile = fopen(DIR_TILE, "rb");    
    if (pFile == NULL) {
        panic("Texture map file \"" DIR_TILE "\" was not found.");
        return ERROR_FILE_NOT_FOUND;
    }
    
    for (UINT tileId = 0; tileId < TILE_VARIETY; tileId++) {
        switch(fReadImage(
                TILE_SIZE * TILE_SIZE,
                pFile,
                pColors,
                pColorCodeBits,
                pEncodedPixelDataBytes,
                ppPalette,
                ppColorCodeToPixelMapping,
                ppEncodedPixelData)) {
            
            case ERROR_NOT_ENOUGH_MEMORY:
            
            return ERROR_NOT_ENOUGH_MEMORY;
            
            case ERROR_SUCCESS:

            break;
            
            default:
            
            return ERROR_INVALID_DATA;
        }
        decodePixelDataFeaturing(
            *pColors,
            *pColorCodeBits,
            *pEncodedPixelDataBytes,
            *ppPalette,
            *ppColorCodeToPixelMapping,
            *ppEncodedPixelData,
            (sPixel* restrict const) &(gTileAtlas[tileId][0]));
    }
    fclose(pFile);
    
    return ERROR_SUCCESS;
}