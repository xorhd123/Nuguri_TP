# .antigravityrules
1. 모든 대답은 '한국어'로 해주세요.
2. 코드를 수정할 때는 기존 스타일(Prettier 설정)을 반드시 따르세요.
# .antigravityrules
1. 모든 대답은 '한국어'로 해주세요.
2. 코드를 수정할 때는 기존 스타일(Prettier 설정)을 반드시 따르세요.
3. 새로운 라이브러리를 설치할 때는 반드시 허락을 구하세요.
4. 주석은 친절하고 구체적으로 달아주세요.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 크로스 플랫폼 헤더 포함
// Windows와 리눅스/macOS는 시스템 API가 다르기 때문에 분기 처리가 필요합니다.
#ifdef _WIN32
    #include <windows.h> // Windows API 사용을 위한 헤더
    #include <conio.h>   // Windows 콘솔 입출력 함수 (_getch, _kbhit 등)
#else
    #include <unistd.h>  // POSIX 운영체제 API (usleep 등)
    #include <termios.h> // 터미널 I/O 설정 (raw 모드 등)
#endif
#include <fcntl.h>
#include <time.h>

// 맵 및 게임 요소 정의 (수정된 부분)
#define MAP_WIDTH 40  // 맵 너비를 40으로 변경
#define MAP_HEIGHT 20
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
char map[MAX_STAGES][MAP_HEIGHT][MAP_WIDTH + 1];
int player_x, player_y;
int stage = 0;
int score = 0;

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
// 터미널 설정을 저장하기 위한 변수
// 프로그램 종료 시 원래 터미널 설정으로 복구하기 위해 사용합니다.
#ifdef _WIN32
    DWORD orig_mode; // Windows 콘솔 모드 저장 변수
#else
    struct termios orig_termios; // POSIX 터미널 속성 저장 구조체
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

int main() {
    // Windows 콘솔의 코드 페이지를 UTF-8(65001)로 변경합니다.
    // 한글이나 특수 문자가 깨지지 않고 올바르게 출력되도록 돕습니다.
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    srand(time(NULL));
    enable_raw_mode();
    load_maps();
    init_stage();

    char c = '\0';
    int game_over = 0;

    while (!game_over && stage < MAX_STAGES) {
        if (kbhit()) {
            // 키 입력 처리
            // Windows는 _getch()를 사용하여 버퍼 없이 바로 입력을 받습니다.
            // 리눅스/macOS는 getchar()를 사용하지만, 미리 설정한 Raw 모드 덕분에 버퍼 없이 동작합니다.
            #ifdef _WIN32
                c = _getch();
            #else
                c = getchar();
            #endif
            if (c == 'q') {
                game_over = 1;
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
        draw_game();
        // 게임 루프 속도 조절 (딜레이)
        // Windows의 Sleep은 밀리초(ms) 단위이고, POSIX의 usleep은 마이크로초(us) 단위입니다.
        // 90ms = 90000us
        #ifdef _WIN32
            Sleep(90);
        #else
            usleep(90000);
        #endif

        if (map[stage][player_y][player_x] == 'E') {
            stage++;
            score += 100;
            if (stage < MAX_STAGES) {
                init_stage();
            } else {
                game_over = 1;
                printf("\x1b[2J\x1b[H");
                printf("축하합니다! 모든 스테이지를 클리어했습니다!\n");
                printf("최종 점수: %d\n", score);
            }
        }
    }

    disable_raw_mode();
    return 0;
}


// 터미널 Raw 모드 활성화/비활성화
// 터미널 Raw 모드 활성화/비활성화
// Raw 모드란? 사용자가 키를 누르자마자 프로그램이 반응하도록 하고(Line buffering 비활성화),
// 입력한 키가 화면에 바로 보이지 않게(Echo 비활성화) 하는 설정입니다.

#ifdef _WIN32
    // Windows 환경에서의 Raw 모드 설정
    void disable_raw_mode() {
        HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(hin, orig_mode); // 원래 모드로 복구
    }

    void enable_raw_mode() {
        HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hin, &orig_mode); // 현재 콘솔 모드 저장
        atexit(disable_raw_mode); // 프로그램 종료 시 자동으로 disable_raw_mode 호출
        DWORD mode = orig_mode;
        // ENABLE_ECHO_INPUT: 입력한 키를 화면에 표시하지 않음
        // ENABLE_LINE_INPUT: 엔터 키를 누르지 않아도 입력을 받음
        mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        SetConsoleMode(hin, mode); // 변경된 모드 적용
    }
#else
    // 리눅스/macOS 환경에서의 Raw 모드 설정
    void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); } // 원래 설정으로 복구
    void enable_raw_mode() {
        tcgetattr(STDIN_FILENO, &orig_termios); // 현재 터미널 속성 저장
        atexit(disable_raw_mode); // 프로그램 종료 시 복구 함수 등록
        struct termios raw = orig_termios;
        // ECHO: 입력 문자 화면 출력 비활성화
        // ICANON: 라인 버퍼링 비활성화 (엔터 없이 즉시 입력)
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // 변경된 속성 적용
}
#endif

// 맵 파일 로드
void load_maps() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }
    int s = 0, r = 0;
    char line[MAP_WIDTH + 2]; // 버퍼 크기는 MAP_WIDTH에 따라 자동 조절됨
    while (s < MAX_STAGES && fgets(line, sizeof(line), file)) {
        if ((line[0] == '\n' || line[0] == '\r') && r > 0) {
            s++;
            r = 0;
            continue;
        }
        if (r < MAP_HEIGHT) {
            line[strcspn(line, "\n\r")] = 0;
            strncpy(map[s][r], line, MAP_WIDTH + 1);
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

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S') {
                player_x = x;
                player_y = y;
            } else if (cell == 'X' && enemy_count < MAX_ENEMIES) {
                enemies[enemy_count] = (Enemy){x, y, (rand() % 2) * 2 - 1};
                enemy_count++;
            } else if (cell == 'C' && coin_count < MAX_COINS) {
                coins[coin_count++] = (Coin){x, y, 0};
            }
        }
    }
}

// 게임 화면 그리기
void draw_game() {
    printf("\x1b[2J\x1b[H");
    printf("Stage: %d | Score: %d\n", stage + 1, score);
    printf("조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");

    char display_map[MAP_HEIGHT][MAP_WIDTH + 1];
    for(int y=0; y < MAP_HEIGHT; y++) {
        for(int x=0; x < MAP_WIDTH; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S' || cell == 'X' || cell == 'C') {
                display_map[y][x] = ' ';
            } else {
                display_map[y][x] = cell;
            }
        }
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

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for(int x=0; x< MAP_WIDTH; x++){
            printf("%c", display_map[y][x]);
        }
        printf("\n");
    }
}

// 게임 상태 업데이트
void update_game(char input) {
    move_player(input);
    move_enemies();
    check_collisions();
}

// 플레이어 이동 로직
void move_player(char input) {
    int next_x = player_x, next_y = player_y;
    char floor_tile = (player_y + 1 < MAP_HEIGHT) ? map[stage][player_y + 1][player_x] : '#';
    char current_tile = map[stage][player_y][player_x];

    on_ladder = (current_tile == 'H');

    switch (input) {
        case 'a': next_x--; break;
        case 'd': next_x++; break;
        case 'w': if (on_ladder) next_y--; break;
        case 's': if (on_ladder && (player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] != '#') next_y++; break;
        case ' ':
            if (!is_jumping && (floor_tile == '#' || on_ladder)) {
                is_jumping = 1;
                velocity_y = -2;
            }
            break;
    }

    if (next_x >= 0 && next_x < MAP_WIDTH && map[stage][player_y][next_x] != '#') player_x = next_x;
    
    if (on_ladder && (input == 'w' || input == 's')) {
        if(next_y >= 0 && next_y < MAP_HEIGHT && map[stage][next_y][player_x] != '#') {
            player_y = next_y;
            is_jumping = 0;
            velocity_y = 0;
        }
    } 
    else {
        if (is_jumping) {
            next_y = player_y + velocity_y;
            if(next_y < 0) next_y = 0;
            velocity_y++;

            if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][next_y][player_x] == '#') {
                velocity_y = 0;
            } else if (next_y < MAP_HEIGHT) {
                player_y = next_y;
            }
            
            if ((player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] == '#') {
                is_jumping = 0;
                velocity_y = 0;
            }
        } else {
            if (floor_tile != '#' && floor_tile != 'H') {
                 if (player_y + 1 < MAP_HEIGHT) player_y++;
                 else init_stage();
            }
        }
    }
    
    if (player_y >= MAP_HEIGHT) init_stage();
}


// 적 이동 로직
void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        if (next_x < 0 || next_x >= MAP_WIDTH || map[stage][enemies[i].y][next_x] == '#' || (enemies[i].y + 1 < MAP_HEIGHT && map[stage][enemies[i].y + 1][next_x] == ' ')) {
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
            init_stage();
            return;
        }
    }
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y) {
            coins[i].collected = 1;
            score += 20;
        }
    }
}

// 비동기 키보드 입력 확인
// 비동기 키보드 입력 확인 함수
// 키보드가 눌렸는지 확인만 하고, 눌리지 않았으면 바로 0을 반환하여 게임 루프가 멈추지 않게 합니다.
int kbhit() {
    #ifdef _WIN32
        return _kbhit(); // Windows는 conio.h의 _kbhit() 함수가 이 기능을 제공합니다.
    #else
        // 리눅스/macOS는 표준 라이브러리에 kbhit()이 없어서 직접 구현해야 합니다.
        struct termios oldt, newt;
        int ch;
        int oldf;
        
        // 1. 현재 터미널 설정 백업 및 변경 (Non-canonical 모드, Echo 끄기)
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        
        // 2. 파일 상태 플래그 변경 (Non-blocking 모드 설정)
        // 입력이 없으면 기다리지 않고 바로 리턴하도록 설정
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        
        // 3. 문자 읽기 시도
        ch = getchar();
        
        // 4. 터미널 설정 및 파일 상태 복구
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        
        // 5. 문자가 읽혔다면 다시 입력 버퍼에 넣고 1 반환 (키 눌림 감지)
        if(ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    #endif
}