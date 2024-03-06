#pragma once

#include "prop_render.h"

/*
 * The code section below outline all functions used throughout this file.
 */

__forceinline void decodePixelDataFeaturing(
    const UINT8 colors, 
    const UINT8 colorCodeBits,
    const UINT32 encodedPixelDataBytes,
    const BYTE* const restrict pPaletteDataBuffer, 
    sPixel* const restrict pCodeToPixelBuffer, 
    const BYTE* const restrict pPixelDataBuffer,
    sPixel* const restrict pDestination);

/*
 * The call to the function defined below decocdes raw pixel data given
 * arguments that are as follows:
 * - a color count
 * - the amount of bits needed to represent a color code
 * - the number of pixels in the image pixel data
 * - a pointer to hold raw palette data
 * - a pointer to a color code-to-pixel mapping
 * - a pointer to hold raw palette data
 * - a pointer to allocated memory where decoded data is written to. 
 * This function does not manipulate any memory or its allocation.
 */

__forceinline void decodePixelDataFeaturing(
        const UINT8 colors,
        const UINT8 colorCodeBits,
        const UINT32 encodedPixelDataBytes, 
        const BYTE* const restrict pPaletteDataBuffer, 
        sPixel* const restrict pCodeToPixelBuffer,
        const BYTE* const restrict pPixelDataBuffer,
        sPixel* const restrict pDestination) {
    
    UINT8 extracted8bitPixel;
    for (UINT8 i = 0; i < colors; i++) {
        pCodeToPixelBuffer[i].alpha = 0;
        // The command below saves the next byte, which certainly
        // is an entire 8-bit color, by assigning it to a variable.
        extracted8bitPixel = pPaletteDataBuffer[i];
        pCodeToPixelBuffer[i].blue = (extracted8bitPixel & 0x03) << 6;
        extracted8bitPixel >>= 2;
        pCodeToPixelBuffer[i].green = (extracted8bitPixel & 0x07) << 5;
        extracted8bitPixel >>= 3;
        pCodeToPixelBuffer[i].red = extracted8bitPixel << 5;            
    }
    
    union {
        struct {
            BYTE low;
            BYTE high;
        };
        UINT16 whole;
    } parsedBits = {0};
    UINT8 bitsToRead = sizeof(pPixelDataBuffer[0]) * 8;
    UINT8 colorCodeBitmask = (1 << colorCodeBits) - 1;
    UINT8 bitOffset = colorCodeBits;
    UINT8 bitsToReadForNextIter;
    // The maximum size of a ".bci" formatted image is unknown. This format
    // lacks pixel width and height.
    UINT32 pixelsWritten = 0;
    for (UINT32 readBytes = 0; 
            readBytes < encodedPixelDataBytes; 
            readBytes++) {
        parsedBits.low = pPixelDataBuffer[readBytes];
        for (; bitOffset <= bitsToRead; bitOffset += colorCodeBits) {
            pDestination[pixelsWritten] = pCodeToPixelBuffer[
                (parsedBits.whole >> (bitsToRead - bitOffset))
                & colorCodeBitmask];
            pixelsWritten++;
            
        }
        bitsToReadForNextIter = bitsToRead % colorCodeBits;
        
        // Consider only the first eight bits for iterpretation in the
        // next iteration, and not any other.
        parsedBits.high = 0;
        // The amount of bits to read for the next iteration is intended
        // to be the eight bits found in the byte alongside any bits not
        // read in the last parsing, which may only be fully completed in
        // the next immediate iteration.
        bitsToRead = 8 + bitsToReadForNextIter;
        // The left shift below places any remainding bits to be
        // parsed outside the first eight bits for the next iteration
        // of the byte translation to pixel data to interpret. The
        // amount of shifts required for the unparsed bits to be
        // interpreted in the next iteration is equal to the difference
        // between the number of bits in a byte, the amount of bits added
        // for every iteration, and the current bit offset.
        UINT8 bitmaskForIter;
        for (bitmaskForIter = 0; bitsToReadForNextIter > 0; 
                bitsToReadForNextIter--) {
            bitmaskForIter <<= 1;
            bitmaskForIter |= 1;
        }
        parsedBits.high  = parsedBits.low & bitmaskForIter;
        // The bit offset must be reset to the color code bit length 
        // since the bits in the next series of bits to parse 
        // through must be read starting at the ones of the lowest 
        // address.
        bitOffset = colorCodeBits;
    }
    return;
}