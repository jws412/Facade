#pragma once

#include "coordinator.h"
#include "prop_character.h"
#include "management_gen.h"

#define isOverflowByAtMost(threshold, n) \
    n >= (~(((UINT64) -1) << (sizeof(n) * 8)) - (threshold * (threshold < 0 ? -1 : 1)))

/*
 * This code section describes all functions governing how character instances
 * behave.
 */

__forceinline void logic(const BOOLEAN isInFocus);

__forceinline void killPlayer();

/*
 * The following variable declarations below are used to create buffers to
 * save any past data used to calculate the next, immediate game update.
 */

// Variables reserved for horizontal movement and its associated player
// inputs.
BOOLEAN isInputRight;
UINT8 curMaxPlayerSpeedX;
INT8 directionVector;
INT8 subPos;

// Variables reserved for vertical movement and its associated player inputs.
BOOLEAN wasInputingJump = TRUE;
BOOLEAN isInputingJump;
BOOLEAN wasJumpNotReleased = FALSE;
BOOLEAN isPlayerGrounded = FALSE;
UINT8 jumpTimer = 0;

// The variable defined below governs the player character's animations.
UINT8 playerAnimationCounter = 0;

/*
 * The function below computes all logic of the game based on the data stored
 * in all the varibles only accessible to this part of the application
 * declared and intialized above.
 */

__forceinline void logic(const BOOLEAN isInFocus) {            
    
    /*
     * The code section below governs the manipulation of the player
     * character's horizontal and vertical velocities as a function
     * of the current player input and the one of the previous game
     * update.
     */
    
    // The player character direction scalar is positive one for a 
    // right-wards direction, negative one for a left-wards direction, 
    // and negative one for a zero velocity. (Credit to Sean Eron Anderson:
    // https://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign)
    
    directionVector = (gPlayer.velocity.x != 0 
        | (gPlayer.velocity.x >> (sizeof(gPlayer.velocity.x) * 8 - 1)));    
    
    curMaxPlayerSpeedX = (GetKeyState('X') & 0x80) ? 
        gCharacterMolds[player].maxSpeedX
        : gCharacterMolds[player].maxSpeedX / PLAYER_MIN_FULLSPEED_COEF;
    
    isInputRight = GetKeyState(VK_RIGHT) & 0x80 && isInFocus;
    if (isInputRight) {
        if (gPlayer.velocity.x < curMaxPlayerSpeedX) {
            gPlayer.velocity.x += PLAYER_ACCELERATION_NUMERATOR_X;
        } else if (gPlayer.velocity.x > curMaxPlayerSpeedX) {
            gPlayer.velocity.x -= PLAYER_ACCELERATION_NUMERATOR_X;
        }
    }
    if (GetKeyState(VK_LEFT) & 0x80 && isInFocus) {
        if (gPlayer.velocity.x > -curMaxPlayerSpeedX) {
            gPlayer.velocity.x -= PLAYER_ACCELERATION_NUMERATOR_X;
        } else if (gPlayer.velocity.x < -curMaxPlayerSpeedX) {
            gPlayer.velocity.x += PLAYER_ACCELERATION_NUMERATOR_X;
        }
    } else if (!isInputRight && isPlayerGrounded) {
        gPlayer.velocity.x -= directionVector * 
            PLAYER_ACCELERATION_NUMERATOR_X;
    }
    
    isInputingJump = GetKeyState(VK_SPACE) & 0x80 && isInFocus;
    if (wasJumpNotReleased) {
        if (isInputingJump && (jumpTimer < PLAYER_MAX_JUMP_HOLD_FRAMES)) {
            // The logic below triggers if the player inputted the jump
            // command while having also inputted it in the previous game
            // update and having not exceeding the maximum number of game
            // updates for which this command can be held.
            gPlayer.velocity.y = PLAYER_MAX_SPEED_Y;
            jumpTimer++;
            isPlayerGrounded = FALSE;
        } else if (wasInputingJump) {
            // The logic below executes if the player did activate the jump
            // command in the previous game update but is not inputting this
            // command on the current game update.
            wasJumpNotReleased = FALSE;
        }
    } else {
        // The logic below triggers if the player did not input a jump
        // command in the previous game update. This particular action
        // makes the player character enter a falling state such that
        // the vertical motion it applies on the character cannot be 
        // controlled by the player.
        if (isPlayerGrounded) {
            gPlayer.velocity.y = 0;
            jumpTimer = 0;
            if (!isInputingJump) {
                wasJumpNotReleased = TRUE;
            }
        } else if (gPlayer.velocity.y > -PLAYER_MAX_SPEED_Y) {
            gPlayer.velocity.y -= PLAYER_ACCELERATION_NUMERATOR_Y;
        }
    }
    
    wasInputingJump = isInputingJump;
    
    /*
     * The section below outlines the logic for updating the player
     * character's position as a function of said entity's current
     * horizontal and vertical velocity.
     */
    
    INT8 playerDisplacementX = gPlayer.velocity.x / PLAYER_SPEED_DENOMINATOR;
    subPos += gPlayer.velocity.x % PLAYER_SPEED_DENOMINATOR;    
    // The state of being half-way through a full pixel in a sub-position
    // is reflected in the player character's position as being offset by
    // one backbuffer pixel.
    if (subPos > PLAYER_SPEED_DENOMINATOR
            || subPos < -PLAYER_SPEED_DENOMINATOR) {        
        playerDisplacementX += directionVector;
        subPos = gPlayer.velocity.x % PLAYER_SPEED_DENOMINATOR;
    }
    gPlayer.pos.x += playerDisplacementX;  
    
    // The vertical velocity does not need a subposition due to the
    // usually, significantly high acquirement of vertical speed when
    // jumping, thus rendering the potential subtle movements from 
    // using sub-positions inperceivable.
    INT8 playerDisplacementY = gPlayer.velocity.y / PLAYER_SPEED_DENOMINATOR;
    gPlayer.pos.y += playerDisplacementY;
    
    /*
     * The logic below dictates the behavior of the player character towards
     * the boundaries of the level described by the number of tile columns
     * present in said level.
     */
    
    // For unsigned integers, their substraction may result in the generation
    // of negative values for signed integers, which are interpreted by an
    // unsigned integer as being greater than half of the greatest value that
    // such an integer can represent. This behavior can result in the player
    // character's coordinate being increased by more than half of the maximum
    // value that such a coordinate can represent. This wrapping behavior can
    // be prevented by setting said coordinate to zero if the player
    // character's coordinate is at most the maximum distance that this
    // entity can travel in one game update.
    const UINT8 playerWidth = gCharacterMolds[player].collision.width;
    
    if (isOverflowByAtMost(playerDisplacementX, gPlayer.pos.x)) {
        gPlayer.pos.x = 0;
        gPlayer.velocity.x = 0;
    } else if (gPlayer.pos.x + playerWidth > gLevel.width) {
        // In this case, the player exceeded the right-most boundary of the
        // level.
        gPlayer.pos.x = gLevel.width - playerWidth;
        gPlayer.velocity.x = 0;
    }    
    
    // The calculated position below must be used for determining if the
    // player character could displace itself beyond the minimum number
    // representatble vertical coordinate and wrap to suddenly have a 
    // significantly large vertical position caused overflowing. This
    // vertical position must be subtracted by one since the player
    // character may spawn in non-air tiles and remain at a constant
    // vertical position. As such, proper detection of the overflow
    // bounrdary without relying on the player character's vertical
    // velocity can only be acheived by decrementing the current
    // player character's vertical coordinate.
    UINT16 playerCoordYUnder = gPlayer.pos.y - 1;
    const UINT8 playerHeight = gCharacterMolds[player].collision.height;
    
    if (isOverflowByAtMost(playerDisplacementY, playerCoordYUnder)) {
        killPlayer();
        // Collision and animation logic can be skipped when the player
        // respawns.
        return;
    } else if (gPlayer.pos.y > (BACKBUFFER_HEIGHT - playerHeight)) {
        // This condition is reached if the player character's
        // y-coordinate exceeds the height of the backbuffer.
        gPlayer.pos.y = BACKBUFFER_HEIGHT - playerHeight;
    }
    
    /*
     * The logic below governs the behavior of the player with regards to
     * the layout of the tilemap and the player character's position within.
     * These changes must be performed after all motion updates concerning
     * the player character since, at this point, the player character's 
     * position may be in a solid, non-traversable, tile.
     */
    
    const UINT16 playerCoordXRightEdge = gPlayer.pos.x + playerWidth - 1;
    const UINT16 tilesUnderPlayer = playerCoordYUnder / TILE_SIZE;
    const UINT16 tilesBehindRightSideOfPlayer = 
        (playerCoordXRightEdge / TILE_SIZE) * COLUMN_SIZE;
    const UINT16 tilesBehindLeftSideOfPlayer = 
        (gPlayer.pos.x / TILE_SIZE) * COLUMN_SIZE;
    const UINT8 tileIdRightWheel = gLevel.pTilemap[
        tilesBehindRightSideOfPlayer + tilesUnderPlayer];
    const UINT8 tileIdLeftWheel = gLevel.pTilemap[
        tilesBehindLeftSideOfPlayer + tilesUnderPlayer];
    
    // The player character features positions that are immediately below
    // its collision box's corners. These are referred to as "wheels." 
    // If such position overlap air tiles, then the player falls.
    if (tileIdRightWheel == tileAir && tileIdLeftWheel == tileAir) {
        if (isPlayerGrounded) {
            wasJumpNotReleased = FALSE;
            isPlayerGrounded = FALSE;
        }
    } else {
        // If such a pair of air tiles is not detected, another pair 
        // is checked. It is the tiles that overlap the corner
        // positions of the player character's collision box. If both are
        // air tiles, the player is assumed to be grounded.
        const UINT16 tilesInAndUnderPlayerBottom = gPlayer.pos.y / TILE_SIZE;
        
        const UINT16 tileIndexRightCorner = 
            tilesBehindRightSideOfPlayer + tilesInAndUnderPlayerBottom;
        const UINT16 tileIndexLeftCorner = 
            tilesBehindLeftSideOfPlayer + tilesInAndUnderPlayerBottom;    
            
        const UINT8 tileIdRightCorner = gLevel.pTilemap[tileIndexRightCorner];
        const UINT8 tileIdLeftCorner = gLevel.pTilemap[tileIndexLeftCorner];
        
        // The state of the player character remains unchanged if its corners
        // are in air.
        if (tileIdRightCorner != tileAir || tileIdLeftCorner != tileAir) {
            const UINT8 tileIdAboveRightCorner = gLevel.pTilemap[
                tileIndexRightCorner + 1];
            const UINT8 tileIdAboveLeftCorner = gLevel.pTilemap[
                    tileIndexLeftCorner + 1];
            if (isPlayerGrounded || (tileIdRightCorner == tileAir 
                    || tileIdLeftCorner == tileAir)
                    && (tileIdAboveRightCorner != tileAir
                    || tileIdAboveLeftCorner != tileAir
                    || jumpTimer == 0
                    || gPlayer.velocity.y >= 0)) {
                if (tileIdRightCorner == tileAir) {
                    gPlayer.pos.x = (gPlayer.pos.x / TILE_SIZE)
                        * TILE_SIZE + TILE_SIZE;
                } else {
                    gPlayer.pos.x = (gPlayer.pos.x / TILE_SIZE)
                        * TILE_SIZE;
                }
                gPlayer.velocity.x = 0;
            } else {
                gPlayer.pos.y = (gPlayer.pos.y / TILE_SIZE) * TILE_SIZE 
                    + TILE_SIZE;
                isPlayerGrounded = TRUE;
            }
        }
        
    }
    
    /*
     * The set of commands in the section below dictate the animation state
     * of the player character as a function of its motion and position.
     */
    
    BOOLEAN isPlayerMirrored = gPlayer.animState < 0;
    
    if (isPlayerGrounded) {
        if (gPlayer.velocity.x != 0) {        
            playerAnimationCounter += playerDisplacementX * directionVector;
            if (playerAnimationCounter > PLAYER_ANIMATION_CHANGE_PERIOD) {
                switch(gPlayer.animState) {
                    case -2: case -3:
                    
                    gPlayer.animState = -1;
                    break;
                    
                    case -1:
                    
                    gPlayer.animState = -2;
                    break;
                    
                    case 0:
                    
                    gPlayer.animState = 1;
                    break;
                    
                    case 1: case 2: default:
                    
                    gPlayer.animState = 0;
                    break;
                }                
                playerAnimationCounter = 0;
            }
            // Orientation of the player graphic is only updated when the
            // player character is grounded.
            if (directionVector == -1 && !isPlayerMirrored
                    || directionVector == 1 && isPlayerMirrored) {
                gPlayer.animState = ~gPlayer.animState;
            }
        } else {
            gPlayer.animState = -isPlayerMirrored;
        }
    } else {
        if (wasJumpNotReleased) {
            gPlayer.animState = 3;
        } else {
            gPlayer.animState = 2;
        }
        if (isPlayerMirrored) {
            gPlayer.animState = ~gPlayer.animState;
        }
    }
    
    /*
     * Non-player characters are updated in this function every time it is
     * called.
     */
    
    sCharacter* pCharacter;
    UINT8 characterId;
    sMold mold;
    
    UINT16 characterPosYUnder;
    UINT16 tileIndexLeftWheelChr;
    UINT16 tileIndexRightWheelChr;
    UINT8 tileIdLeftChr;
    UINT8 tileIdRightChr;
    
    BOOLEAN isLeftSolidChr;

    for (UINT8 instanceId = 0; 
            instanceId < gMutableCharacterArray.instances;
            instanceId++) {
        
        pCharacter = &(gMutableCharacterArray.pCharacter[0]) + instanceId;
        characterId = pCharacter->id;
        mold = gCharacterMolds[characterId];
        switch(characterId) {
            
            case bug:
            
            characterPosYUnder = pCharacter->pos.y - 1;
            tileIndexLeftWheelChr = pCharacter->pos.x
                / TILE_SIZE * COLUMN_SIZE + characterPosYUnder / TILE_SIZE;
            tileIndexRightWheelChr = (pCharacter->pos.x 
                + mold.collision.width - 1) / TILE_SIZE * COLUMN_SIZE 
                + characterPosYUnder / TILE_SIZE;
            
            tileIdLeftChr = gLevel.pTilemap[tileIndexLeftWheelChr];
            tileIdRightChr = gLevel.pTilemap[tileIndexRightWheelChr];
            
            if (tileIdLeftChr == tileAir && tileIdRightChr == tileAir
                    && pCharacter->velocity.y <= PLAYER_MAX_SPEED_Y) {
                
                pCharacter->velocity.y -= PLAYER_ACCELERATION_NUMERATOR_Y;
                if (-pCharacter->velocity.y > PLAYER_MAX_SPEED_Y) {
                    pCharacter->velocity.y = -PLAYER_MAX_SPEED_Y;
                }
                pCharacter->pos.y += pCharacter->velocity.y 
                    / PLAYER_SPEED_DENOMINATOR;
            } else {
                pCharacter->velocity.y = 0;
                pCharacter->pos.y = (characterPosYUnder + TILE_SIZE - 1)
                    / TILE_SIZE * TILE_SIZE;
            }
            
            // This character's vertical position can overflow. 
            // This event causes this character to despawn.
            if (isOverflowByAtMost(PLAYER_MAX_SPEED_Y + 1, characterPosYUnder)
                    || isOverflowByAtMost(PLAYER_MAX_SPEED_Y + 1, 
                        pCharacter->pos.y)) {
                pCharacter->id = idNull;
                pCharacter->pos.y = 0;
                continue;
            }
            
            isLeftSolidChr = gLevel.pTilemap[tileIndexLeftWheelChr + 1] 
                != tileAir;
            
            if (isLeftSolidChr || gLevel.pTilemap[tileIndexRightWheelChr + 1]
                    != tileAir) {
                if (isLeftSolidChr) {
                    pCharacter->animState = 0;
                } else {
                    pCharacter->animState = -1;
                }
            }            

            // The switch whose input is the bug character's animation state
            // updates horizontal position.
            switch(pCharacter->animState) {
                
                case ANIM_OFFSCREEN:
                
                pCharacter->animState = 0;
                continue;
                
                case ~ANIM_OFFSCREEN:
                
                pCharacter->animState = -1;
                continue;
                
                case 2: case -3:
                
                continue;
                
                case 0: case 1:
                
                pCharacter->pos.x += mold.maxSpeedX 
                    / PLAYER_SPEED_DENOMINATOR;
                pCharacter->animState = (pCharacter->pos.x >> 3) & 1;
                break;
                
                case -1: case -2: default:
                
                pCharacter->pos.x -= mold.maxSpeedX 
                    / PLAYER_SPEED_DENOMINATOR;
                pCharacter->animState = ~((pCharacter->pos.x >> 3) & 1);
                break;
            }
            
            // This character's horizontal position can overflow. Such an
            // occurance causes this character to face rightwards and to
            // remain in bounds. This check must be performed immediately
            // after when the character's horizontal position updates.
            if (isOverflowByAtMost(mold.maxSpeedX, pCharacter->pos.x)) {
                pCharacter->animState = 0;
                pCharacter->pos = gLevel.posPlayerSpawn;
                continue;
            }
            
            // The player character respawns when touching this character
            // while not being airborne.
            if ((((gPlayer.pos.x > pCharacter->pos.x)
                    && gPlayer.pos.x
                    < (pCharacter->pos.x + mold.collision.width))
                    || (((gPlayer.pos.x + playerWidth) > pCharacter->pos.x)
                    && (gPlayer.pos.x + playerWidth)
                    < (pCharacter->pos.x + mold.collision.width)))
                    && gPlayer.pos.y 
                    <= pCharacter->pos.y + mold.collision.height) {
                
                if (isPlayerGrounded) {
                    killPlayer();
                } else {
                    // This block of code executes if the player jumps on
                    // this character.
                    
                    // This pair of statements change this character's
                    // sprite to its defeated variant.
                    if (pCharacter->animState >= 0) {
                        // This block is executed for the right-facing
                        // animation frame.
                        pCharacter->animState = 2;
                    } else {
                        // This block executes for the left-facing
                        // animation frame.
                        pCharacter->animState = -3;
                    }
                    // Assignment of the inverse-signed vertical velocity
                    // of the player simulates a bouncing effect.
                    gPlayer.velocity.y = -gPlayer.velocity.y;
                    wasInputingJump = TRUE;
                    isInputingJump = TRUE;
                    wasJumpNotReleased = TRUE;
                    jumpTimer = 0;
                }
            }
            break;
            
            default:
            // This point is reached if a character id is unknown. That is,
            // no behavior is programmed for said id the character
            // bears.
            break;
        }
    }
    
    return;
}

/*
 * The "killPlayer" function resets all characters, including the player, to
 * their original states. These states describe position, velocity,
 * and animation state.
 */

__forceinline void killPlayer() {
    
    gPlayer.pos = gLevel.posPlayerSpawn;
    resetActors();
    return;
}