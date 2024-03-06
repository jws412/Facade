#pragma once

#include "coordinator.h"
#include "read.h"
#include "prop_character.h"

#define bitmapDirOf(character) DIR_CHARACTER #character ".bci"
#define moldDirOf(character) DIR_CHARACTER #character ".mld"

#include "prop_dir.h"

/*
 * The code section below describes all functions pertaining to creating
 * characters in a level. Such functions must all be included in this file.
 */

__forceinline LRESULT initCharacterMolds(
        UINT8* const restrict pColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData);

__forceinline void freeCharactersMolds();

/*
 * The "initCharacterMolds" function is called exactly once in the main 
 * function of this program. This call's intent is to create so called 
 * "molds" for characters. These items are used as bases for all created 
 * characters to have their attributes initialized from.
 */

__forceinline LRESULT initCharacterMolds(
        UINT8* const restrict pColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData) {    
    
    const CHAR* dirPixelData[] = generateCharacterArray(bitmapDirOf);
    const CHAR* dirMold[] = generateCharacterArray(moldDirOf);
    // The set of variables below corresponds to all variables used in the
    // initializations of all character molds that specifically pertainin
    // to the allocation of memory and the creation of color codes within
    // this initialization process.
    UINT16 encodedPixelDataPixels;
    sMold curMold;
    sPixel* pDecodedImage;
    for (UINT moldId = 0; moldId < CHARACTER_VARIETY; moldId++) {        
        FILE* pFile = fopen(dirMold[moldId], "rb");
        if (pFile == NULL) {
            panic("Mold file is missing in " DIR_CHARACTER ".");
            return ERROR_FILE_NOT_FOUND;
        }
        
        // The command below initializes the collision, maximum horizontal
        // speed, and the number of distinct frames that the mold features.
        fread(&curMold.moldInfo, sizeof(sMold), 1, pFile);
        fclose(pFile);
        
        encodedPixelDataPixels = curMold.collision.width
            * curMold.collision.height * curMold.frames;
        
        pFile = fopen(dirPixelData[moldId], "rb");
        if (pFile == NULL) {
            panic("Character graphic data was not found in " 
                DIR_CHARACTER ".");
            return ERROR_FILE_NOT_FOUND;
        }
        
        switch(fReadImage(
                encodedPixelDataPixels,
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
        
        pDecodedImage = malloc(encodedPixelDataPixels * BYTES_PER_PIXEL);
        
        decodePixelDataFeaturing(
            *pColors,
            *pColorCodeBits,
            *pEncodedPixelDataBytes,
            *ppPalette,
            *ppColorCodeToPixelMapping,
            *ppEncodedPixelData,
            pDecodedImage);
        
        // The command below associates the current character mold's
        // provides a value to the 
        curMold.pPixelData = pDecodedImage;
        // Since the variable describing the amount of bits per color code is
        // computed by incrementing, it must be reset back to zero for the next
        // iteration.
        *pColorCodeBits = 0;
        // The finalized mold can now be placed into the array reserved for
        // containing all molds created in this program.
        gCharacterMolds[moldId] = curMold;
    }
    // The temporary pointer to set the pixel data in the allocated memories
    // to each character mold must not be pointing to anything after being
    // used.
    pDecodedImage = NULL;
    return ERROR_SUCCESS;
}

__forceinline void freeCharactersMolds() {
    for (UINT8 i = 0; i < CHARACTER_VARIETY; i++) {
        free(gCharacterMolds[i].pPixelData);
    }
    return;
}