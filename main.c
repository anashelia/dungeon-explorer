
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "terminal_draw.h"

#ifndef _WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#else
#include <conio.h>
#endif

#define MAX_HP 100
#define POTION_HEAL 25

typedef struct {
    int x;
    int y;
    int width;
    int height;
    bool visible;
} Room;

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
    bool visible;
} Corridor;

typedef struct {
    int x;
    int y;
    char symbol;
    Color color;
    char description[50];
    bool visible;
    bool active;
    int hp;
    int value;
} Entity;


/* ---------------- DUNGEON FILE ---------------- */

bool read_dungeon(const char* filename,
                  int* width, int* height,
                  Room** rooms, int* room_count,
                  Corridor** corridors, int* corridor_count,
                  Entity** entities, int* entity_count) {

    FILE* file = fopen(filename, "r");

    if (!file) {
        return false;
    }

    if (fscanf(file, "%d %d", width, height) != 2) {
        fclose(file);
        return false;
    }

    if (fscanf(file, "%d", room_count) != 1 || *room_count <= 0) {
        fclose(file);
        return false;
    }

    *rooms = malloc(*room_count * sizeof(Room));

    if (!*rooms) {
        fclose(file);
        return false;
    }

    for (int i = 0; i < *room_count; i++) {
        if (fscanf(file, "%d %d %d %d",
                   &(*rooms)[i].x,
                   &(*rooms)[i].y,
                   &(*rooms)[i].width,
                   &(*rooms)[i].height) != 4) {
            fclose(file);
            return false;
        }

        (*rooms)[i].visible = false;
    }

    if (fscanf(file, "%d", corridor_count) != 1 ||
        *corridor_count < 0) {
        fclose(file);
        return false;
    }

    *corridors = NULL;

    if (*corridor_count > 0) {
        *corridors = malloc(*corridor_count * sizeof(Corridor));

        if (!*corridors) {
            fclose(file);
            return false;
        }
    }

    for (int i = 0; i < *corridor_count; i++) {
        if (fscanf(file, "%d %d %d %d",
                   &(*corridors)[i].x1,
                   &(*corridors)[i].y1,
                   &(*corridors)[i].x2,
                   &(*corridors)[i].y2) != 4) {
            fclose(file);
            return false;
        }

        (*corridors)[i].visible = false;
    }

    int monster_count;
    int treasure_count;

    if (fscanf(file, "%d", &monster_count) != 1 ||
        fscanf(file, "%d", &treasure_count) != 1 ||
        monster_count < 0 || treasure_count < 0) {
        fclose(file);
        return false;
    }

    *entity_count = monster_count + treasure_count;
    *entities = NULL;

    if (*entity_count > 0) {
        *entities = calloc(*entity_count, sizeof(Entity));

        if (!*entities) {
            fclose(file);
            return false;
        }
    }

    /* Read monsters */

    for (int i = 0; i < monster_count; i++) {
        if (fscanf(file, "%d %d %c %49s",
                   &(*entities)[i].x,
                   &(*entities)[i].y,
                   &(*entities)[i].symbol,
                   (*entities)[i].description) != 4) {
            fclose(file);
            return false;
        }

        (*entities)[i].color = RED;
        (*entities)[i].visible = false;
        (*entities)[i].active = true;

        if ((*entities)[i].symbol == 'G') {
            (*entities)[i].hp = 10;
        }
        else if ((*entities)[i].symbol == 'O') {
            (*entities)[i].hp = 18;
        }
        else if ((*entities)[i].symbol == 'D') {
            (*entities)[i].hp = 30;
        }
        else {
            (*entities)[i].hp = 12;
        }
    }

    /* Read treasures */

    for (int i = 0; i < treasure_count; i++) {
        int idx = monster_count + i;

        if (fscanf(file, "%d %d %c %49[^\n]",
                   &(*entities)[idx].x,
                   &(*entities)[idx].y,
                   &(*entities)[idx].symbol,
                   (*entities)[idx].description) != 4) {
            fclose(file);
            return false;
        }

        (*entities)[idx].color = YELLOW;
        (*entities)[idx].visible = false;
        (*entities)[idx].active = true;

        if ((*entities)[idx].symbol == '$') {
            (*entities)[idx].value =
                atoi((*entities)[idx].description);
        }
    }

    fclose(file);
    return true;
}


/* ---------------- DRAWING ---------------- */

void draw_room(Room room) {

    if (!room.visible) return;

    for (int x = room.x; x < room.x + room.width; x++) {
        draw_char(x, room.y, '#', BLUE);
        draw_char(x, room.y + room.height - 1, '#', BLUE);
    }

    for (int y = room.y + 1;
         y < room.y + room.height - 1; y++) {

        draw_char(room.x, y, '#', BLUE);

        draw_char(room.x + room.width - 1, y, '#', BLUE);
    }

    for (int x = room.x + 1;
         x < room.x + room.width - 1; x++) {

        for (int y = room.y + 1;
             y < room.y + room.height - 1; y++) {

            draw_char(x, y, '.', WHITE);
        }
    }
}


void draw_corridor(Corridor corridor) {

    if (!corridor.visible) return;

    if (corridor.y1 == corridor.y2) {

        int start_x = corridor.x1 < corridor.x2 ?
                      corridor.x1 : corridor.x2;

        int end_x = corridor.x1 < corridor.x2 ?
                    corridor.x2 : corridor.x1;

        for (int x = start_x; x <= end_x; x++) {
            draw_char(x, corridor.y1, '-', CYAN);
        }
    }

    if (corridor.x1 == corridor.x2) {

        int start_y = corridor.y1 < corridor.y2 ?
                      corridor.y1 : corridor.y2;

        int end_y = corridor.y1 < corridor.y2 ?
                    corridor.y2 : corridor.y1;

        for (int y = start_y; y <= end_y; y++) {
            draw_char(corridor.x1, y, '|', CYAN);
        }
    }

    draw_char(corridor.x1, corridor.y1, '+', CYAN);
    draw_char(corridor.x2, corridor.y2, '+', CYAN);
}


void draw_entity(Entity entity) {

    if (!entity.visible || !entity.active) return;

    draw_char(entity.x, entity.y,
              entity.symbol, entity.color);
}


void draw_player(int x, int y) {
    draw_char(x, y, '@', GREEN);
}


/* ---------------- VISIBILITY ---------------- */

void reveal_area(int player_x, int player_y, int radius,
                 Room* rooms, int room_count,
                 Corridor* corridors, int corridor_count,
                 Entity* entities, int entity_count) {

    for (int i = 0; i < room_count; i++) {

        int center_x = rooms[i].x + rooms[i].width / 2;
        int center_y = rooms[i].y + rooms[i].height / 2;

        int dx = center_x - player_x;
        int dy = center_y - player_y;

        if (dx * dx + dy * dy <= radius * radius) {
            rooms[i].visible = true;
        }
    }

    for (int i = 0; i < corridor_count; i++) {

        int dx1 = corridors[i].x1 - player_x;
        int dy1 = corridors[i].y1 - player_y;

        int dx2 = corridors[i].x2 - player_x;
        int dy2 = corridors[i].y2 - player_y;

        if (dx1 * dx1 + dy1 * dy1 <= radius * radius ||
            dx2 * dx2 + dy2 * dy2 <= radius * radius) {

            corridors[i].visible = true;
        }
    }

    for (int i = 0; i < entity_count; i++) {

        if (!entities[i].active) continue;

        int dx = entities[i].x - player_x;
        int dy = entities[i].y - player_y;

        if (dx * dx + dy * dy <= radius * radius) {
            entities[i].visible = true;
        }
    }
}


/* ---------------- MOVEMENT ---------------- */

bool is_position_valid(int x, int y,
                       int width, int height,
                       Room* rooms, int room_count,
                       Corridor* corridors, int corridor_count) {

    if (x < 0 || x >= width ||
        y < 0 || y >= height) {
        return false;
    }

    for (int i = 0; i < room_count; i++) {

        if (x > rooms[i].x &&
            x < rooms[i].x + rooms[i].width - 1 &&
            y > rooms[i].y &&
            y < rooms[i].y + rooms[i].height - 1) {

            return true;
        }
    }

    for (int i = 0; i < corridor_count; i++) {

        if (corridors[i].y1 == corridors[i].y2 &&
            y == corridors[i].y1) {

            int min_x = corridors[i].x1 < corridors[i].x2 ?
                        corridors[i].x1 : corridors[i].x2;

            int max_x = corridors[i].x1 < corridors[i].x2 ?
                        corridors[i].x2 : corridors[i].x1;

            if (x >= min_x && x <= max_x) {
                return true;
            }
        }

        if (corridors[i].x1 == corridors[i].x2 &&
            x == corridors[i].x1) {

            int min_y = corridors[i].y1 < corridors[i].y2 ?
                        corridors[i].y1 : corridors[i].y2;

            int max_y = corridors[i].y1 < corridors[i].y2 ?
                        corridors[i].y2 : corridors[i].y1;

            if (y >= min_y && y <= max_y) {
                return true;
            }
        }
    }

    return false;
}


/* ---------------- PLAYER AND GAME STATE ---------------- */

void draw_legend(int x, Entity* entities, int entity_count) {

    int y = 0;

    print_at(x, y++, "===== LEGEND =====");

    print_at(x, y++, "@ : Player");
    print_at(x, y++, "G : Goblin");
    print_at(x, y++, "O : Orc");
    print_at(x, y++, "D : Dragon");
    print_at(x, y++, "$ : Gold");
    print_at(x, y++, "! : Potion");

    y++;

    print_at(x, y++, "Discovered:");

    for (int i = 0; i < entity_count; i++) {

        if (entities[i].visible && entities[i].active) {

            print_at(x, y++, "%c : %s",
                     entities[i].symbol,
                     entities[i].description);
        }
    }
}


int count_monsters(Entity* entities, int entity_count) {

    int count = 0;

    for (int i = 0; i < entity_count; i++) {

        if (entities[i].active &&
            (entities[i].symbol == 'G' ||
             entities[i].symbol == 'O' ||
             entities[i].symbol == 'D')) {

            count++;
        }
    }

    return count;
}


void show_message(int width, int height, const char* message) {

    print_at(0, height + 3, "%-*s", width, "");
    print_at(0, height + 3, "%s", message);

    sleep_ms(1000);
}


/* ---------------- COMBAT ---------------- */

void fight_monster(Entity* monster,
                   int* player_hp,
                   int width,
                   int height) {

    char message[120];

    snprintf(message, sizeof(message),
             "A %s attacks you!",
             monster->description);

    show_message(width, height, message);

    while (monster->hp > 0 && *player_hp > 0) {

        /* Player attacks */

        int player_damage = 4 + rand() % 7;

        monster->hp -= player_damage;

        if (monster->hp < 0) {
            monster->hp = 0;
        }

        snprintf(message, sizeof(message),
                 "You hit the %s for %d damage! Monster HP: %d",
                 monster->description,
                 player_damage,
                 monster->hp);

        show_message(width, height, message);

        if (monster->hp == 0) {
            break;
        }

        /* Monster attacks */

        int monster_damage;

        if (monster->symbol == 'G') {
            monster_damage = 2 + rand() % 5;
        }
        else if (monster->symbol == 'O') {
            monster_damage = 4 + rand() % 7;
        }
        else {
            monster_damage = 7 + rand() % 9;
        }

        *player_hp -= monster_damage;

        if (*player_hp < 0) {
            *player_hp = 0;
        }

        snprintf(message, sizeof(message),
                 "%s hits you for %d damage! Your HP: %d",
                 monster->description,
                 monster_damage,
                 *player_hp);

        show_message(width, height, message);
    }

    if (*player_hp > 0) {

        monster->active = false;

        snprintf(message, sizeof(message),
                 "You defeated the %s!",
                 monster->description);

        show_message(width, height, message);
    }
    else {
        show_message(width, height,
                     "You have been defeated!");
    }
}


/* ---------------- ENTITY INTERACTIONS ---------------- */

void interact_with_entity(int x, int y,
                          Entity* entities, int entity_count,
                          int* player_hp, int* gold,
                          int width, int height) {

    for (int i = 0; i < entity_count; i++) {

        Entity* entity = &entities[i];

        if (!entity->active ||
            entity->x != x ||
            entity->y != y) {
            continue;
        }

        /* Monsters */

        if (entity->symbol == 'G' ||
            entity->symbol == 'O' ||
            entity->symbol == 'D') {

            fight_monster(entity, player_hp, width, height);
        }

        /* Gold */

        else if (entity->symbol == '$') {

            *gold += entity->value;

            char message[100];

            snprintf(message, sizeof(message),
                     "You collected %d gold! Total: %d",
                     entity->value, *gold);

            entity->active = false;

            show_message(width, height, message);
        }

        /* Health potion */

        else if (entity->symbol == '!') {

            int old_hp = *player_hp;

            *player_hp += POTION_HEAL;

            if (*player_hp > MAX_HP) {
                *player_hp = MAX_HP;
            }

            char message[100];

            snprintf(message, sizeof(message),
                     "You used a potion! HP: %d -> %d",
                     old_hp, *player_hp);

            entity->active = false;

            show_message(width, height, message);
        }

        else {
            show_message(width, height,
                         "You found something!");
        }

        return;
    }
}


/* ---------------- MAIN ---------------- */

int main() {

    srand((unsigned int)time(NULL));

    char filename[100];

    printf("Enter dungeon filename: ");

    if (scanf("%99s", filename) != 1) {
        return 1;
    }

    int width, height;

    Room* rooms = NULL;
    Corridor* corridors = NULL;
    Entity* entities = NULL;

    int room_count = 0;
    int corridor_count = 0;
    int entity_count = 0;

    if (!read_dungeon(filename,
                      &width, &height,
                      &rooms, &room_count,
                      &corridors, &corridor_count,
                      &entities, &entity_count)) {

        printf("Failed to read dungeon file!\n");

        free(rooms);
        free(corridors);
        free(entities);

        return 1;
    }

    int player_x = rooms[0].x + 1;
    int player_y = rooms[0].y + 1;

    int player_hp = MAX_HP;
    int gold = 0;

    bool running = true;
    bool won = false;

    char message[120] = "Explore the dungeon!";

    while (running) {

        clear_screen();

        reveal_area(player_x, player_y, 5,
                    rooms, room_count,
                    corridors, corridor_count,
                    entities, entity_count);

        /* Draw rooms */

        for (int i = 0; i < room_count; i++) {
            draw_room(rooms[i]);
        }

        /* Draw corridors */

        for (int i = 0; i < corridor_count; i++) {
            draw_corridor(corridors[i]);
        }

        /* Draw entities */

        for (int i = 0; i < entity_count; i++) {
            draw_entity(entities[i]);
        }

        /* Draw player */

        draw_player(player_x, player_y);

        /* Draw legend */

        draw_legend(width + 2, entities, entity_count);

        /* Draw status */

        print_at(0, height + 1,
                 "HP: %d/%d   Gold: %d",
                 player_hp, MAX_HP, gold);

        print_at(0, height + 2,
                 "WASD: Move | Q: Quit");

        print_at(0, height + 3, "%s", message);

        if (player_hp <= 0) {
            running = false;
            won = false;
            break;
        }

        if (count_monsters(entities, entity_count) == 0) {
            running = false;
            won = true;
            break;
        }

        /* Read keyboard input */

        char ch = '\0';
        bool key_pressed = false;

#ifdef _WIN32

        if (_kbhit()) {
            ch = _getch();
            key_pressed = true;
        }

#else

        struct termios oldt, newt;

        tcgetattr(STDIN_FILENO, &oldt);

        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);

        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        fd_set readfds;
        struct timeval tv;

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (select(STDIN_FILENO + 1,
                   &readfds, NULL, NULL, &tv) > 0) {

            ch = getchar();
            key_pressed = true;
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

#endif

        if (key_pressed) {

            int new_x = player_x;
            int new_y = player_y;

            switch (ch) {

                case 'w':
                case 'W':
                    new_y--;
                    break;

                case 's':
                case 'S':
                    new_y++;
                    break;

                case 'a':
                case 'A':
                    new_x--;
                    break;

                case 'd':
                case 'D':
                    new_x++;
                    break;

                case 'q':
                case 'Q':
                    running = false;
                    break;
            }

            if (running &&
                is_position_valid(new_x, new_y,
                                  width, height,
                                  rooms, room_count,
                                  corridors, corridor_count)) {

                player_x = new_x;
                player_y = new_y;

                strcpy(message, "You moved.");

                interact_with_entity(player_x, player_y,
                                     entities, entity_count,
                                     &player_hp, &gold,
                                     width, height);

                if (player_hp <= 0) {
                    running = false;
                    won = false;
                }
            }
        }

        sleep_ms(50);
    }

    /* ---------------- GAME END ---------------- */

    clear_screen();

    if (won) {

        printf("\n");
        printf("====================================\n");
        printf("          DUNGEON COMPLETED!\n");
        printf("====================================\n");
        printf("\n");
        printf("Congratulations! You defeated all monsters!\n");
        printf("Remaining HP: %d/%d\n", player_hp, MAX_HP);
        printf("Gold collected: %d\n", gold);
    }
    else if (player_hp <= 0) {

        printf("\n");
        printf("====================================\n");
        printf("             GAME OVER\n");
        printf("====================================\n");
        printf("\n");
        printf("You were defeated in the dungeon.\n");
        printf("Gold collected: %d\n", gold);
    }
    else {

        printf("\nYou left the dungeon.\n");
        printf("Gold collected: %d\n", gold);
    }

    free(rooms);
    free(corridors);
    free(entities);

    return 0;
}
