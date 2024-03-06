#pragma once

#include "coordinator.h"
#include "prop_dir.h"

/*
 * The functions declared in this document pertain to character generation
 * in levels. These functions use the "gCharacters" array to store enemy
 * information in the level. This information is found in generator files,
 * which can be opened as text files.
 */

__forceinline LRESULT initActors();

__forceinline void resetActors();

__forceinline void freeActors();

/*
 * The "initActors" function loads generation information about the first
 * level. This information is saved in the "gCharacters" array. This 
 * data is used by the logic update procedure of this application.
 */

// The "gInitialCharacterArray" struct stores all information about 
// initial states of characters in the level.
sCharacterArray gInitialCharacterArray;
UINT16 characterArrayBytes;

__forceinline LRESULT initActors() {
    #define CHUNK_SIZE 46
    #define READING_COMMENT 0x01
    #define READING_NUMBER 0x02
    #define SEARCHING_ACTORS 0x04
    #define SEARCHING_ID 0x08
    #define SEARCHING_X 0x10
    #define SEARCHING_Y 0x20
    
    FILE* pFile = fopen(DIR_FIRST_GEN, "rb");
    
    BYTE chunk[CHUNK_SIZE];
    UINT8 extractedChars;
    
    UINT16 numberBuffer = 0;
    BYTE flag = SEARCHING_ACTORS;
    UINT8 i;
    UINT8 characters = 0;
    UINT32 prevBrackets = 0;
    UINT32 curBrackets = 0;
    
    do {
        extractedChars = fread(chunk, sizeof(BYTE), sizeof(chunk), pFile);
        for (i = 0; i < extractedChars; i++) {
            if ((flag & READING_COMMENT) != 0) {
                flag |= READING_COMMENT;
                while (++i < (sizeof(chunk) - 1) && chunk[i + 1] != '\n');
                if (chunk[i + 1] == '\n') {
                    flag &= (BYTE) ~READING_COMMENT;
                    i++;
                } else {
                    continue;
                }
            }
            
            switch(chunk[i]) {
                case '#':
                
                flag |= READING_COMMENT;
                continue;
                
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':

                numberBuffer = numberBuffer * 10 + chunk[i] - '0';
                flag |= READING_NUMBER;
                break;
                
                case '{':
                
                // The fallthrough behavior of the bracket characters require
                // a particular incrementation system. An opening bracket
                // counts as two entries to accomodate the decrement
                // afterwards. The overall result is an incrementation of
                // the bracket count by one.
                curBrackets += 2;
                // Fallthrough
                case '}':
                
                prevBrackets = curBrackets--;
                if (prevBrackets < curBrackets) {
                    goto unmatchedBracket;
                }
                // Fallthrough
                case ' ': case '\r': case '\t': case '\n' :
                
                switch(flag & (SEARCHING_ACTORS | SEARCHING_ID | SEARCHING_X
                        | SEARCHING_Y | READING_NUMBER)) {
                    
                    case (SEARCHING_ACTORS | READING_NUMBER):
                    
                    gMutableCharacterArray.instances = numberBuffer;
                    gInitialCharacterArray.instances = numberBuffer;
                    characterArrayBytes = numberBuffer * sizeof(sCharacter);
                    // Allocation of memory for storage of mutable characters.
                    // That is, the characters the logic of the game intends
                    // to modify.
                    gMutableCharacterArray.pCharacter = malloc(
                        characterArrayBytes);
                    // Allocation of memory for storage of the initial states
                    // of characters. The game logics intends to not modify
                    // any of the characters it features.
                    gInitialCharacterArray.pCharacter = malloc(
                        characterArrayBytes);
                    if (gMutableCharacterArray.pCharacter == NULL) {
                        panic("Actor memoy allocation failed.");
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }
                    flag ^= (SEARCHING_ID | SEARCHING_ACTORS | READING_NUMBER);
                    break;
                    
                    case (SEARCHING_ID | READING_NUMBER):
                    
                    gMutableCharacterArray.pCharacter[characters].id = 
                        (UINT8) numberBuffer;
                    // By default, any character faces towards the left.
                    gMutableCharacterArray.pCharacter[characters].animState = 
                        (INT8) -1;
                    flag ^= (SEARCHING_X | SEARCHING_ID | READING_NUMBER);
                    break;
                    
                    case (SEARCHING_X | READING_NUMBER):
                    
                    gMutableCharacterArray.pCharacter[characters].pos.x = 
                        (UINT16) numberBuffer;
                    flag ^= (SEARCHING_Y | SEARCHING_X | READING_NUMBER);
                    break;
                    
                    case (SEARCHING_Y | READING_NUMBER):
                    
                    gMutableCharacterArray.pCharacter[characters++].pos.y = 
                        (UINT16) numberBuffer;
                    flag ^= (SEARCHING_ID | SEARCHING_Y | READING_NUMBER);
                    break;

                    default:
                    
                    flag &= (BYTE) ~READING_NUMBER;
                    break;
                }
                
                numberBuffer = 0;
                
                break;
                
                default:

                panic("Bad data in " DIR_FIRST_GEN ".");
                return ERROR_INVALID_DATA;
            }
        }
    } while (extractedChars == sizeof(chunk));
    fclose(pFile);
    
    if (curBrackets != 0) {
        unmatchedBracket:
        
        panic("Unmatched bracket in " DIR_FIRST_GEN ".");
        return ERROR_INVALID_DATA;
    }
    if (flag != SEARCHING_ID) {
        panic("Character member mismatch in " DIR_FIRST_GEN ".");
    }
    
    // Copies of all characters in their initial states are set in the
    // initial array. The game's logic uses this initial array to reset 
    // characters to their initial states.
    memcpy(gInitialCharacterArray.pCharacter, 
        gMutableCharacterArray.pCharacter, characterArrayBytes);
    
    return ERROR_SUCCESS;
}

/*
 * The "resetActors" function resets the attributes of all characters to
 * their initial, respective states.
 */

__forceinline void resetActors() {
    
    memcpy(gMutableCharacterArray.pCharacter, 
        gInitialCharacterArray.pCharacter, characterArrayBytes);
    
    return;
}

/*
 * The "freeActors" function deallocates all memory reserved for characters
 * of the currently loaded level.
 */

__forceinline void freeActors() {

    free(gMutableCharacterArray.pCharacter);
    free(gInitialCharacterArray.pCharacter);
    return;
}