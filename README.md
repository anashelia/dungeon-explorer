DUNGEON EXPLORER
================

A terminal-based dungeon exploration and combat game written in C.
It reads a dungeon layout from a text file and lets you walk around
with WASD, revealing rooms, corridors, monsters, and treasure as
you go.

The world is drawn with colored ASCII characters, and there's a
legend on the right showing everything you've discovered so far.
Monsters fight back when you step on them, gold adds to your score,
and potions restore HP. Defeat every monster to win.


OVERVIEW
--------

The program loads a dungeon description from a file: the dungeon
dimensions, a list of rectangular rooms, a list of corridors
connecting them, and a list of entities (monsters and treasures).

You start in the top-left corner of the first room and explore
using the keyboard. A limited radius of the dungeon is visible
around you at any time; as you move, more of the map is revealed
and added to the legend.

Stepping onto a monster starts a turn-based fight. Stepping onto
gold or a potion picks it up. The game ends when you:

  - Defeat every monster (WIN), or
  - Run out of HP (GAME OVER), or
  - Press Q to leave the dungeon (QUIT).


WHAT IT DOES
------------

  * Prompts for a dungeon filename at startup.

  * Parses the file into:
      - Dungeon width and height.
      - Rooms (x, y, width, height).
      - Corridors (x1, y1, x2, y2).
      - Entities (monsters and treasure, each with a position,
        symbol, and description).

  * Places the player in the top-left floor tile of the first room,
    drawn as a green '@'.

  * Runs a game loop:
      1. Clears the screen.
      2. Reveals everything within radius 5 of the player.
      3. Draws visible rooms (blue '#' walls, white '.' floors).
      4. Draws visible corridors (cyan '-' and '|', corners '+').
      5. Draws visible entities:
           - Monsters in red ('G', 'O', 'D').
           - Treasure in yellow ('$', '!').
      6. Draws the player.
      7. Draws a legend on the right listing the symbol table and
         everything discovered so far.
      8. Shows a status line: HP, Gold, controls, last message.
      9. Reads a keypress (non-blocking) and acts on it.

  * When the player moves onto an entity, interact_with_entity()
    triggers the appropriate action:
      - Monster : fight_monster() runs a turn-based battle.
      - '$'     : adds its value to your gold total.
      - '!'     : restores POTION_HEAL (25) HP, capped at MAX_HP (100).
      - Other   : prints a generic "You found something!" message.

  * Exits cleanly when Q is pressed, HP reaches 0, or all monsters
    are defeated. A summary screen is printed in each case.


CONTROLS
--------

    W / w   move up
    S / s   move down
    A / a   move left
    D / d   move right
    Q / q   quit


COMBAT
------

Fighting is turn-based and fully automatic once you step on a
monster:

  1. The player always attacks first.
  2. Player damage = 4 + rand() % 7  (i.e. 4..10).
  3. If the monster survives, it strikes back:
        Goblin ('G') : 2 + rand() % 5   (2..6)
        Orc    ('O') : 4 + rand() % 7   (4..10)
        Dragon ('D') : 7 + rand() % 9   (7..15)
        Other        : 7 + rand() % 9   (7..15)
  4. Both HP values are clamped at 0 (never negative).
  5. Repeat until the monster dies or the player dies.
  6. Each exchange is shown on screen with a 1-second pause so you
     can read the damage.

Monster starting HP is set by symbol in read_dungeon():

    'G' (Goblin) : 10 HP
    'O' (Orc)    : 18 HP
    'D' (Dragon) : 30 HP
    other        : 12 HP

Defeating a monster marks it inactive, so it disappears from the
map and from the legend. When the last monster falls, the win
screen is shown.


VISUAL LEGEND
-------------

    @   Player
    #   Room wall
    .   Room floor
    -   Horizontal corridor
    |   Vertical corridor
    +   Corridor corner
    G   Goblin  (red)
    O   Orc     (red)
    D   Dragon  (red)
    $   Gold    (yellow)
    !   Potion  (yellow)


FILES
-----

  main.c
      The entire game. Contains:
        - Data structures: Room, Corridor, Entity.
        - read_dungeon(): parses the input file.
        - draw_room(), draw_corridor(), draw_entity(),
          draw_player(): rendering.
        - reveal_area(): fog-of-war based on distance from player.
        - is_position_valid(): walkability check.
        - draw_legend(): right-side legend.
        - count_monsters(): counts remaining live monsters.
        - show_message(): prints a status message with a pause.
        - fight_monster(): turn-based combat.
        - interact_with_entity(): picks up gold, potions, or starts
          a fight.
        - main(): game loop, input handling, end screens.

  terminal_draw.h
      Header-only terminal library. Provides:
        - clear_screen()
        - get_screen_size()
        - draw_char(x, y, c, color)
        - print_at(x, y, format, ...)
        - sleep_ms(ms)
        - Color enum (BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA,
          CYAN, WHITE)

  input.txt
      Example dungeon file (see FORMAT below).


DUNGEON FILE FORMAT
-------------------

The file is a whitespace-separated sequence of values:

    <width> <height>
    <room_count>
    <room_x> <room_y> <room_width> <room_height>      (repeated)
    <corridor_count>
    <x1> <y1> <x2> <y2>                              (repeated)
    <monster_count>
    <x> <y> <symbol> <name>                          (repeated)
    <treasure_count>
    <x> <y> <symbol> <description with spaces>       (repeated)

Example (from input.txt):

    50 25
    3
    2 2 15 8
    22 2 15 8
    12 15 15 8
    2
    16 6 22 6
    29 10 29 15
    3
    2
    8 5 G Goblin
    28 5 O Orc
    15 5 D Dragon
    10 5 $ 50 gold
    25 18 ! Magic potion

Notes on the format:

  - Monster names are single tokens (no spaces).
  - Treasure descriptions can contain spaces; the parser reads the
    rest of the line.
  - Corridors are axis-aligned: either x1 == x2 (vertical) or
    y1 == y2 (horizontal). Diagonal corridors are not supported.
  - Monster HP is NOT read from the file — it's assigned by symbol
    in read_dungeon() (see COMBAT above).
  - Gold value is parsed from the leading integer of the treasure
    description via atoi().


BUILDING
--------

Only main.c needs to be compiled (terminal_draw.h is header-only):

    gcc main.c -o dungeon

On Windows PowerShell, run the resulting binary as:

    .\dungeon.exe


RUNNING
-------

    .\dungeon.exe            (Windows PowerShell)
    ./dungeon                (Linux / macOS / Git Bash / WSL)

You'll be prompted:

    Enter dungeon filename:

Type `input.txt` (or the path to any dungeon file in the same
format) and press Enter.

Tips:
  - Maximize the terminal first: the legend is drawn to the right
    of the dungeon at column `width + 2`.
  - The example dungeon is 50 columns wide, so you'll need a fairly
    wide terminal to see the whole map and the legend together.
  - Each combat exchange pauses for 1 second (see show_message());
    this is intentional, so you can follow the fight.


HOW IT WORKS (INTERNALS)
------------------------

  - Fog of war: reveal_area() marks a room, corridor, or entity as
    visible if its center (for rooms), either endpoint (for
    corridors), or its own position (for entities) is within radius
    5 (Euclidean distance, squared comparison) of the player. Once
    revealed, it stays revealed.

  - Walkability: is_position_valid() returns true if the target
    tile is strictly inside a room's floor area or on a corridor
    segment. Walls, the outside world, and out-of-bounds are all
    blocked.

  - Input handling is non-blocking:
      - On Windows: _kbhit() / _getch() from <conio.h>.
      - On Unix: select() with a zero timeout, plus termios to
        disable canonical mode and echo while reading.
    This lets the loop run at ~20 FPS while still responding to
    keys.

  - Rendering is immediate-mode: the screen is cleared and fully
    redrawn every frame. Flicker on Windows is expected because of
    system("cls") in clear_screen().

  - All map data is dynamically allocated after parsing. Entities
    are allocated with calloc() so every field starts zeroed. All
    arrays are freed at exit.

  - Two-phase entity handling: monsters are read first (red), then
    treasure (yellow). The split point is monster_count; there is
    no per-entity color field in the file format.

  - Combat is deterministic given the RNG seed. srand(time(NULL))
    is called once at startup, so each run differs.


NOTES AND CAVEATS
-----------------

  * Windows vs. Unix input: the code uses #ifdef _WIN32 to pick
    the right non-blocking input method. Both branches are
    implemented.

  * Room visibility is based on the room's CENTER, not on its
    individual tiles. A large room may stay hidden until you're
    close to its middle, even if you're standing next to its wall.
    This is a simplification, not a bug.

  * Entity visibility is also permanent once revealed — entities
    are not re-hidden when you walk away.

  * Because the legend grows as you discover things, on very small
    terminals it can overwrite the dungeon. Resize the window if
    that happens.

  * The dungeon width/height (from the file) is independent of the
    terminal size. The legend is placed at column `width + 2`,
    which assumes the dungeon fits comfortably on screen.

  * The combat loop is fully automatic — there's no flee option
    and no player input during a fight. If your HP is too low and
    you step on a Dragon, you can lose before you can react.

  * Potions are single-use: entity->active is set to false after
    use, so the potion disappears and cannot be reused.

  * Gold is accumulated across the run and reported on the end
    screen, but there is no shop or spending mechanic.

  * Monster HP is hard-coded per symbol in read_dungeon(). If you
    add new monster symbols to a dungeon file, they default to
    12 HP and use the "other" damage roll (7..15).

  * Input file parsing is defensive: every fscanf return value is
    checked, and failures free resources and return false. Malformed
    files should fail cleanly rather than crash.

  * The program sleeps 50 ms per frame (~20 FPS) plus 1 second per
    combat message. Combat-heavy runs can take a while.

  * End-of-game summary differs by outcome:
      - Win  : "DUNGEON COMPLETED!" with remaining HP and gold.
      - Loss : "GAME OVER" with gold collected.
      - Quit : "You left the dungeon." with gold collected.


LICENSE
-------

This program is provided as-is for educational and demonstration
purposes. Use and modify it freely.