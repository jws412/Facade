// Linkers: user32, gdi32, winmm
// This code is designed to be compiled with GCC.

#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#include "coordinator.h"
#include "management_tile.h"
#include "logic.h"
#include "management_background.h"
#include "prop_dir.h"
#include "prop_render.h"
#include "managment_level.h"
#include "management_character.h"
#include "management_gen.h"
#include "render_character.h"

/*
 * This section establishes data structures used throughout this file, and no
 * other program.
 */

typedef struct {
    UINT8 processHandleCount;
    UINT8 cpuPercent;
    USHORT ramKb;
    USHORT pagefileKb;
    USHORT fps;
} sPerformanceStatistics;

/*
 * This section establishes and outlines function symbols used thoughout 
 * this program.
 */

__forceinline LRESULT spawnWindow(HINSTANCE instance);

__forceinline LRESULT mainWndProc(
    HWND handle,
    UINT32 id,
    WPARAM primary,
    LPARAM secondary);

__forceinline LRESULT initBackbuffer();

__forceinline void drawFrame(
    const HDC destinationDc, 
    const HDC sourceDc,
    const sPerformanceStatistics ps,
    const sPixel pixelstringArr[const static BACKBUFFER_HEIGHT 
    * BACKBUFFER_WIDTH]);

__forceinline LRESULT cleanup();

/* 
 * The section below defines variables used be all main procedures in the
 * application's game update-rendering loop, initialization procedures,
 * and cleanup procedures. 
 */

HWND gWindowHandle;

/*
 * The function below is the entry point to the application. It performs all
 * necessary initialization procedures to generate a window on which renders
 * the application's asthetics.
 */

INT WINAPI WinMain(
    const HINSTANCE instance, 
    __attribute__ ((unused)) HINSTANCE prevInstance, 
    __attribute__ ((unused)) PSTR cmdLine,
    __attribute__ ((unused)) INT cmdShow) {    
    // Allocation of memory to string buffers for messages to be rendered on
    // the application.
    for (UINT8 i = 0; i < MAX_DEBUG_MESSAGE_NUMBER; i++) {
        gRenderInfo.pMessages[i] = malloc(MAX_DEBUG_MESSAGE_SIZE);
    }
    // The declaration and definition of the pointer variable 
    // "pixelstringbackgroundArr" holds pixel data. This data 
    // corresponds to the background only used in the rendering 
    // protocol.
    sPixel pixelstringbackgroundArr[BACKBUFFER_HEIGHT * BACKBUFFER_WIDTH];
    // The commands below initialize all characters to conventionally "null"
    // characters except for the first character, which must be the player.
    gPlayer.id = player;
    // If any initialization procedure fails, the program terminates
    // immediately. This fail can be caused by the existence of a duplicate
    // instance of the application, a behavior caused by the creation of this
    // application's mutext in the function call immediately below. This mutex
    // need not to be saved, since it is not needed for any future reference.
    CreateMutex(NULL, FALSE, MUTEX_TITLE);
    if (GetLastError() == ERROR_ALREADY_EXISTS
            || spawnWindow(instance) != ERROR_SUCCESS
            || initBackbuffer() != ERROR_SUCCESS
            || initLevel() != ERROR_SUCCESS
            || initActors() != ERROR_SUCCESS) {
        return ERROR_SUCCESS;
    }
    
    {
        UINT8 colors = 0;
        UINT8 colorCodeBits;
        UINT32 encodedPixelDataBytes = 0;
        
        // One color in the palette header is represented using one byte.
        BYTE* pPalette = malloc(colors);
        sPixel* pColorCodeToPixelMapping = malloc(colors * sizeof(sPixel));
        BYTE* pEncodedPixelData = malloc(encodedPixelDataBytes);
        
        LRESULT lastError = initBackground(pixelstringbackgroundArr,
            sizeof(pixelstringbackgroundArr) 
            / sizeof(pixelstringbackgroundArr[0]),
            &colors,
            &colorCodeBits,
            &encodedPixelDataBytes,
            &pPalette,
            &pColorCodeToPixelMapping,
            &pEncodedPixelData);
        if (lastError != ERROR_SUCCESS) {
            return lastError;
        }
        
        lastError = initCharacterMolds(
            &colors,
            &colorCodeBits,
            &encodedPixelDataBytes,
            &pPalette,
            &pColorCodeToPixelMapping,
            &pEncodedPixelData);
        if (lastError != ERROR_SUCCESS) {
            return lastError;
        }
        
        lastError = initTilePixelData(
            &colors,
            &colorCodeBits,
            &encodedPixelDataBytes,
            &pPalette,
            &pColorCodeToPixelMapping,
            &pEncodedPixelData);
        if (lastError != ERROR_SUCCESS) {
            return lastError;
        }
        
        free(pColorCodeToPixelMapping);
        free(pPalette);
        free(pEncodedPixelData);
    }
    
    // Variable used to store the handle to the process in which this program
    // is executing in. This handle does not need to be subjected to a
    // closing or termination operation. This handle is used to retrieve
    // performance statistics about the process in execution containing the
    // threads that this application uses.
    const HANDLE currentProcessHandle = GetCurrentProcess();
    // The priority level of the current process should be high. This process
    // is time-critical because the update frequency must be as constant as
    // possible.
    if (SetPriorityClass(currentProcessHandle, HIGH_PRIORITY_CLASS) == 0) {
        panic("Modification of process priority level failed.");
    }
    // Variables used to obtain device contexts required for rendering the
    // backbuffer. The destination device context is the window's pixels
    // that are displayed to the monitor if this windows is in view, while
    // the source device context is the backbuffer's pixel data.
    const HDC destinationDc = GetDC(gWindowHandle);
    const HDC sourceDc = CreateCompatibleDC(destinationDc);
    SelectObject(sourceDc, gBackbuffer.bitmapHandle);
    
    // Select the custom font into the device context of the newly created
    // window.
    const HFONT debugFontHandle = CreateFont(
        DEBUG_CHAR_HEIGHT,
        DEBUG_CHAR_WIDTH,
        0,
        0,
        FW_REGULAR, // font weight
        FALSE, // italics
        FALSE, // underlined
        FALSE, // strikethrough
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY,
        FF_MODERN | DEFAULT_PITCH,
        NULL); // Font name
    
    SelectObject(
        sourceDc,
        debugFontHandle
    );
    
    // Variables used to measure timing statistics.
    UINT64 ticksStart, ticksEnd;
    UINT64 frequency;
    UINT64 ticksSample;
    
    // Variables used for averaging time statistics.
    UINT32 iterationTimeSum;
    UINT32 iterationTally = 0;
    
    // Variables used to measure CPU usage statistics.
    SYSTEM_INFO systemInfo;
    INT64 userCPUTimeCurrent = 0;
    INT64 userCPUTimePrevious = 0;
    INT64 kernelCPUTimeCurrent = 0;
    INT64 kernelCPUTimePrevious = 0;
    GetSystemInfo(&systemInfo);

    // Variable used to store performance and resource usage metrics.
    sPerformanceStatistics ps = {0};
    
    // Variables used for the logic governing debug information display 
    // behavior.
    BOOLEAN keyIsPressed;
    BOOLEAN keyWasPressed = FALSE;
    
    QueryPerformanceFrequency((LARGE_INTEGER*) &frequency);
    QueryPerformanceCounter((LARGE_INTEGER*) &ticksStart);
    QueryPerformanceCounter((LARGE_INTEGER*) &ticksSample);
    
    // Increase accuracy of the Windows timer resolution in order make the
    // application's thread sleep for more precise intervals of time when
    // calling the "Sleep" function.
    if (timeBeginPeriod(MINIMUM_TIME_RESOLUTION) == TIMERR_NOCANDO) {
        panic("Denied change of the Windows timer resolution.");
    }
    
    MSG message;
    
    for (;;) {
        if (PeekMessage(&message, gWindowHandle, 0, 0,
            PM_NOREMOVE)) {            
            
            // Checking for special control inputs.
            if (GetAsyncKeyState(VK_CONTROL)){
                if (message.wParam == 'W') {
                    break;
                } else if (message.wParam == 'Z') {
                    memset(
                        &gRenderInfo.messageSizes, 
                        0x00, 
                        sizeof(gRenderInfo.messageSizes[0]) 
                            * MAX_DEBUG_MESSAGE_NUMBER); 
                }

                keyIsPressed = message.wParam == 'C'
                    && message.message == WM_KEYDOWN;
                if (keyIsPressed && !keyWasPressed) {
                    gIsDebug = !gIsDebug;
                }
                keyWasPressed = keyIsPressed;
            } // End checks for inputs controlling debug settings.

            if (GetMessage(&message, NULL, 0, 0) <= 0) {
                break;
            }
            DispatchMessage(&message);
        }
        logic(message.message == WM_ACTIVATE || message.wParam != 0);
        drawFrame(
            destinationDc,
            sourceDc, 
            ps,
            pixelstringbackgroundArr);

        iterationTally++;

        // The loop below intends to ellapse a time as close as possible to
        // the target microsecond amount per frame.        
        do {
            QueryPerformanceCounter((LARGE_INTEGER*) &ticksEnd);
            iterationTimeSum = (LONGLONG) (ticksEnd - ticksStart)
                * 1000000 / frequency;
            if (iterationTimeSum < MICROSEC_PER_UPDATE_SLEEP) {
                Sleep((MICROSEC_PER_UPDATE_SLEEP - iterationTimeSum) / 1000);
            }
        } while (iterationTimeSum <= MICROSEC_PER_UPDATE_LOGIC);
        
        // Recalibrate the frequency
        QueryPerformanceFrequency((LARGE_INTEGER*) &frequency);
        // Update the start tick counter
        QueryPerformanceCounter((LARGE_INTEGER*) &ticksStart);
        
        // Calculate average iterations per second, if necessary.
        if (iterationTally == UPDATE_SAMPLE_SIZE) {
            QueryPerformanceCounter((LARGE_INTEGER*) &ticksEnd);
            
            iterationTimeSum = (LONGLONG) (ticksEnd - ticksSample)
                * 1000000 / frequency;
            
            // The addition of half of the time period rounds the FPS.
            ps.fps = (1000000 * UPDATE_SAMPLE_SIZE + iterationTimeSum / 2) 
                / iterationTimeSum;
            
            // The command below is needed to save the number of handles that
            // the process in which executes this application is featuring.
            // The retrieval of this handle number is saved to a buffer whose
            // size matches that of the pointer's datum required in the call
            // of the "GetProcessHandleCount" function.
            DWORD bufferHandleCount;
            GetProcessHandleCount(
                currentProcessHandle, 
                &bufferHandleCount);
            ps.processHandleCount = (BYTE) bufferHandleCount;
            
            // The commands below saves the amount of memory, regardless of
            // type, that the process in which this application runs in 
            // is allocated to.
            PROCESS_MEMORY_COUNTERS tempMemCounter;
            GetProcessMemoryInfo(
                currentProcessHandle, 
                &tempMemCounter,
                sizeof(PROCESS_MEMORY_COUNTERS));

            ps.ramKb = (USHORT) (tempMemCounter.WorkingSetSize / 1000);
            ps.pagefileKb = (USHORT) (tempMemCounter.PagefileUsage / 1000);
            
            // The commands below determine the CPU usage times for every
            // complete sampling. The dummy variable declared and defined
            // below is used to satisfy the input requirements of the call
            // to the "GetProcessTimes" function.
            FILETIME dummyTime;
            GetProcessTimes(
                currentProcessHandle,
                &dummyTime,
                &dummyTime,
                (FILETIME*) &kernelCPUTimeCurrent,
                (FILETIME*) &userCPUTimeCurrent);

            ps.cpuPercent = ((kernelCPUTimeCurrent + userCPUTimeCurrent)
                - (kernelCPUTimePrevious + userCPUTimePrevious))
                / (systemInfo.dwNumberOfProcessors * iterationTimeSum / 10);
            
            // The assignments below reinstate the original values of the
            // metrics used to determine iterations per second and frames
            // per second.
            iterationTally = 0;
            // Update both CPU usage times by the kernel and the user to their
            // current usage times.
            kernelCPUTimePrevious = kernelCPUTimeCurrent;
            userCPUTimePrevious = userCPUTimeCurrent;
            // Allow another full set of samples to be averaged.
            QueryPerformanceCounter((LARGE_INTEGER*) &ticksSample);
        }
    }
    
    /*
     * The code section below releases or deletes all resources used by the
     * application before terminating it.
     */
    
    DeleteDC(sourceDc);
    ReleaseDC(gWindowHandle, destinationDc);
    DeleteObject(debugFontHandle);
    
    LRESULT lastCode;
    if ((lastCode = cleanup()) != ERROR_SUCCESS) {
        return lastCode;
    }
    return ERROR_SUCCESS;
}

/*
 * This function creates a window with its main procedure function set to the
 * "mainWndProc" function. This window bears minimize, maximize, and close
 * buttons, and a title defined in the "prop.h" header.
 */

__forceinline LRESULT spawnWindow(HINSTANCE handleInstance) {

    WNDCLASSEX windowClass;

    // Initialization of attributes of the window class to register.
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = mainWndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = handleInstance;
    windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    windowClass.hIconSm = LoadImage(
        handleInstance,
        MAKEINTRESOURCE(5),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CXSMICON),
        LR_DEFAULTCOLOR);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = CreateSolidBrush(0xFF00FF);
    windowClass.lpszMenuName = INNER_TITLE;
    windowClass.lpszClassName = CLASS_TITLE;

    if (!RegisterClassEx(&windowClass)) {
        panic("Window registration failed.");
        return ERROR_INVALID_DATA;
    }
    
    // Dummy values for monitor info struct
    RECT rectangleDummy = {0};
    // Determination of monitor information.
    MONITORINFO monitorInfo = {
        sizeof(MONITORINFO), 
        rectangleDummy, 
        rectangleDummy,
        0};
    if (GetMonitorInfo(MonitorFromWindow(gWindowHandle, 
            MONITOR_DEFAULTTOPRIMARY),
            &monitorInfo) == 0) {
        panic("Monitor information retrieval was unsuccessful.");
    }
    
    RECT rectBuffer = monitorInfo.rcMonitor;
    const USHORT monitorWidth = (USHORT) rectBuffer.right - rectBuffer.left;
    const USHORT monitorHeight = (USHORT) rectBuffer.bottom - rectBuffer.top;
    
    // Save the dimensions of the window for later retrieval.
    gWindowDimensions = (sDimensions) {
        monitorWidth,
        monitorHeight,
    };
    
    // Creation of the window that this program uses, assuming that its
    // registration was successful. The "WS_VISIBLE" window style ensures
    // that the created window appears, and the "WS_POPUP" window style
    // ensures that the created window is borderless.
    gWindowHandle = CreateWindow(
        CLASS_TITLE,
        DISPLAY_TITLE,
        WS_VISIBLE | WS_POPUP,
        rectBuffer.left,
        rectBuffer.top,
        monitorWidth,
        monitorHeight,
        NULL,
        NULL,
        handleInstance,
        NULL);
    
    if (gWindowHandle == NULL) {
        panic("Window creation failed.");
        return ERROR_INVALID_DATA;
    }
    
    return ERROR_SUCCESS;
}

LRESULT initBackbuffer() {
    // The "BITMAPINFO" struct is necessary when using the "CreateDIBSection"
    // function as opposed to the "CreateBitmap" function.
    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
    bitmapInfo.bmiHeader.biWidth = BACKBUFFER_WIDTH;
    // bitmapInfo.bmiHeader.biWidth = gWindowDimensions.width;
    bitmapInfo.bmiHeader.biHeight = BACKBUFFER_HEIGHT;
    // bitmapInfo.bmiHeader.biHeight = gWindowDimensions.height;
    bitmapInfo.bmiHeader.biBitCount = BITMAP_BPP;
    // The symbol "BI_RGB" indicates that no compression procedure is
    // employed.
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biPlanes = 1;
    
    // Create the backbuffer to be stretched onto the application windows's
    // pixels.
    gBackbuffer.bitmapHandle = CreateDIBSection(
        NULL,
        &bitmapInfo,
        DIB_RGB_COLORS,
        &gBackbuffer.pPixelData,
        NULL,
        0);
    // The "CreateDIBSection" returns a NULL pointer if it fails to create a 
    // device independent bitmap.
    if (gBackbuffer.bitmapHandle == NULL) {
        panic("Backbuffer memory allocation failed.");
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    gBackbuffer.memorySize = (BACKBUFFER_WIDTH * 
        BACKBUFFER_HEIGHT * BYTES_PER_PIXEL);
    return ERROR_SUCCESS;
}

/*
 * This function is the main procedure for the window generated by the
 * "spawnWindow" function.
 */

LRESULT mainWndProc(HWND handle, UINT32 messageId, WPARAM primary,
    LPARAM secondary) {
    switch(messageId) {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        case WM_ACTIVATE:
            ShowCursor(FALSE);
            break;
        default:
            return DefWindowProc(handle, messageId, primary, secondary);
    }
    return ERROR_SUCCESS;
}

__forceinline void drawFrame(
        const HDC destinationDc,
        const HDC sourceDc,
        const sPerformanceStatistics ps,
        const sPixel pixelstringArr[const static BACKBUFFER_HEIGHT 
        * BACKBUFFER_WIDTH]) {    
    
    /*
     * The first subprocess performed in the rendering protocol renders the
     * the background's pixel data. It executes first for all other graphics
     * to render on it.
     */
    
    sPixel* const pBackbuffer = gBackbuffer.pPixelData;
    
    memcpy(pBackbuffer, pixelstringArr, 
        BACKBUFFER_HEIGHT * BACKBUFFER_WIDTH * sizeof(pixelstringArr[0]));
    
    /*
     * This processing section of this function concerns the player
     * character's appearance on the viewport. The player character remains
     * in the center of the screen when scrolling applies.
     */
    
    // The variables below are used to determine the tiles to render
    // on the viewport. Tile indices from the tilemap are used.
    UINT16 leftRenderBoundaryTileIndex;
    UINT16 rightRenderBoundaryTileIndex;
    
    sPosition screenPos = gPlayer.pos;
    const UINT8 playerWidth = gCharacterMolds[gPlayer.id].collision.width;
    enum {
        SCREEN_LEFT,
        SCREEN_SCROLLING,
        SCREEN_RIGHT,
    } screenState;
    
    if (screenPos.x > (gLevel.width - (BACKBUFFER_WIDTH - playerWidth) 
                / 2) - TILE_SIZE) {
        screenPos.x = (screenPos.x - gLevel.width + BACKBUFFER_WIDTH 
            - playerWidth) + TILE_SIZE;
        leftRenderBoundaryTileIndex = (gLevel.width - BACKBUFFER_WIDTH) 
            / TILE_SIZE * COLUMN_SIZE;
        rightRenderBoundaryTileIndex = gLevel.width
            / TILE_SIZE * COLUMN_SIZE;
        screenState = SCREEN_RIGHT;
    } else if (screenPos.x >= (BACKBUFFER_WIDTH - playerWidth) / 2) {
        screenPos.x = (BACKBUFFER_WIDTH - playerWidth) / 2;
        leftRenderBoundaryTileIndex = (gPlayer.pos.x - BACKBUFFER_WIDTH / 2
            + playerWidth / 2) / TILE_SIZE * COLUMN_SIZE;
        rightRenderBoundaryTileIndex = (gPlayer.pos.x + BACKBUFFER_WIDTH / 2
            + playerWidth / 2 + (TILE_SIZE - 1)) / TILE_SIZE * COLUMN_SIZE;
        screenState = SCREEN_SCROLLING;
    } else {
        leftRenderBoundaryTileIndex = 0;
        rightRenderBoundaryTileIndex = BACKBUFFER_WIDTH 
            / TILE_SIZE * COLUMN_SIZE;
        screenState = SCREEN_LEFT;
    }
    
    // A unique call to the "renderCharacter" function is done for the
    // player. The player is always fully on-screen.
    renderCharacter(
        gCharacterMolds[player],
        gPlayer.animState,
        screenPos,
        playerWidth,
        0);
    
    /*
     * The third subprocess in this function displays all NPC graphics.
     * Any NPC that is on the player's viewport becomes visible and
     * able to update. NPC sprites are in function of the player character's
     * screen and level position.
     */
    
    const UINT16 cameraLeftPosX = gPlayer.pos.x - screenPos.x;
    const UINT16 cameraRightPosX = cameraLeftPosX + BACKBUFFER_WIDTH;
    
    UINT16 characterLeftPosX;
    UINT16 characterRightPosX;
    UINT8 startCharacterPixelDataColumn;
    UINT8 endCharacterPixelDataColumn;
    
    sCharacter characterInstance;
    UINT8 characterId;
    INT8 characterAnimState;
    sMold characterMold;
    UINT8 characterWidth;
    
    
    for (UINT8 instanceId = 0; 
            instanceId < gMutableCharacterArray.instances; 
            instanceId++) {
        
        characterInstance = gMutableCharacterArray.pCharacter[instanceId];
        characterId = characterInstance.id;
        if (characterId == idNull) {
            continue;
        }
        
        characterLeftPosX = characterInstance.pos.x;
        characterMold = gCharacterMolds[characterId];
        characterWidth = characterMold.collision.width;
        characterRightPosX = characterLeftPosX + characterWidth;
        characterAnimState = characterInstance.animState;
        if (characterRightPosX > cameraLeftPosX
                && characterLeftPosX < cameraRightPosX) {
            
            // The first "if-else" statements here bounds the sprite's 
            // right-most pixel column to render.
            if (characterRightPosX > cameraRightPosX) {
                startCharacterPixelDataColumn = cameraRightPosX 
                    - characterLeftPosX;
            } else {
                startCharacterPixelDataColumn = characterWidth;
            }
            // The second "if-else" statements here finds the sprite's first
            // pixel column to render.
            if (characterLeftPosX < cameraLeftPosX) {
                endCharacterPixelDataColumn = cameraLeftPosX
                    - characterLeftPosX;
                // Overlapping character and camera X positions renders
                // at a screen X position of zero.
                characterLeftPosX = cameraLeftPosX;
            } else {
                endCharacterPixelDataColumn = 0;
            }
            renderCharacter(
                characterMold,
                characterAnimState,
                (sPosition) {
                    characterLeftPosX - cameraLeftPosX,
                    characterInstance.pos.y},
                startCharacterPixelDataColumn,
                endCharacterPixelDataColumn);
        } else {
            switch(characterId) {
                
                case bug:
                
                switch(characterAnimState) {
                    
                    case 2: case -3:
                    // Setting the id of this character to null ceases
                    // all of its behavior. This case is triggered when
                    // this character is in its defeat state.
                    gMutableCharacterArray.pCharacter[instanceId].id = 
                        idNull;
                    break;
                    
                    default:
                    // This case suspends the behavior of the character
                    // until it appears on-screen again.
                    gMutableCharacterArray.pCharacter[instanceId]
                        .animState = characterAnimState >= 0 ?  
                        ANIM_OFFSCREEN : ~ANIM_OFFSCREEN;
                    break;
                }
                break;
                
                default:
                // This point is reached if a character's id is unknown.
                // The character carries out its behavior whether 
                // on-screen or off-screen.
                break;
            }
        }
    }
    
    /*
     * The fourth rendering procedure of this function pertains to all
     * tiles in the viewport. Pixels of the transparent color are winnowed
     * out during rendering.
     */
    
    const sPixel* pTile;
    const UINT16 tileScreenNegatedOffsetX = screenState == SCREEN_SCROLLING ?
        - ((gPlayer.pos.x + playerWidth / 2) % playerWidth) : 0;
    // The algorithm renders the left-most column of tiles first. This
    // operation executes regardless whether these tiles are cutoff or
    // not.
    const UINT16 leftColumnIndexEnd = leftRenderBoundaryTileIndex 
        + COLUMN_SIZE;
    sPixel pixelBuffer;
    const UINT8 tileStartX = -(INT8) tileScreenNegatedOffsetX;
    // The algorithm uses the "backbufferWidths" variable for determining
    // the next row to render. This variable is a multiple of the backbuffer
    // width for the next two parts. The algorithm's first part initializes 
    // this value to the pixel offset however. This first step is crutial to
    // shift the extracted tile pixels. This shift ensures that these pixels
    // render starting from the backbuffer's left border.
    UINT32 backbufferWidths = -tileStartX;
    // The process uses the "tileWidths" variable to render pixels in
    // subsequent rows from tiles.
    UINT16 tileWidths = 0;
    for (UINT16 tileIndex = leftRenderBoundaryTileIndex;
            tileIndex < leftColumnIndexEnd; 
            tileIndex++) {
        pTile = gTileAtlas[gLevel.pTilemap[tileIndex]];
        for (tileWidths = 0; 
                tileWidths < (TILE_SIZE * TILE_SIZE); 
                tileWidths += TILE_SIZE,
                backbufferWidths += BACKBUFFER_WIDTH) {
            for (UINT8 pixels = tileStartX; pixels < TILE_SIZE; pixels++) {
                pixelBuffer = pTile[tileWidths + pixels];
                if (pixelBuffer.whole == COLOR_TRANSPARENT) {
                    continue;
                }
                pBackbuffer[backbufferWidths + pixels] = pixelBuffer;
            }
        }
    }
    // The second part of this process may render all fully on-screen tiles. 
    // This result occurs when tiles are not offset. That is, the tiles form 
    // a grid featuring no seams from cutoff tiles.
    const UINT16 rightColumnIndexEnd = rightRenderBoundaryTileIndex 
        - (tileScreenNegatedOffsetX > 0 ? COLUMN_SIZE : 0);
    UINT32 tileScreenBottomLeftOffset = (UINT16) (tileScreenNegatedOffsetX
        + TILE_SIZE);
    
    for (UINT16 tileIndex = leftColumnIndexEnd;
            tileIndex < rightColumnIndexEnd;
            tileIndex++) {
        pTile = gTileAtlas[gLevel.pTilemap[tileIndex]];
        for (tileWidths = 0,
                backbufferWidths = 0; 
                tileWidths < (TILE_SIZE * TILE_SIZE); 
                tileWidths += TILE_SIZE,
                backbufferWidths += BACKBUFFER_WIDTH) {
            for (UINT8 pixels = 0; pixels < TILE_SIZE; pixels++) {
                pixelBuffer = pTile[tileWidths + pixels];
                if (pixelBuffer.whole == COLOR_TRANSPARENT) {
                    continue;
                }
                pBackbuffer[backbufferWidths + pixels 
                    + tileScreenBottomLeftOffset] = pixelBuffer;
            }
        }
        // The second routine determines the row to render after
        // rendering the tile below it.
        tileScreenBottomLeftOffset += BACKBUFFER_WIDTH * TILE_SIZE;
        // This process also determines the next column to render after
        // rendering the entire row.
        if (tileScreenBottomLeftOffset 
                >= (TILE_SIZE * COLUMN_SIZE * BACKBUFFER_WIDTH)) {
            // This statement's code block performs a subtraction to
            // determine the next tile column. The process determines the 
            // number of pixels in the area reserved for tiles. It
            // subtracts this quantity from the current offset, also in
            // pixels. This block is expected to execute once the offset
            // exceeds the backbuffer. As such, the offset is brought back
            // within the bounds of the backbuffer. The process subsequently 
            // adds the number of pixels in a tile width. This procedure 
            // translates as the expression of the assignment operation in
            // this block.
            tileScreenBottomLeftOffset = tileScreenBottomLeftOffset -
                (TILE_SIZE * COLUMN_SIZE * BACKBUFFER_WIDTH - TILE_SIZE);
        }
    }
    
    // A final process renders the right-most tile column on the player's
    // viewport. These tiles may feature pixels cut off from the right.
    // The last tile x-coordinate is assumed to be the very last tile
    // column.
    const UINT8 endPixelColumn = (UINT8) -(UINT16) tileScreenNegatedOffsetX;
    // The second part leaves the "backbufferWidths" variable with a value
    // execeeding the backbuffer's height. It must be reset to zero.
    backbufferWidths = 0;
    tileScreenBottomLeftOffset = BACKBUFFER_WIDTH - endPixelColumn;
    for (UINT16 tileIndex = rightColumnIndexEnd;
            tileIndex < rightRenderBoundaryTileIndex;
            tileIndex++) {
        pTile = gTileAtlas[gLevel.pTilemap[tileIndex]];
        for (tileWidths = 0; 
                tileWidths < (TILE_SIZE * TILE_SIZE); 
                tileWidths += TILE_SIZE,
                backbufferWidths += BACKBUFFER_WIDTH) {
            for (UINT8 pixels = 0; pixels < endPixelColumn; pixels++) {
                pixelBuffer = pTile[tileWidths + pixels];
                if (pixelBuffer.whole == COLOR_TRANSPARENT) {
                    continue;
                }
                pBackbuffer[backbufferWidths + pixels 
                    + tileScreenBottomLeftOffset] = pixelBuffer;
            }
        }
    }
    
    /*
     * The final rendering subprocess shall display debug information. Listed
     * data are as follows:
     * - Memory resources used by the process
     * - Computational resources used
     * - The player character's coordinates
     * - Any debug message resulting from calls of the "debugPrintf"
     *   function.
     */
    
    if (gIsDebug) {
        // Render debug information, whose update rate depends on the
        // sample rate used to determine this FPS. All debug data is written
        // in the backbuffer.
        CHAR buffer[MAX_DEBUG_MESSAGE_SIZE];
        
        TextOut(sourceDc, 0, 0, buffer, 
            sprintf(buffer, "FPS: %i", ps.fps));
        
        TextOut(sourceDc, 0, DEBUG_CHAR_HEIGHT * 1, 
            buffer, sprintf(buffer, "CPU Usage: %i%%", ps.cpuPercent));
        
        TextOut(sourceDc, 0, DEBUG_CHAR_HEIGHT * 2, 
            buffer, sprintf(buffer, "RAM Usage: %iKB", ps.ramKb));
            
        TextOut(sourceDc, 0, DEBUG_CHAR_HEIGHT * 3, 
            buffer, sprintf(buffer, "Pagefile Usage: %iKB", ps.pagefileKb));
        
        TextOut(sourceDc, 0, DEBUG_CHAR_HEIGHT * 4, 
            buffer, sprintf(buffer, "Handle Count: %i", ps.processHandleCount));
            
        TextOut(sourceDc, 0, DEBUG_CHAR_HEIGHT * 5, 
            buffer, sprintf(buffer, "X/Y: %i %i", 
                gPlayer.pos.x, gPlayer.pos.y));
        
        // Render on the backbuffer any debug messages.
        const CHAR* pMessage;
        for (UINT8 i = 0; i < MAX_DEBUG_MESSAGE_NUMBER; i++) {
            pMessage = gRenderInfo.pMessages[i];
            if (pMessage != NULL) {
                TextOut(sourceDc,
                    0,
                    BACKBUFFER_HEIGHT
                        + ( (-i - 1) * DEBUG_CHAR_HEIGHT),
                    (const CHAR*) pMessage, 
                    gRenderInfo.messageSizes[i]);
            }
        }
    }
    // This function does not render a backbuffer to be stretched, but rather
    // one that is static in size.
    if (StretchBlt(
            destinationDc,
            0,
            0,
            gWindowDimensions.width,
            gWindowDimensions.height,
            sourceDc,
            0,
            0,
            BACKBUFFER_WIDTH,
            BACKBUFFER_HEIGHT,
            SRCCOPY) == 0) {
        panic("Backbuffer copy to window failed.");
    }
    return;
}

__forceinline LRESULT cleanup() {
    // Window objects cannot be deleted.
    if (DeleteObject(gBackbuffer.bitmapHandle) == 0) {
        panic("Handle to backbuffer pixel data was unsuccessfully deleted.");
        return ERROR_INVALID_PARAMETER;
    }
    // Free memory for debug string buffers.    
    for (UINT i = 0; i < MAX_DEBUG_MESSAGE_NUMBER; i++) {
        free(gRenderInfo.pMessages[i]);
    }
    
    freeTilemap();
    // Free memory pertaining to character molds.
    freeCharactersMolds();
    // Release memory pertaining to character instances.
    freeActors();
    // The timer resolution set used throughout this application is no longer
    // required and can be reinstated to its original value.
    timeEndPeriod(MINIMUM_TIME_RESOLUTION);
    return ERROR_SUCCESS;
}