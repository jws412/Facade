#pragma once

/*
 * The set of function prototypes delimited by this block comment and the
 * one following it describe all function declarations regarding reading
 * image data.
 */

__forceinline LRESULT fReadImage(
    const UINT32 encodedPixelDataPixels,
    FILE* const restrict pFile,
    UINT8* const restrict pCurColors,
    UINT8* const restrict pColorCodeBits,
    UINT32* const restrict pCurEncodedPixelDataBytes,
    BYTE* restrict * const restrict ppPalette,
    sPixel* restrict * const restrict ppColorCodeToPixelMapping,
    BYTE* restrict * const restrict ppEncodedPixelData);

/*
 * The "fReadImage" function reads image data of a file passed as an 
 * argument. This image features color number, palette, and pixel
 * data information. This information is read and saved to dereferenced
 * arguments passed to the function call. The references passed and
 * potentially derefenced to reflect changes are as follows:
 * - Color number
 * - Number of bytes in encoded pixel data
 * - Address to the memory allocated for image palette data
 * - Address to the memory allocated for color code-to-pixel mapping
 *   derived from palette data
 * - Address to the memory allocated for encoded pixel data.
 * The function call requires the amount of pixels present in the pixel data.
 * This value must be passed as the first argument, preceding the file 
 * pointer argument.
 */

UINT8 maxColors = 0;
UINT32 maxEncodedPixelDataBytes = 0;

__forceinline LRESULT fReadImage(
        const UINT32 encodedPixelDataPixels,
        FILE* const restrict pFile,
        UINT8* const restrict pCurColors,
        UINT8* const restrict pColorCodeBits,
        UINT32* const restrict pCurEncodedPixelDataBytes,
        BYTE* restrict * const restrict ppPalette,
        sPixel* restrict * const restrict ppColorCodeToPixelMapping,
        BYTE* restrict * const restrict ppEncodedPixelData) {

    UINT8 colors = fgetc(pFile);
    if (colors > maxColors) {
        maxColors = colors;
        *pCurColors = colors;
        free(*ppPalette);
        // Every color in a palette header uses one byte.
        *ppPalette = malloc(colors);
        if (*ppPalette == NULL) {
            panic("Memory allocation for raw character palette data \
                failed.");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        free(*ppColorCodeToPixelMapping);
        *ppColorCodeToPixelMapping = malloc(colors * sizeof(sPixel));
        if (*ppColorCodeToPixelMapping == NULL) {
            panic("Memory allocation of character palette pixel map \
                failed.");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    
    fread(*ppPalette, colors, sizeof((*ppPalette)[0]), pFile);
    
    // At this point, the "colors" variable becomes unused. Its modification
    // does not affect future behavior in the scope of this function.
    UINT8 colorCodeBits = 0;
    --colors;
    do {
        colorCodeBits++;
    } while (colors >>= 1);
    
    *pColorCodeBits = colorCodeBits;
    UINT32 encodedPixelDataBits = encodedPixelDataPixels * colorCodeBits;
    UINT32 encodedPixelDataBytes = (encodedPixelDataBits + 7) / 8;
    *pCurEncodedPixelDataBytes = encodedPixelDataBytes;
    
    if (encodedPixelDataBytes > maxEncodedPixelDataBytes) {
        maxEncodedPixelDataBytes = encodedPixelDataBytes;
        free(*ppEncodedPixelData);
        *ppEncodedPixelData = malloc(encodedPixelDataBytes);
        if (*ppEncodedPixelData == NULL) {
            panic("Memory allocation of raw character graphics data \
                failed.");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    fread(
        *ppEncodedPixelData,
        encodedPixelDataBytes,
        sizeof((*ppEncodedPixelData)[0]),
        pFile);
    
    return ERROR_SUCCESS;
}
