#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <windows.h>

#define MAP_HEIGHT 12
#define MAP_WIDTH 10
#define MAX_ENEMIES 5
#define MAX_TRAPS 12
#define INVENTORY_WIDTH 10
#define INVENTORY_HEIGHT 10
#define MAX_GROUND_ITEMS 10

typedef int (*AttackFunction)(int attack, int defense);

typedef struct {
    char name[50];
    int width;
    int height;
    char symbol;
    int isEquipped;
    int posX;
    int posY;
    int attackBonus;
    int defenseBonus;
    int healthBonus;
    int itemX;
    int itemY;
} Item;

typedef struct {
    int width;
    int height;
    int** slots;  // Macierz slotow (0 - wolny, 1 - zajety)
    Item*** items;
} Inventory;

typedef struct {
    char name[50];
    int health;
    int max_health;
    int attack;
    int defense;
    int posX;
    int posY;
    int gold;
    Inventory* inventory;
} Player;

typedef struct {
    char name[50];
    int health;
    int attack;
    int defense;
    int EposX;
    int EposY;
} Enemy;

typedef struct {
    int posX;
    int posY;
    int damage;
    int discovered;
    char description[100];
} Trap;

typedef struct {
    Player* player;
    Enemy** enemies;
    int enemyCount;
    Trap** traps;
    int trapCount;
    char** map;
    int enemiesDefeated;
    int level;
    int portalX;
    int portalY;
    int portalActive;
    int totalEnemiesDefeated;
    Item** groundItems;
    int groundItemCount;
} GameWorld;

// Prototypy funkcji
int isHere(GameWorld* world, int x, int y, int currentEnemies, int currentTraps);
Player* createPlayer();
void initPlayer(Player* player);
Enemy* createEnemy(int x, int y);
void initEnemy(Enemy* enemy, int x, int y);
Trap* createTrap(int x, int y);
void initTrap(Trap* trap, int x, int y);
void checkTraps(GameWorld* world);
GameWorld* createGameWorld();
int battle(Player* player, Enemy* enemy, GameWorld* world);
void printMap(GameWorld* world);
void moveEnemy(Enemy* enemy);
void movePlayerAndEnemy(GameWorld* world);
void freeGameWorld(GameWorld* world);
void initMap(GameWorld* world);
void reloadMap(GameWorld* world);
void nextLevel(GameWorld* world);
void activatePortal(GameWorld* world);
void addItemToGround(GameWorld* world, Item* item, int x, int y);
void removeItemFromGround(GameWorld* world, int index);
int normalAttack(int attack, int defense);
int criticalAttack(int attack, int defense);

// Funkcje ekwipunku
Inventory* createInventory(int width, int height);
void freeInventory(Inventory* inv);
int canPlaceItem(Inventory* inv, Item* item, int x, int y);
int addItemToInventory(Inventory* inv, Item* item, int x, int y);
void removeItemFromInventory(Inventory* inv, Item* item);
void printInventory(Inventory* inv);
void inventoryMenu(GameWorld* world);
Item* createHealthPotion();
Item* createSword();
Item* createArmor();
void useItem(Player* player, Item* item);

void saveGame(GameWorld* world);
GameWorld* loadGame();

Inventory* createInventory(int width, int height) {
    Inventory* inv = (Inventory*)malloc(sizeof(Inventory));
    inv->width = width;
    inv->height = height;

    // Alokacja macierzy slotów
    inv->slots = (int**)malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++) {
        inv->slots[i] = (int*)malloc(width * sizeof(int));
        // Inicjalizacja slotów jako wolne
        for (int j = 0; j < width; j++) {
            inv->slots[i][j] = 0;
        }
    }

    // Alokacja tablicy wskaźników na przedmioty
    inv->items = (Item***)malloc(height * sizeof(Item**));
    for (int i = 0; i < height; i++) {
        inv->items[i] = (Item**)malloc(width * sizeof(Item*));
        // Inicjalizacja wskaźników na NULL
        for (int j = 0; j < width; j++) {
            inv->items[i][j] = NULL;
        }
    }

    return inv;
}

void freeInventory(Inventory* inv) {
    if (!inv) return;

    for (int y = 0; y < inv->height; y++) {
        for (int x = 0; x < inv->width; x++) {
            Item* item = inv->items[y][x];
            
            if (item != NULL && item->posX == x && item->posY == y) {
                free(item);
            }
            inv->items[y][x] = NULL; 
        }
    }

    for (int i = 0; i < inv->height; i++) {
        free(inv->slots[i]);
        free(inv->items[i]);
    }
    free(inv->slots);
    free(inv->items);
    free(inv);
}

int canPlaceItem(Inventory* inv, Item* item, int x, int y) {
    if (x < 0 || y < 0 || x >= inv->width || y >= inv->height) {
        return 0;
    }

    if (x + item->width > inv->width || y + item->height > inv->height) {
        return 0;
    }

    for (int i = y; i < y + item->height; i++) {
        for (int j = x; j < x + item->width; j++) {
            if (inv->slots[i][j] != 0) {
                return 0;
            }
        }
    }

    return 1;
}

int addItemToInventory(Inventory* inv, Item* item, int x, int y) {
    if (!canPlaceItem(inv, item, x, y)) {
        return 0;
    }

    // Zapisz oryginalne rozmiary przedmiotu
    int originalWidth = item->width;
    int originalHeight = item->height;

    for (int i = y; i < y + originalHeight; i++) {
        for (int j = x; j < x + originalWidth; j++) {
            inv->slots[i][j] = 1;
            inv->items[i][j] = item;
        }
    }
    item->posX = x;
    item->posY = y;
    item->isEquipped = 1;

    return 1;
}

void addItemToGround(GameWorld* world, Item* item, int x, int y) {
    if (world->groundItemCount >= MAX_GROUND_ITEMS) {
        printf("Nie mozna dodac wiecej przedmiotow na ziemi!\n");
        free(item); // Dodane zwolnienie pamięci
        return;
    }

    Item* newItem = (Item*)malloc(sizeof(Item));
    if (newItem == NULL) {
        printf("Blad alokacji pamieci dla nowego przedmiotu!\n");
        free(item); // Dodane zwolnienie pamięci
        return;
    }
    memcpy(newItem, item, sizeof(Item));
    newItem->posX = x;
    newItem->posY = y;
    newItem->isEquipped = 0;

    world->groundItems[world->groundItemCount] = newItem;
    world->groundItemCount++;

    world->map[y][x] = 'I';
}

void removeItemFromGround(GameWorld* world, int index) {
    if (index < 0 || index >= world->groundItemCount) return;

    free(world->groundItems[index]);
    for (int i = index; i < world->groundItemCount - 1; i++) {
        world->groundItems[i] = world->groundItems[i + 1];
    }
    world->groundItemCount--;
}

void removeItemFromInventory(Inventory* inv, Item* item) {
    if (!item || !item->isEquipped) return;

    int x = item->posX;
    int y = item->posY;

    // Sprawdź czy przedmiot jest w podanej lokalizacji
    if (x < 0 || y < 0 || x >= inv->width || y >= inv->height ||
        inv->items[y][x] != item) {
        return;
    }

    // Zwolnij wszystkie sloty zajmowane przez przedmiot
    for (int i = y; i < y + item->height; i++) {
        for (int j = x; j < x + item->width; j++) {
            if (i < inv->height && j < inv->width) {
                inv->slots[i][j] = 0;
                inv->items[i][j] = NULL;
            }
        }
    }

    free(item);
}

void printInventory(Inventory* inv) {
    printf("Ekwipunek (%dx%d):\n", inv->width, inv->height);

    // Nagłówki kolumn
    printf("   ");
    for (int x = 0; x < inv->width; x++) {
        printf("%2d ", x);
    }
    printf("\n");

    for (int y = 0; y < inv->height; y++) {
        printf("%2d ", y);
        for (int x = 0; x < inv->width; x++) {
            if (inv->slots[y][x] == 0) {
                printf(" . ");
            }
            else {
                Item* item = inv->items[y][x];
                // Sprawdź czy to jest główny slot przedmiotu (lewy górny róg)
                if (item != NULL && item->posX == x && item->posY == y) {
                    printf("[%c]", item->symbol);
                }
                // Sprawdź czy slot jest zajęty przez przedmiot
                else if (item != NULL) {
                    printf(" # ");
                }
                else {
                    printf(" . ");
                }
            }
        }
        printf("\n");
    }
}

void inventoryMenu(GameWorld* world) {
    while (1) {
        system("cls");
        printf("==== EKWIPUNEK ====\n");
        printInventory(world->player->inventory);

        printf("\n1. Przenies przedmiot\n2. Uzyj przedmiotu\n3. Wroc\nWybierz: ");
        int choice;
        scanf_s("%d", &choice);

        if (choice == 3) {
            break;
        }
        else if (choice == 2) {
            printf("Podaj pozycje przedmiotu (x y): ");
            int x, y;
            scanf_s("%d %d", &x, &y);

            if (x >= 0 && x < world->player->inventory->width &&
                y >= 0 && y < world->player->inventory->height) {
                Item* item = world->player->inventory->items[y][x];
                if (item != NULL && item->posX == x && item->posY == y) {
                    useItem(world->player, item);
                    printf("Uzyto przedmiotu: %s\n", item->name);
                    Sleep(2000);
                }
                else {
                    printf("Nie ma przedmiotu na tej pozycji!\n");
                    Sleep(1000);
                }
            }
            else {
                printf("Nieprawidlowa pozycja!\n");
                Sleep(1000);
            }
        }
        else if (choice == 1) {
            printf("Podaj pozycje przedmiotu (x y): ");
            int oldX, oldY;
            scanf_s("%d %d", &oldX, &oldY);

            printf("Podaj nowa pozycje (x y): ");
            int newX, newY;
            scanf_s("%d %d", &newX, &newY);

            if (oldX >= 0 && oldX < world->player->inventory->width &&
                oldY >= 0 && oldY < world->player->inventory->height &&
                newX >= 0 && newX < world->player->inventory->width &&
                newY >= 0 && newY < world->player->inventory->height) {

                Item* item = world->player->inventory->items[oldY][oldX];
                if (item != NULL && item->posX == oldX && item->posY == oldY) {
                    removeItemFromInventory(world->player->inventory, item);
                    if (!addItemToInventory(world->player->inventory, item, newX, newY)) {
                        addItemToInventory(world->player->inventory, item, oldX, oldY);
                        printf("Nie mozna przeniesc przedmiotu!\n");
                        Sleep(1000);
                    }
                    else {
                        printf("Przedmiot przeniesiony.\n");
                        Sleep(1000);
                    }
                }
                else {
                    printf("Nie ma przedmiotu na tej pozycji!\n");
                    Sleep(1000);
                }
            }
            else {
                printf("Nieprawidlowa pozycja!\n");
                Sleep(1000);
            }
        }
    }
}

Item* createHealthPotion() {
    Item* potion = (Item*)malloc(sizeof(Item));
    strcpy_s(potion->name, 50, "Potion of Health");
    potion->width = 1;
    potion->height = 1;
    potion->symbol = 'H';
    potion->isEquipped = 0;
    potion->posX = -1;
    potion->posY = -1;
    potion->attackBonus = 0;
    potion->defenseBonus = 0;
    potion->healthBonus = 30;
    return potion;
}

Item* createSword() {
    Item* sword = (Item*)malloc(sizeof(Item));
    strcpy_s(sword->name, 50, "Long Sword");
    sword->width = 1;
    sword->height = 3;
    sword->symbol = 'S';
    sword->isEquipped = 0;
    sword->posX = -1;
    sword->posY = -1;
    sword->attackBonus = rand() % 15 + 5;
    sword->defenseBonus = 0;
    sword->healthBonus = 0;
    return sword;
}

Item* createArmor() {
    Item* armor = (Item*)malloc(sizeof(Item));
    strcpy_s(armor->name, 50, "Plate Armor");
    armor->width = 2;
    armor->height = 3;
    armor->symbol = 'A';
    armor->isEquipped = 0;
    armor->posX = -1;
    armor->posY = -1;
    armor->attackBonus = 0;
    armor->defenseBonus = rand() % 20 + 5;
    armor->healthBonus = 0;
    return armor;
}

void useItem(Player* player, Item* item) {
    if (!item) return;

    player->health += item->healthBonus;
    if (player->health > player->max_health) {
        player->health = player->max_health;
    }
    player->attack += item->attackBonus;
    player->defense += item->defenseBonus;

    if (strstr(item->name, "Potion") != NULL) {
        removeItemFromInventory(player->inventory, item);
    }
}

int isHere(GameWorld* world, int x, int y, int currentEnemies, int currentTraps) {
    // Gracz
    if (world->player->posX == x && world->player->posY == y)
        return 1;

    // zainicjowani przeciwnicy
    for (int i = 0; i < currentEnemies; i++) {
        if (world->enemies[i]->EposX == x && world->enemies[i]->EposY == y)
            return 1;
    }

    // zainicjowane pulapki
    for (int i = 0; i < currentTraps; i++) {
        if (world->traps[i]->posX == x && world->traps[i]->posY == y)
            return 1;
    }
    return 0;
}

int normalAttack(int attack, int defense) {
    int damage = attack / 2 + rand() % (attack / 2 + 1) - defense / 3;
    return (damage < 1) ? 1 : damage;
}

int criticalAttack(int attack, int defense) {
    int baseDamage = attack + rand() % (attack + 1);
    int damage = baseDamage - defense / 4;
    return (damage < 1) ? 1 : damage;
}

// Tworzenie i inicjalizacja gracza
Player* createPlayer() {
    Player* player = (Player*)malloc(sizeof(Player));
    if (!player) {
        printf("Blad alokacji pamieci dla gracza\n");
        exit(1);
    }
    return player;
}

void initPlayer(Player* player) {
    printf("Podaj swoje imie: ");
    scanf_s("%49s", player->name, (unsigned)_countof(player->name));
    player->max_health = 200;
    player->health = player->max_health;
    player->attack = 17;
    player->defense = rand() % 10 + 5;
    player->gold = 15;
    player->posX = 0;
    player->posY = 0;
    player->inventory = createInventory(INVENTORY_WIDTH, INVENTORY_HEIGHT);

}

// Tworzenie i inicjalizacja przeciwnika
Enemy* createEnemy(int x, int y) {
    Enemy* enemy = (Enemy*)malloc(sizeof(Enemy));
    if (!enemy) {
        printf("Blad alokacji pamieci dla przeciwnika\n");
        exit(1);
    }
    initEnemy(enemy, x, y);
    return enemy;
}

void initEnemy(Enemy* enemy, int x, int y) {
    strcpy_s(enemy->name, 50, rand() % 2 ? "Goblin" : "Ork");
    enemy->health = rand() % 50 + 20;
    enemy->attack = rand() % 5 + 5;
    enemy->defense = rand() % 5 + 2;
    enemy->EposX = x;
    enemy->EposY = y;

}

// Tworzenie i inicjalizacja pulapki
Trap* createTrap(int x, int y) {
    Trap* trap = (Trap*)malloc(sizeof(Trap));
    if (!trap) {
        printf("Blad alokacji pamieci dla pulapki\n");
        exit(1);
    }
    initTrap(trap, x, y);
    return trap;
}

void initTrap(Trap* trap, int x, int y) {
    trap->posX = x;
    trap->posY = y;
    trap->damage = rand() % 15 + 5;
    trap->discovered = 0;
    strcpy_s(trap->description, 100, rand() % 2 ? "Kolce" : "Spadajace glazy");
}


void checkTraps(GameWorld* world) {
    for (int i = 0; i < world->trapCount; i++) {
        Trap* trap = world->traps[i];

        if (world->player->posX == trap->posX && world->player->posY == trap->posY && !trap->discovered) {
            printf("Odkryles pulapke: %s!\n", trap->description);
            world->player->health -= trap->damage;
            printf("Zostales ranny! Straciles %d HP.\n", trap->damage);
            Sleep(2000);
            trap->discovered = 1;

            if (world->player->health <= 0) {
                printf("Zostales zabity przez pulapke!\nKoniec gry.\n");
				freeGameWorld(world);
                exit(0);
            }
        }
    }
}

void initMap(GameWorld* world) {
    world->map = (char**)malloc(MAP_HEIGHT * sizeof(char*));
    if (!world->map) {
        printf("Blad alokacji pamieci dla mapy\n");
        exit(1);
    }
    for (int i = 0; i < MAP_HEIGHT; i++) {
        world->map[i] = (char*)malloc(MAP_WIDTH * sizeof(char));
        if (!world->map[i]) {
            printf("Blad alokacji pamieci dla wiersza mapy\n");
            exit(1);
        }
        for (int j = 0; j < MAP_WIDTH; j++) {
            world->map[i][j] = '.';
        }
    }
}

void reloadMap(GameWorld* world) {
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            world->map[i][j] = '.';
        }
    }


    for (int i = 0; i < world->groundItemCount; i++) {
        Item* item = world->groundItems[i];
        world->map[item->posY][item->posX] = 'I';
    }

    for (int i = 0; i < world->trapCount; i++) {
        if (world->traps[i]->discovered) {
            world->map[world->traps[i]->posY][world->traps[i]->posX] = 'T';
        }
    }

    for (int i = 0; i < world->enemyCount; i++) {
        world->map[world->enemies[i]->EposY][world->enemies[i]->EposX] = 'E';
    }

    if (world->portalActive) {
        world->map[world->portalY][world->portalX] = 'O';
    }
    world->map[world->player->posY][world->player->posX] = 'P';
}

void nextLevel(GameWorld* world) {
    if (world->level >= 3) {
        printf("Gratulacje! Ukonczyles wszystkie 3 poziomow gry!\n");
        Sleep(3000);
        freeGameWorld(world);
        exit(0);
    }

    world->level++;
    world->enemiesDefeated = 0;
    world->portalActive = 0;

    // Zwolnienie pamięci starych przeciwników
    for (int i = 0; i < world->enemyCount; i++) {
        free(world->enemies[i]);
    }
    free(world->enemies);

    // Zwolnienie pamięci starych pułapek
    for (int i = 0; i < world->trapCount; i++) {
        free(world->traps[i]);
    }
    free(world->traps);

    // Zwolnienie przedmiotów na ziemi
    for (int i = 0; i < world->groundItemCount; i++) {
        free(world->groundItems[i]);
    }
    world->groundItemCount = 0;

    world->enemyCount = MAX_ENEMIES + world->level / 2;
    world->trapCount = MAX_TRAPS + world->level / 2;

    // Inicjalizacja nowych przeciwników
    world->enemies = (Enemy**)malloc(world->enemyCount * sizeof(Enemy*));
    for (int i = 0; i < world->enemyCount; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (isHere(world, x, y, i, 0));

        world->enemies[i] = createEnemy(x, y);
        world->enemies[i]->health += world->level * 5;
        world->enemies[i]->attack += world->level * 2;
        world->enemies[i]->defense += world->level;
    }

    // Inicjalizacja nowych pułapek
    world->traps = (Trap**)malloc(world->trapCount * sizeof(Trap*));
    for (int i = 0; i < world->trapCount; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (isHere(world, x, y, world->enemyCount, i));

        world->traps[i] = createTrap(x, y);
        world->traps[i]->damage += world->level * 2;
    }

    // Generowanie nowych przedmiotów na ziemi
    int itemsToPlace = 5 + rand() % 6; // 5-10 przedmiotów na nowym poziomie
    for (int i = 0; i < itemsToPlace && world->groundItemCount < MAX_GROUND_ITEMS; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (isHere(world, x, y, world->enemyCount, world->trapCount) ||
            world->map[y][x] == 'I');

        Item* newItem = NULL;
        int itemType = rand() % 100;

        if (itemType < 50) { // 40% szansy na miksturę zdrowia
            newItem = createHealthPotion();
        }
        else if (itemType < 75) { // 40% szansy na miecz
            newItem = createSword();
        }
        else { // 20% szansy na zbroję
            newItem = createArmor();
        }

        addItemToGround(world, newItem, x, y);
    }

    world->player->posX = 0;
    world->player->posY = 0;

    world->player->health = (int)world->player->max_health * 0.8;
    if (world->player->health < 1) world->player->health = 1;

    // Odświeżenie mapy
    reloadMap(world);

    printf("Witaj na poziomie %d! Przeciwnicy sa silniejsi!\n", world->level);
    Sleep(2000);
}


GameWorld* createGameWorld() {
    GameWorld* world = (GameWorld*)malloc(sizeof(GameWorld));
    if (!world) {
        printf("Blad alokacji pamieci dla swiata gry\n");
        exit(1);
    }

    // Inicjalizacja podstawowych parametrów świata
    world->level = 1;
    world->enemiesDefeated = 0;
    world->portalActive = 0;
    world->portalX = 0;
    world->portalY = 0;
    world->totalEnemiesDefeated = 0;
    world->groundItemCount = 0;

    // Inicjalizacja mapy
    initMap(world);

    // Inicjalizacja gracza
    world->player = createPlayer();
    initPlayer(world->player);

    // Inicjalizacja przeciwników
    world->enemyCount = MAX_ENEMIES;
    world->enemies = (Enemy**)malloc(world->enemyCount * sizeof(Enemy*));
    if (!world->enemies) {
        printf("Blad alokacji pamieci dla przeciwnikow\n");
        exit(1);
    }

    // Inicjalizacja pułapek
    world->trapCount = MAX_TRAPS;
    world->traps = (Trap**)malloc(world->trapCount * sizeof(Trap*));
    if (!world->traps) {
        printf("Blad alokacji pamieci dla pulapek\n");
        exit(1);
    }

    // Inicjalizacja przedmiotów na ziemi
    world->groundItems = (Item**)malloc(MAX_GROUND_ITEMS * sizeof(Item*));
    if (!world->groundItems) {
        printf("Blad alokacji pamieci dla przedmiotow na ziemi\n");
        exit(1);
    }

    // Umieszczanie przeciwników na mapie
    for (int i = 0; i < world->enemyCount; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (isHere(world, x, y, i, 0));

        world->enemies[i] = createEnemy(x, y);
    }

    // Umieszczanie pułapek na mapie
    for (int i = 0; i < world->trapCount; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (isHere(world, x, y, world->enemyCount, i));

        world->traps[i] = createTrap(x, y);
    }

    // Generowanie losowych przedmiotów na mapie
    int itemsToPlace = 5 + rand() % 6; // 5-10 przedmiotow na start
    for (int i = 0; i < itemsToPlace && world->groundItemCount < MAX_GROUND_ITEMS; i++) {
        int x, y;
        do {
            x = rand() % MAP_WIDTH;
            y = rand() % MAP_HEIGHT;
        } while (isHere(world, x, y, world->enemyCount, world->trapCount) ||
            world->map[y][x] == 'I');

        Item* newItem = NULL;
        int itemType = rand() % 100;

        if (itemType < 50) { // 50% szansy na miksturę zdrowia
            newItem = createHealthPotion();
        }
        else if (itemType < 75) { // 45% szansy na miecz
            newItem = createSword();
        }
        else { // 25% szansy na zbroję
            newItem = createArmor();
        }

        addItemToGround(world, newItem, x, y);
    }

    // Umieszczenie gracza w lewym górnym rogu
    world->player->posX = 0;
    world->player->posY = 0;

    // Odświeżenie mapy
    reloadMap(world);

    return world;
}

void activatePortal(GameWorld* world) {
    do {
        world->portalX = rand() % MAP_WIDTH;
        world->portalY = rand() % MAP_HEIGHT;
    } while (isHere(world, world->portalX, world->portalY, world->enemyCount, world->trapCount) ||
        (world->player->posX == world->portalX && world->player->posY == world->portalY));

    world->portalActive = 1;
    printf("Pojawil sie magiczny portal prowadzacy do nastepnego poziomu!\n");
    reloadMap(world);
    Sleep(2000);
}

int battle(Player* player, Enemy* enemy, GameWorld* world) {
    system("cls");
    printf("==== WALKA ====\n");
    while (1) {
        printf("Gracz %s: %d/%d HP | Atak: %d | Obrona: %d\n",
            player->name, player->health, player->max_health, player->attack, player->defense);
        printf("Przeciwnik %s: %d HP | Atak: %d | Obrona: %d\n\n",
            enemy->name, enemy->health, enemy->attack, enemy->defense);

        printf("1. Normalny atak\n2. Ucieczka\nWybierz akcje: ");
        int action;
        scanf_s("%d", &action);
        while (getchar() != '\n');

        if (action == 1) {
            // Losowy wybór typu ataku
            AttackFunction attackFunc = (rand() % 100 < 15) ? criticalAttack : normalAttack;
            if (attackFunc == criticalAttack) {
                printf("Wykonujesz atak krytyczny!\n");
            }
            else {
                printf("Wykonujesz atak normalny.\n");
            }

            int damage = attackFunc(player->attack, enemy->defense);
            enemy->health -= damage;
            printf("Zadales %d obrazen!\n", damage);
        }
        else if (action == 2) {
            if (rand() % 2) {
                printf("Udalo ci sie uciec!\n");
                return 0;
            }
            else {
                printf("Nie udalo ci sie uciec!\n");
            }
        }
        else {
            printf("Nieznana akcja. Sprobuj ponownie.\n\n");
            continue;
        }

        if (enemy->health <= 0) {
            printf("Pokonales %s!\n", enemy->name);
            int gold = 10 + rand() % 20;
            player->gold += gold;
            printf("Zdobywasz %d zlota.\n", gold);
            world->enemiesDefeated++;
            world->totalEnemiesDefeated++;

            if (world->enemiesDefeated >= 5 && !world->portalActive) {
                activatePortal(world);
            }
            return 1;
        }

        // Tura przeciwnika - przeciwnik używa normalnego ataku
        int enemyDamage = normalAttack(enemy->attack, player->defense);
        player->health -= enemyDamage;
        printf("%s zadaje %d obrazen!\n", enemy->name, enemyDamage);

        if (player->health <= 0) {
            printf("Zostales pokonany!\nKoniec gry.\n");
            freeGameWorld(world);
            exit(0);
        }

        printf("\nNacisnij Enter, aby kontynuowac...");
        while (getchar() != '\n');
        system("cls");
    }
    return 0;
}


void printMap(GameWorld* world) {
    system("cls");
    printf("Gracz: %s | Poziom: %d | HP: %d/%d | Atak: %d | Obrona: %d | Zloto: %d\n",
        world->player->name, world->level, world->player->health,
        world->player->max_health, world->player->attack, world->player->defense, world->player->gold);
    printf("Pokonani wrogowie: %d/5 (lvl) | %d (total)\n",
        world->enemiesDefeated, world->totalEnemiesDefeated);

    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            printf(" %c ", world->map[i][j]);
        }
        printf("\n");
    }
}


void moveEnemy(Enemy* enemy) {
    int dir = rand() % 4;
    int newX = enemy->EposX;
    int newY = enemy->EposY;

    if (dir == 0) newY--;
    else if (dir == 1) newY++;
    else if (dir == 2) newX--;
    else newX++;

    if (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT) {
        enemy->EposX = newX;
        enemy->EposY = newY;
    }
}


void freeGameWorld(GameWorld* world) {
    if (!world) return;

    for (int i = 0; i < world->groundItemCount; i++) {
        free(world->groundItems[i]);
    }
    free(world->groundItems);

    // Zwolnij ekwipunek
    if (world->player && world->player->inventory) {
        for (int y = 0; y < world->player->inventory->height; y++) {
            for (int x = 0; x < world->player->inventory->width; x++) {
                Item* item = world->player->inventory->items[y][x];
                if (item != NULL && item->posX == x && item->posY == y) {
                    free(item);
                }
            }
        }
        freeInventory(world->player->inventory);
    }

    // Zwolnij gracza
    free(world->player);

    // Zwolnij przeciwników
    for (int i = 0; i < world->enemyCount; i++) {
        free(world->enemies[i]);
    }
    free(world->enemies);

    // Zwolnij pułapki
    for (int i = 0; i < world->trapCount; i++) {
        free(world->traps[i]);
    }
    free(world->traps);

    // Zwolnij mape
    if (world->map) {
        for (int i = 0; i < MAP_HEIGHT; i++) {
            free(world->map[i]);
        }
        free(world->map);
    }

    free(world);
}


void movePlayerAndEnemy(GameWorld* world) {
    char move;
    printf("Ruch (WASD), I - ekwipunek, Z - zapisz gre, P - podnies przedmiot, Q - wyjscie: ");
    scanf_s(" %c", &move);
    while (getchar() != '\n');

    if (move == 'i' || move == 'I') {
        inventoryMenu(world);
        return;
    }
    else if (move == 'q' || move == 'Q') {
        freeGameWorld(world);
        exit(0);
    }
    else if (move == 'z' || move == 'Z') {
        saveGame(world);
        return;
    }
    else if (move == 'p' || move == 'P') {
        for (int i = 0; i < world->groundItemCount; i++) {
            if (world->player->posX == world->groundItems[i]->posX &&
                world->player->posY == world->groundItems[i]->posY) {

                Item* newItem = (Item*)malloc(sizeof(Item));
                if (!newItem) {
                    printf("Błąd alokacji pamięci dla przedmiotu!\n");
                    Sleep(1000);
                    return;
                }
                memcpy(newItem, world->groundItems[i], sizeof(Item));

                int added = 0;
                for (int y = 0; y < world->player->inventory->height && !added; y++) {
                    for (int x = 0; x < world->player->inventory->width && !added; x++) {
                        if (canPlaceItem(world->player->inventory, newItem, x, y)) {
                            if (addItemToInventory(world->player->inventory, newItem, x, y)) {
                                printf("Podniesiono %s!\n", newItem->name);
                                added = 1;
                                
                                free(world->groundItems[i]);
                                for (int j = i; j < world->groundItemCount - 1; j++) {
                                    world->groundItems[j] = world->groundItems[j + 1];
                                }
                                world->groundItemCount--;
                            }
                        }
                    }
                }

                if (!added) {
                    printf("Nie masz miejsca w ekwipunku na %s!\n", newItem->name);
                    free(newItem); 
                }

                Sleep(1000);
                reloadMap(world);
                return;
            }
        }
        printf("Nie ma przedmiotu do podniesienia!\n");
        Sleep(1000);
        return;
    }

    // Ruch gracza
    int newX = world->player->posX;
    int newY = world->player->posY;

    if (move == 'w' || move == 'W') newY--;
    else if (move == 's' || move == 'S') newY++;
    else if (move == 'a' || move == 'A') newX--;
    else if (move == 'd' || move == 'D') newX++;

    if (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT) {
        world->player->posX = newX;
        world->player->posY = newY;

        // Sprawdź portal
        if (world->portalActive && world->player->posX == world->portalX && world->player->posY == world->portalY) {
            nextLevel(world);
            return;
        }

        checkTraps(world);
    }

    // Ruch przeciwników
    for (int i = 0; i < world->enemyCount; i++) {
        if (rand() % 2) moveEnemy(world->enemies[i]);
    }

    // Walka z przeciwnikami
    for (int i = 0; i < world->enemyCount; i++) {
        if (world->player->posX == world->enemies[i]->EposX &&
            world->player->posY == world->enemies[i]->EposY) {

            printf("Znalazles przeciwnika: %s!\n", world->enemies[i]->name);
            if (battle(world->player, world->enemies[i], world)) {
                // Szansa na drop przedmiotu
                int dropChance = rand() % 100;
                Item* droppedItem = NULL;

                if (dropChance < 90) {
                    int itemType = rand() % 100;
                    Item* droppedItem = NULL;

                    if (itemType < 60) {
                        droppedItem = createHealthPotion();
                        printf("Przeciwnik upuscil miksture zdrowia!\n");
                    }
                    else if (itemType < 90) {
                        droppedItem = createSword();
                        printf("Przeciwnik upuscil miecz!\n");
                    }
                    else {
                        droppedItem = createArmor();
                        printf("Przeciwnik upuscil zbroje!\n");
                    }

                    int added = 0;
                    for (int y = 0; y < world->player->inventory->height && !added; y++) {
                        for (int x = 0; x < world->player->inventory->width && !added; x++) {
                            if (canPlaceItem(world->player->inventory, droppedItem, x, y)) {
                                if (addItemToInventory(world->player->inventory, droppedItem, x, y)) {
                                    printf("Zdobyto %s!\n", droppedItem->name);
                                    added = 1;
                                }
                            }
                        }
                    }

                    if (!added) {
                        Item* groundItem = (Item*)malloc(sizeof(Item));
                        if (groundItem) {
                            memcpy(groundItem, droppedItem, sizeof(Item));
                            addItemToGround(world, groundItem, world->player->posX, world->player->posY);
                            printf("Położono %s na ziemi (brak miejsca w ekwipunku).\n", groundItem->name);
                        }
                        free(droppedItem);
                    }
                }
                else {
                    printf("Przeciwnik nie upuscil zadnych przedmiotow.\n");
                }

                // Usuń pokonanego przeciwnika
                free(world->enemies[i]);
                for (int j = i; j < world->enemyCount - 1; j++) {
                    world->enemies[j] = world->enemies[j + 1];
                }
                world->enemyCount--;
                i--; 

                Sleep(2000);
            }
        }
    }
    reloadMap(world);
}

void saveGame(GameWorld* world) {
    FILE* file;
    if (fopen_s(&file, "savegame.dat", "wb") != 0) {
        printf("Nie mozna otworzyc pliku do zapisu!\n");
        Sleep(1000);
        return;
    }

    // Zapisz podstawowe informacje o swiecie
    fwrite(&world->level, sizeof(int), 1, file);
    fwrite(&world->enemiesDefeated, sizeof(int), 1, file);
    fwrite(&world->portalActive, sizeof(int), 1, file);
    fwrite(&world->portalX, sizeof(int), 1, file);
    fwrite(&world->portalY, sizeof(int), 1, file);
    fwrite(&world->totalEnemiesDefeated, sizeof(int), 1, file);
    fwrite(&world->enemyCount, sizeof(int), 1, file);
    fwrite(&world->trapCount, sizeof(int), 1, file);

    // Zapisz dane gracza
    fwrite(world->player->name, sizeof(char), 50, file);
    fwrite(&world->player->health, sizeof(int), 1, file);
    fwrite(&world->player->max_health, sizeof(int), 1, file);
    fwrite(&world->player->attack, sizeof(int), 1, file);
    fwrite(&world->player->defense, sizeof(int), 1, file);
    fwrite(&world->player->posX, sizeof(int), 1, file);
    fwrite(&world->player->posY, sizeof(int), 1, file);
    fwrite(&world->player->gold, sizeof(int), 1, file);

    // Zapisz ekwipunek
    Inventory* inv = world->player->inventory;
    fwrite(&inv->width, sizeof(int), 1, file);
    fwrite(&inv->height, sizeof(int), 1, file);

    // Zapisz przedmioty w ekwipunku
    int itemCount = 0;
    for (int y = 0; y < inv->height; y++) {
        for (int x = 0; x < inv->width; x++) {
            if (inv->items[y][x] != NULL && inv->items[y][x]->posX == x && inv->items[y][x]->posY == y) {
                itemCount++;
            }
        }
    }
    fwrite(&itemCount, sizeof(int), 1, file);

    for (int y = 0; y < inv->height; y++) {
        for (int x = 0; x < inv->width; x++) {
            Item* item = inv->items[y][x];
            if (item != NULL && item->posX == x && item->posY == y) {
                fwrite(item, sizeof(Item), 1, file);
            }
        }
    }

    // Zapisz przeciwnikow
    for (int i = 0; i < world->enemyCount; i++) {
        fwrite(world->enemies[i], sizeof(Enemy), 1, file);
    }

    // Zapisz pulapki
    for (int i = 0; i < world->trapCount; i++) {
        fwrite(world->traps[i], sizeof(Trap), 1, file);
    }

    fclose(file);
    printf("Gra zapisana pomyslnie!\n");
    Sleep(1000);
}

GameWorld* loadGame() {
    FILE* file;
    if (fopen_s(&file, "savegame.dat", "rb") != 0) {
        printf("Nie znaleziono zapisu gry!\n");
        Sleep(1000);
        return NULL;
    }

    GameWorld* world = (GameWorld*)malloc(sizeof(GameWorld));
    if (!world) {
        printf("Blad alokacji pamieci dla swiata gry\n");
        fclose(file);
        return NULL;
    }

    // Inicjalizuj wszystkie pola na NULL/0
    memset(world, 0, sizeof(GameWorld));

    // Wczytaj podstawowe informacje o swiecie
    fread(&world->level, sizeof(int), 1, file);
    fread(&world->enemiesDefeated, sizeof(int), 1, file);
    fread(&world->portalActive, sizeof(int), 1, file);
    fread(&world->portalX, sizeof(int), 1, file);
    fread(&world->portalY, sizeof(int), 1, file);
    fread(&world->totalEnemiesDefeated, sizeof(int), 1, file);
    fread(&world->enemyCount, sizeof(int), 1, file);
    fread(&world->trapCount, sizeof(int), 1, file);

    // Wczytaj gracza
    world->player = (Player*)malloc(sizeof(Player));
    fread(world->player->name, sizeof(char), 50, file);
    fread(&world->player->health, sizeof(int), 1, file);
    fread(&world->player->max_health, sizeof(int), 1, file);
    fread(&world->player->attack, sizeof(int), 1, file);
    fread(&world->player->defense, sizeof(int), 1, file);
    fread(&world->player->posX, sizeof(int), 1, file);
    fread(&world->player->posY, sizeof(int), 1, file);
    fread(&world->player->gold, sizeof(int), 1, file);

    // Wczytaj ekwipunek
    int invWidth, invHeight;
    fread(&invWidth, sizeof(int), 1, file);
    fread(&invHeight, sizeof(int), 1, file);
    world->player->inventory = createInventory(invWidth, invHeight);

    // Wczytaj przedmioty
    int itemCount;
    fread(&itemCount, sizeof(int), 1, file);
    for (int i = 0; i < itemCount; i++) {
        Item* item = (Item*)malloc(sizeof(Item));
        fread(item, sizeof(Item), 1, file);
        addItemToInventory(world->player->inventory, item, item->posX, item->posY);
    }

    // Wczytaj przeciwnikow
    world->enemies = (Enemy**)malloc(world->enemyCount * sizeof(Enemy*));
    for (int i = 0; i < world->enemyCount; i++) {
        world->enemies[i] = (Enemy*)malloc(sizeof(Enemy));
        fread(world->enemies[i], sizeof(Enemy), 1, file);
    }

    // Wczytaj pulapki
    world->traps = (Trap**)malloc(world->trapCount * sizeof(Trap*));
    for (int i = 0; i < world->trapCount; i++) {
        world->traps[i] = (Trap*)malloc(sizeof(Trap));
        fread(world->traps[i], sizeof(Trap), 1, file);
    }

    // Inicjalizacja mapy
    initMap(world);
    reloadMap(world);

    fclose(file);
    printf("Gra wczytana pomyslnie!\n");
    Sleep(1000);
    return world;
}

int main() {
    srand((unsigned)time(NULL));
    GameWorld* world = NULL;

    printf("1. Nowa gra\n2. Wczytaj gre\nWybierz: ");
    int choice;
    scanf_s("%d", &choice);
    while (getchar() != '\n');

    if (choice == 1) {
        world = createGameWorld();
    }
    else if (choice == 2) {
        world = loadGame();
        if (world == NULL) {
            printf("Tworzenie nowej gry...\n");
            Sleep(1000);
            world = createGameWorld();
        }
    }
    else {
        printf("Nieprawidlowy wybor. Tworzenie nowej gry...\n");
        Sleep(1000);
        world = createGameWorld();
    }

    while (1) {
        printMap(world);
        movePlayerAndEnemy(world);
    }

    return 0;
}
