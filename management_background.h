#pragma once

#include "coordinator.h"
#include "decode.h"
#include "dir.h"

/*
 * The function prototypes highlighted in this file pertain to
 * initializing the background. This background is rendered prior to all
 * other textures in the rendering protocol.
 */

__forceinline LRESULT initBackground(
        sPixel* const restrict pPixelRegion,
        const UINT32 pixels,
        UINT8* const restrict pColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData);


/*
 * The "initBackground" function sets all pixels of the passed pixel
 * array. The set pixels are from the "bck.bci" file.
 */

__forceinline LRESULT initBackground(
        sPixel* const restrict pPixelRegion,
        const UINT32 pixels,
        UINT8* const restrict pColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData) {
    
    FILE* pFile = fopen(DIR_BACKGROUND, "rb");
    
    if (pFile == NULL) {
        panic("Background graphic is missing in " DIR_BACKGROUND ".");
        return ERROR_FILE_NOT_FOUND;
    }
    
    switch(fReadImage(
            pixels,
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
    fclose(pFile);
    
    decodePixelDataFeaturing(
        *pColors,
        *pColorCodeBits,
        *pEncodedPixelDataBytes,
        *ppPalette,
        *ppColorCodeToPixelMapping,
        *ppEncodedPixelData,
        pPixelRegion);
    
    return ERROR_SUCCESS;
}