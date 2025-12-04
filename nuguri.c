#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
#endif
#include <fcntl.h>
#include <time.h>

// 맵 및 게임 요소 정의 (수정된 부분)
#define MAX_STAGES 2
#define MAX_ENEMIES 15 // 최대 적 개수 증가
#define MAX_COINS 30   // 최대 코인 개수 증가

// 구조체 정의
typedef struct {
    int x, y;
    int dir; // 1: right, -1: left
} Enemy;

typedef struct {
    int x, y;
    int collected;
} Coin;

// 전역 변수
char ***map;
int map_width = 0;
int map_height = 0;
int player_x, player_y;
int stage = 0;
int score = 0;
int life = 3;


// 플레이어 상태
int is_jumping = 0;
int velocity_y = 0;
int on_ladder = 0;

// 게임 객체
Enemy enemies[MAX_ENEMIES];
int enemy_count = 0;
Coin coins[MAX_COINS];
int coin_count = 0;

// 터미널 설정
#ifdef _WIN32
    DWORD orig_mode;
#else
    struct termios orig_termios;
#endif
// 함수 선언
void disable_raw_mode();
void enable_raw_mode();
void load_maps();
void init_stage();
void draw_game();
void update_game(char input);
void move_player(char input);
void move_enemies();
void check_collisions();
int kbhit();

// 화면 지우기
void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        printf("\x1b[2J\x1b[H");
    #endif
}

void show_title_screen() {

    clear_screen();

    printf("\n\n");
    printf("  =======================================================\n");
    printf("  |                                                     |\n");
    printf("  |    _   _   _   _   _   _         _   _   _   _      |\n");
    printf("  |   / \\ / \\ / \\ / \\ / \\ / \\       / \\ / \\ / \\ / \\     |\n");
    printf("  |  ( N | U | G | U | R | I )     ( G | A | M | E )    |\n");
    printf("  |   \\_/ \\_/ \\_/ \\_/ \\_/ \\_/       \\_/ \\_/ \\_/ \\_/     |\n");
    printf("  |                                                     |\n");
    printf("  =======================================================\n");
    printf("\n");
    printf("               너구리의 모험을 시작합니다  \n");
    printf("\n");
    printf("        [조작법]\n");
    printf("        ←/→/↑/↓ : 이동 및 사다리 타기\n");
    printf("        SPACE   : 점프\n");
    printf("        q       : 게임 종료\n");
    printf("\n");
    printf("    ===================================================\n");
    printf("             Press Enter to Start Game... \n");
    printf("    ===================================================\n");

    // 엔터 키 입력 대기
    while (getchar() != '\n');
}

// is_clear: 1이면 클리어, 0이면 게임 오버
void show_ending_screen(int is_clear, int final_score) {
    
    clear_screen();

    printf("\n\n");
    if (is_clear == 1) {
        // 게임 클리어 화면
        printf("           /\\___/\\   \n");
        printf("          (  o o  )  <  축하합니다! \n");
        printf("          /   *   \\     너구리가 집으로 돌아갔어요!\n");
        printf("          \\__\\_/__/  \n");
        printf("            /   \\    \n");
        printf("           (     )     \n");
        printf("           /     \\    \n");
        printf("\n");
        printf("    모든 스테이지를 정복하셨습니다!\n");
    } else if (is_clear == 0) {
        // 게임 오버 화면
        printf("            G A M E  O V E R       \n");
        printf("    ===============================\n");
        printf("    너구리가 지쳐서 쓰러졌습니다...\n");
        printf("    다시 도전해보세요!\n");
        printf("    ===============================\n");
    }

    else{
        printf("\n");
        printf("    ===============================\n");
        printf("           최종 점수 : %d 점\n", final_score);
        printf("    ===============================\n");
        printf("\n");
        printf("    아무 키나 누르면 종료합니다...\n");
    }
    
    // 종료 전 대기 
    getchar(); 
}

int main() {
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    srand(time(NULL));
    enable_raw_mode();
    load_maps();
    show_title_screen();
    init_stage();

    char c = '\0';
    int game_over = 0;
    int is_clear = 0; //0:게임오버, 1:게임클리어

    while (!game_over && stage < MAX_STAGES) {
        if (kbhit()) {
            #ifdef _WIN32
                c = _getch();
            #else
                c = getchar();
            #endif
            if (c == 'q') {
                game_over = 1;
                is_clear = 2;
                continue;
            }
            if (c == '\x1b') {
                getchar(); // '['
                switch (getchar()) {
                    case 'A': c = 'w'; break; // Up
                    case 'B': c = 's'; break; // Down
                    case 'C': c = 'd'; break; // Right
                    case 'D': c = 'a'; break; // Left
                }
            }
        } else {
            c = '\0';
        }

        update_game(c);
	if (life == 0){
		game_over = 1;
	}
        draw_game();
        #ifdef _WIN32
            Sleep(140);
        #else
            usleep(140000);
        #endif

        if (map[stage][player_y][player_x] == 'E') {
            stage++;
            score += 100;
            if (stage < MAX_STAGES) {
                init_stage();
            } else {
                game_over = 1;
                is_clear = 1;
                printf("\x1b[2J\x1b[H");
                printf("축하합니다! 모든 스테이지를 클리어했습니다!\n");
                printf("최종 점수: %d\n", score);
            }
        }
    }

    disable_raw_mode();
    show_ending_screen(is_clear, score);
    return 0;
}


// 터미널 Raw 모드 활성화/비활성화
#ifdef _WIN32
    void disable_raw_mode() {
        HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(hin, orig_mode);
    }

    void enable_raw_mode() {
        HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hin, &orig_mode); // tcsetattr 대응
        atexit(disable_raw_mode);
        DWORD mode = orig_mode;
        mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        SetConsoleMode(hin, mode);
    }
#else
    void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }
    void enable_raw_mode() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        atexit(disable_raw_mode);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
#endif

// 맵 파일 로드
void load_maps() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }

    // 맵 한 줄을 읽을 임시 버퍼
    // 대략 최대 50 X 50 크기의 맵을 저장 가능
    char line[2600];

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n\r")] = 0; // 줄 바꿈 문자를 제거
        if (strlen(line) > 0 && line[0] != '\n') {
            map_width = strlen(line);
            break;
        }
    }
    
    rewind(file);
    int stage_rows = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n\r")] = 0;
        
        if (line[0] == '\n') continue;

        // 빈 줄을 스테이지 구분자로 간주
        if (strlen(line) == 0) {
            break;
        }
        else {
            if (strlen(line) == map_width) {
                 stage_rows++;
            }
        }
    }

    map_height = stage_rows;
    
    if (map_width <= 0 || map_height <= 0) {
        fprintf(stderr, "오류: 맵 크기를 파일에서 결정할 수 없습니다.\n");
        fclose(file);
        exit(1);
    }


    map = (char***)malloc(MAX_STAGES * sizeof(char**));
    if (!map) { perror("메모리 할당 실패 (스테이지)"); fclose(file); exit(1); }

    for (int s = 0; s < MAX_STAGES; s++) {

        map[s] = (char**)malloc(map_height * sizeof(char*));

        if (!map[s]) { perror("메모리 할당 실패 (맵 높이)"); fclose(file); exit(1); }

        for (int r = 0; r < map_height; r++) {
            map[s][r] = (char*)malloc((map_width + 1) * sizeof(char));
            if (!map[s][r]) { perror("메모리 할당 실패 (맵 너비)"); fclose(file); exit(1); }
        }
    }

    // 파일 포인터를 다시 처음으로 되돌리고 맵 데이터 읽기
    rewind(file);
    int s = 0, r = 0;

    while (s < MAX_STAGES && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n\r")] = 0;

        if (line[0] == '\n') continue;
        
        // 빈 줄이 나오고, 현재 스테이지에 행이 있다면 다음 스테이지로
        if (strlen(line) == 0 && r > 0) {
            s++;
            r = 0;
            continue;
        }

        if (r < map_height && strlen(line) == map_width) {
            strncpy(map[s][r], line, map_width);
            map[s][r][map_width] = '\0';
            r++;
        }
    }
    
    fclose(file);
}


// 현재 스테이지 초기화
void init_stage() {
    enemy_count = 0;
    coin_count = 0;
    is_jumping = 0;
    velocity_y = 0;

    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S') {
                player_x = x;
                player_y = y;
            }
            else if (cell == 'X' && enemy_count < MAX_ENEMIES) {
                enemies[enemy_count] = (Enemy){x, y, (rand() % 2) * 2 - 1};
                enemy_count++;
            }
            else if (cell == 'C' && coin_count < MAX_COINS) {
                coins[coin_count++] = (Coin){x, y, 0};
            }
        }
    }
}

// 게임 화면 그리기
void draw_game() {
    printf("\x1b[2J\x1b[H");
    printf("Stage: %d | Score: %d | Life: %d\n", stage + 1, score, life);
    printf("조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");

    char **display_map = (char**)malloc(map_height * sizeof(char*));
    if (display_map == NULL) {
        perror("메모리 할당 실패 (display_map 행)");
        return;
    }

    for(int y = 0; y < map_height; y++) {
        display_map[y] = (char*)malloc((map_width + 1) * sizeof(char));
        if (display_map[y] == NULL) {
            perror("메모리 할당 실패 (display_map 열)");
            return;
        }

        for(int x = 0; x < map_width; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S' || cell == 'X' || cell == 'C') {
                display_map[y][x] = ' ';
            }
            else {
                display_map[y][x] = cell;
            }
        }
        display_map[y][map_width] = '\0'; 
    }
    
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected) {
            display_map[coins[i].y][coins[i].x] = 'C';
        }
    }

    for (int i = 0; i < enemy_count; i++) {
        display_map[enemies[i].y][enemies[i].x] = 'X';
    }

    display_map[player_y][player_x] = 'P';

    for (int y = 0; y < map_height; y++) {
        printf("%s\n", display_map[y]);
    }

    printf("\x1b[%d;1H", map_height + 3);

    for (int y = 0; y < map_height; y++) {
        free(display_map[y]);
    }
    free(display_map);
}

// 게임 상태 업데이트
void update_game(char input) {
    move_player(input);
    move_enemies();
    check_collisions();
}

void move_player(char input) {
    int next_x = player_x, next_y = player_y;
    char floor_tile = (player_y + 1 < map_height) ? map[stage][player_y + 1][player_x] : '#';
    char current_tile = map[stage][player_y][player_x];

    on_ladder = (current_tile == 'H');

    switch (input) {
        case 'a': next_x--; break;
        case 'd': next_x++; break;
        case 'w': if (on_ladder) next_y--; break;
        case 's':
            if (on_ladder && (player_y + 1 < map_height) && map[stage][player_y + 1][player_x] != '#') next_y++;
            if (floor_tile == '#' && (player_y + 2 < map_height) && map[stage][player_y + 2][player_x] == 'H'){
                player_y += 2;
            }
            break;
            
        case ' ':
            if (!is_jumping && (floor_tile == '#' || on_ladder)) {
                if(on_ladder && map[stage][player_y - 1][player_x] == '#'){
                    player_y -= 2;
                    break;
                }

                if(!on_ladder){
                    int can_jump = 1;

                    if (player_y > 0 && map[stage][player_y - 1][player_x] == '#') {
                        can_jump = 0;
                    }

                    if (can_jump) { 
                        if(map[stage][player_y - 2][player_x] == '#'){
                            is_jumping = 1;
                            velocity_y = -1;
                        }

                        else{
                            is_jumping = 1;
                            velocity_y = -2;
                        }
                    }
                }
            }
            break;
    }
    
    if (next_x >= 0 && next_x < map_width && map[stage][player_y][next_x] != '#') {
        player_x = next_x;
    }
    
    if (on_ladder && (input == 'w' || input == 's')) {
        if(next_y >= 0 && next_y < map_height && map[stage][next_y][player_x] != '#') {
            player_y = next_y;
            is_jumping = 0; 
            velocity_y = 0;

            return; 
        }
    } 
    
    int is_airborne = !(floor_tile == '#' || on_ladder); 
    
    if (is_jumping || is_airborne) {
        if (!is_jumping && is_airborne) {
            is_jumping = 1;
            velocity_y = 1; 
        }
        
        int target_y = player_y + velocity_y;

        // 점프 이동시 충돌 검사
        if (velocity_y < 0) {
            int next_y_try = target_y;
            int hit_obstacle = 0;
            
            // 이동 경로 (player_y - 1)부터 next_y_try까지의 모든 칸 검사
            for (int y = player_y - 1; y >= next_y_try; y--) {
                if (y < 0) {
                    hit_obstacle = 1; // 맵 상단 경계 충돌
                    break;
                }
                
                char obstacle = map[stage][y][player_x];
                
                if (obstacle == '#') {
                    hit_obstacle = 1;
                    break;
                }
                
                // check_collisions 함수 내의 적 충돌 기능을 한 픽셀씩 이동할때마다 검사하기 위해 move_player 함수 내로 이동
                // 적 충돌 기능이 중복되어도 여기서 적과 충돌하여 init_stage()로 인해 플레이어가 처음 위치로 이동해서 목숨이 2개 감소하는 현상은 없음
                for (int i = 0; i < enemy_count; i++) {
                    if (y == enemies[i].y && player_x == enemies[i].x) {
                        score = (score > 50) ? score - 50 : 0;
                        life--;

                        if (life > 0) {
                            init_stage();
                        }

                        return;
                    }
                } 
            }

            if (hit_obstacle) {
                velocity_y = 0;
                target_y = player_y; // 이동하지 못하도록 목표 위치를 현재 위치로 재설정
            }
        }

        //낙하시 충돌 검사
        else if (velocity_y >= 0) {
            int landed = 0;
            // 이동 경로 (player_y + 1)부터 target_y까지의 모든 칸 검사
            int y_start = player_y; 
            for (int y = y_start + 1; y <= target_y; y++) {
                if (y >= map_height) { 
                    landed = 2; 

                    break;
                }
                
                char obstacle = map[stage][y][player_x];

                if (obstacle == '#' || obstacle == 'H') { 
                    player_y = y - 1; 
                    is_jumping = 0;
                    velocity_y = 0;
                    landed = 1;

                    break;
                }
                
                for (int i = 0; i < enemy_count; i++) {
                    if (y == enemies[i].y && player_x == enemies[i].x) {
                        score = (score > 50) ? score - 50 : 0;
                        life--;

                        if (life > 0) {
                            init_stage();
                        }

                        return;
                    }
                }
            }
            if (landed == 1) return; 
            if (landed == 2) { 
                init_stage(); 

                return;
            }
        }

        player_y = target_y;
        velocity_y++;
    }
    
    if (player_y >= map_height) init_stage();
}


// 적 이동 로직
void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        if (next_x < 0 || next_x >= map_width || map[stage][enemies[i].y][next_x] == '#' || (enemies[i].y + 1 < map_height && map[stage][enemies[i].y + 1][next_x] == ' ')) {
            enemies[i].dir *= -1;
        } else {
            enemies[i].x = next_x;
        }
    }
}

// 충돌 감지 로직
void check_collisions() {
    for (int i = 0; i < enemy_count; i++) {
        if (player_x == enemies[i].x && player_y == enemies[i].y) {
            score = (score > 50) ? score - 50 : 0;
	    life--;
	    if(life> 0){
            init_stage();
	    }
            return;
        }
    }
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y) {
            #ifdef _WIN32
                Beep(500, 200);
            #else
                printf("\a");
                fflush(stdout);
            #endif
            coins[i].collected = 1;
            score += 20;
        }
    }
}

// 비동기 키보드 입력 확인
int kbhit() {
    #ifdef _WIN32
        return _kbhit();
    #else
        struct termios oldt, newt;
        int ch;
        int oldf;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if(ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    #endif
}
