# Changelog

This document aims to record the evolution of project Facade.

## [Unreleased] - 2023-12-27

The author preserves a motion test on January 16, 2024. This iteration
is intended to serve as the foundation of a game prototype. Plans for this
project to appear in the @Hack 2024 event are set. Base features are as
follows:
 - A backbuffer of pixel count matching the monitor's. The application window
   displays the contents every game update;
 - A controllable, white square. Arrow keys and the space bar controls its
   position. This square is bounded by the dimensions of the window;
 - A debug interface describing current CPU, RAM, and Pagefile usage
   alongside handle count. Ctrl + C toggles on and off this menu's
   rendering;
 - A debug console to print messages accompanying the debug interface.

## [v0.1.0-alpha] - 2024-01-30

This development version establishes the basis for characters, graphic 
decoding and rendering routines, and the level tile map.

### Added
 - Usage of an indexed color palette to decode a linear sequence of indices
   for the rendering protocol. Binary number-coded image shall describe
   this graphic format. Files adhering to the format can feature the 
   ```.bci``` file extension;
 - A mirroring procedure for loaded graphics;
 - Reservation of a unique file for tile graphic data. It shall be known
   as the tile texture map. It is a linear sequence of contents of binary
   number-coded images. The ```.tmp``` file extension of this format can 
   denote this convention;
 - Tile rendering logic;
 - The air tile;
 - The stone tile;
 - The brick tile;
 - An introductory level immediately loaded upon execution. Level data 
   originates from the ```tutorial_1.lvl``` file. It describes the
   level's width and terrain in terms of air and solid tiles. The level
   decoding procedure translates this information to a tile map;
 - Decoding procedure for also new mold information files. These bear the
   ```.mld``` extension. They describe the width, height, and animation frame 
   count of the binary number-coded image they share names with;
 - Characters, which individually feature positions, velocities, mold 
   identifiers, and animation states;
 - Molds organized by identifiers, which define universal characteristics of
   a characters associated to the mold identifier;
 - Holding X increases the maximum horizontal speed attainable by what
   the player is controlling;
 - Player collision with the tiles of the level tile map;
 - Player coordinates in the debug interface.

### Changed
 - Resized the backbuffer to 384x216 pixels instead of the native monitor
   dimensions. The rendering routine stretches this memory region to the
   monitor's dimensions.
 - A Horus player character substitutes the white square. It features 
   animations for running and jumping;
 - Reworked motion logic as a function of player input;
 - Holding space increases the jump height of the player character;
 - The player character cannot exceed a defined maximum vertical
   and horizontal speed.

### Fixed
N/A

## [v0.2.0-beta] - 2024-02-20

This iteration implements the first non-player character.

### Added
 - Character spawn slots, which does not include the player character;
 - A decoding procedure for a generation file accompanying a level. It
   describes the spawn locations for characters of a particular mold type,
   alongside the amount of spawn slots. This file format bears the 
   ```.txt``` extension;
 - Player character respawning behaviour when falling into a pit;
 - The scarab beetle character. This entity causes the player character to 
   respawn if in contact while not airborne. The player character needs to 
   jump on this character to kill it;
 - A static background behind all on-screen characters and tiles. It is
   stored as a binary number-coded image.

### Changed
 - Altered level design in the introductory level to be more challenging.

### Fixed
 - BUGFIX: The player character immediately faces right when landing.

## [v0.3.0-beta] - 2024-02-27

This version polishes and finalizes all graphics.

### Added
 - Character reinitialization upon player death.
 
### Changed
 - Reworked tile rendering. The tile map rendering procedure displays the
   left and right most tile columns with separate sub-procedures;
 - Removed some dark orange pixels in the gradient present in the background;
 - Re-textured the player character;
 - Darkened the scarab beetle character's graphics;
 - Increased the height of the cliff located after the first pit in the
   introductory level;
 - Added another scarab beetle near the end of the introductory level.

### Fixed
 - BUGFIX: Non-player characters can behave as if they were offscreen if the
   player character shifts the viewport to the left. This bug occurs at the
   right-most part of a loaded level.

## [v1.0.0-release] - 2024-02-29

### Added
N/A

### Changed
 - Reworked non-player character rendering procedure such that the
   sub-procedure to write to the backbuffer is referenced only once;
 - Increased the priority level of the process;
 - Removed the brick tile and its corresponding texture in the tile
   texture map.

### Fixed
 - BUGFIX: Mirrored, 100+ pixel wide sprites render garbage data.
 - BUGFIX: The right ends of 100+ pixel wide sprites can wrap around the
   screen if appearing cut off in the player's viewport.
 - BUGFIX: The collision width of the scarab beetle character does not 
   match the one of its mold.
