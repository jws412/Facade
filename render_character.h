#pragma once

#include "coordinator.h"
#include "prop_render.h"

/*
 * Functions defined in this file take care of rendering character sprites on
 * the window backbuffer. They are not responsible for checking bounds of what
 * they are rendering however. That is, arguments can cause pixels to render
 * outside the backbuffer's allocation memory.
 */

__forceinline void renderCharacter(
        const sMold mold,
        const INT8 animState,
        const sPosition screenPos,
        const UINT8 stopColumn,
        const UINT8 leftShiftedColumns);

/*
 * The "renderCharacter" function renders a character sprite on the window
 * backbuffer. 
 */

__forceinline void renderCharacter(
        const sMold mold,
        const INT8 animState,
        const sPosition screenPos,
        const UINT8 stopColumn,
        const UINT8 leftShiftedColumns) {
    
    const UINT8 characterWidth = mold.collision.width;
    const UINT8 characterHeight = mold.collision.height;
    const BOOLEAN isMirrored = animState < 0;
    const INT8 framesToSubtract = isMirrored ? animState : ~animState;
    sPixel* const restrict pReferencePixel = (sPixel*) gBackbuffer.pPixelData
        + (screenPos.y * BACKBUFFER_WIDTH) + screenPos.x;
    const sPixel* const restrict pFramePixelData = mold.pPixelData
        + (characterWidth * characterHeight 
        * (mold.frames + framesToSubtract));
    const INT8 progressionStep = ((3 >> isMirrored) - 2);
    const UINT8 mirroredFactor = characterWidth * isMirrored - isMirrored;
    const UINT16 lastPixel = characterHeight * characterWidth;
    sPixel pixelBuffer;
    
    for (UINT16 spritePixelRows = 0,
            backbufferPixelRows = 0;
            spritePixelRows < lastPixel;
            spritePixelRows += characterWidth,
            backbufferPixelRows += BACKBUFFER_WIDTH) {
        for (UINT8 pixels = leftShiftedColumns;
                pixels < stopColumn;
                pixels++) {
            pixelBuffer = pFramePixelData[spritePixelRows
                + pixels * progressionStep + mirroredFactor];
            if (pixelBuffer.whole != COLOR_TRANSPARENT) {
                pReferencePixel[backbufferPixelRows + pixels
                    - leftShiftedColumns] = pixelBuffer;
            }
        }
    }
    return;
}