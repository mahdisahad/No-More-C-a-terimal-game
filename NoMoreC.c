#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#define WID 80
#define HT 20
#define D_H 3
#define D_W 5
#define O_W 1
#define O_H 2
#define J_H 4
#define REF_T 100000

typedef struct {
int x;
int y;
int w;
int h;
} Obs;

typedef struct {
int x;
int y;
int jump;
int j_height;
int orig_y;
struct timeval jump_start;
struct timeval peak_time;
} D;

typedef struct {
int x;
int y;
} S;

void bdr() {
    for (int xx=0; xx <WID; xx++) {
        mvaddch(0, xx,'-');
        mvaddch(HT - 1, xx, '-');
    }
    for (int yy = 0; yy < HT; yy++) {
        mvaddch(yy,0, '|');
        mvaddch(yy, WID-1, '|');
    }
}

void dino(D *d) {
    if (d->jump) {
        mvprintw(d->y- d->j_height, d->x, " ^_^ ");
        mvprintw(d->y- d->j_height+1, d->x, " /|\\ ");
        mvprintw(d->y -d->j_height+ 2, d->x, " / \\ ");
    } else {
        mvprintw(d->y, d->x, " >_< ");
        mvprintw(d->y + 1, d->x, " /|\\ ");
        mvprintw(d->y + 2, d->x, " / \\ ");
    }
}

void obstacle(Obs *o) {
    for (int i = 0; i < o->h; i++) {
        for (int j = 0; j < o->w; j++) {
            mvprintw(o->y + i, o->x + j, "#");
        }
    }
}

void background(S *stars) {
   mvprintw(1, 1, "                                                |>>>");
    mvprintw(2, 1, "                                            _  _|_  _");
    mvprintw(3, 1, "                                           |;|_|;|_|;|");
    mvprintw(4, 1, "                                           \\.    .  /");
    mvprintw(5, 1, "                                            \\:  .  /");
    mvprintw(6, 1, "                                             ||:   |");
    mvprintw(7, 1, "                                             ||:.  |");
    mvprintw(8, 1, "                                             ||:   |");
    mvprintw(9, 1, "                                             ||: , |");
    mvprintw(10, 1, "                                             ||: . |");
    mvprintw(11, 1, "              __                            _||_   |");
    mvprintw(12, 1, "     ____--`~    '--~~__            __ ----~    ~`---,              ___");
    mvprintw(13, 1, "-~--~                   ~---__ ,--~'                  ~~----_____-~'   `~----~~");

    for (int i = 0; i < 25; i++) {
        mvprintw(stars[i].y, stars[i].x, "*");
    }
}

void add_obs(Obs *o, int *num) {
    if (*num < 5) {
        int n_x = WID - 2;
        int o_type = rand() % 2;
        o[*num].x = n_x;
        o[*num].y = HT - 3;
        o[*num].w = (o_type == 0) ? O_W : 2;
        o[*num].h = O_H;
        (*num)++;
    }
}

void update_obs(Obs *o, int *num) {
    for (int i = 0; i < *num; i++) {
        o[i].x--;
        if (o[i].x < 1) {
            for (int j = i; j < *num - 1; j++) {
                o[j] = o[j + 1];
            }
            (*num)--;
            i--;
        }
    }
}

int check_collision(D *d, Obs *o, int num) {
    if (d->jump) return 0;

    for (int i = 0; i < num; i++) {
        if (d->x + D_W > o[i].x && d->x < o[i].x + o[i].w &&
            d->y + D_H >= o[i].y) {
            return 1;
        }
    }
    return 0;
}

void gameover(Mix_Chunk *gameover_sound) {
    Mix_HaltMusic();
    Mix_PlayChannel(-1, gameover_sound, 0);
    mvprintw(HT / 2, WID / 2 - 20, "GAME OVER! You have to Code in C forever!");
    refresh();
    usleep(3000000);
}

void player_info(char *name, struct timeval st_time) {
    struct timeval c_time;
    gettimeofday(&c_time, NULL);
    long elapsed_us = (c_time.tv_sec - st_time.tv_sec) * 1000000 + (c_time.tv_usec - st_time.tv_usec);
    int sec = elapsed_us / 1000000;
    int msec = (elapsed_us % 1000000) / 1000;
    mvprintw(1, WID - 15, "Player: %s", name);
    mvprintw(2, WID - 15, "Time: %d.%03d", sec, msec);
}

void generate_stars(S *s) {
    for (int i = 0; i < 25; i++) {
        s[i].x = rand() % WID;
        s[i].y = rand() % (HT - 2);
    }
}

int main() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return 1;
    }

    Mix_Chunk *jump_snd = Mix_LoadWAV("jump_sound.wav");
    Mix_Chunk *gameover_snd = Mix_LoadWAV("game_over_sound.wav");

    if (!jump_snd || !gameover_snd) {
        fprintf(stderr, "Error loading sound: %s\n", Mix_GetError());
        return 1;
    }

    Mix_Music *bg_music = Mix_LoadMUS("background_music.wav");
    if (!bg_music) {
        fprintf(stderr, "Error loading background music: %s\n", Mix_GetError());
        return 1;
    }

    Mix_PlayMusic(bg_music, -1);

    srand(time(NULL));
    initscr();
    noecho();
    curs_set(0);
    timeout(100);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK); 
        init_pair(2, COLOR_YELLOW, COLOR_BLACK); 
        init_pair(3, COLOR_GREEN, COLOR_BLACK); 
        init_pair(4, COLOR_BLUE, COLOR_BLACK); 
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK); 
        init_pair(6, COLOR_CYAN, COLOR_BLACK); 
        init_pair(7, COLOR_WHITE, COLOR_BLACK); 
    }

    attron(A_BOLD);
    mvprintw(HT / 2 - 4, WID / 2 - 10, "Welcome to No more 'C' !\n\n");
    attroff(A_BOLD);

    mvprintw(HT / 2 - 3, WID / 2 - 30, "Your mission is to escape the '#'s that make your life hell.");
    refresh();

    mvprintw(HT / 2 - 2, WID / 2 - 30, "If you lose, you'll have to #include C in every moment of your life!");
    refresh();

    mvprintw(HT / 2 - 1, WID / 2 - 10, "Enter your name: ");
    refresh();

    timeout(-1);
    echo();
    char player_name[50];
    getstr(player_name);
    noecho();
    timeout(100);

    clear();
    mvprintw(HT / 2 - 2, WID / 2 - 10, "Press Enter to start...");
    refresh();

    mvprintw(HT / 2 - 1, WID / 2 - 25, "Pro-tip: hold space to jump over difficult obstacles!");
    refresh();

    while (1) {
        int ch = getch();
        if (ch == '\n') break;
    }

    S stars[25];
    generate_stars(stars);

    D d = {5, HT - 3, 0, 0, HT - 3, {0}, {0}};
    Obs o[5];
    int num = 0;
    struct timeval last_jump = {0};
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    int last_color_time = 0;
    int color_idx = 0;
    int colors[] = {1, 2, 3, 4, 5, 6, 7};

    while (1) {
        clear();
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        long elapsed_us = (current_time.tv_sec - start_time.tv_sec) * 1000000 + (current_time.tv_usec - start_time.tv_usec);
        int elapsed_secs = elapsed_us / 1000000;

        if (elapsed_secs - last_color_time >= 1) {
            color_idx = (color_idx + 1) % 7;
            last_color_time = elapsed_secs;
        }

        bkgd(COLOR_PAIR(colors[color_idx]));

        background(stars);
        bdr();
        dino(&d);
        for (int i = 0; i < num; i++) {
            obstacle(&o[i]);
        }

        player_info(player_name, start_time);

        int ch = getch();
        if (ch == 'q') break;

        if (ch == ' ' && !d.jump) {
            struct timeval current_time;
            gettimeofday(&current_time, NULL);
            d.jump = 1;
            d.j_height = 0;
            d.jump_start = current_time;
            last_jump = current_time;

            Mix_PlayChannel(-1, jump_snd, 0);
        }

        if (d.jump) {
            struct timeval jump_time;
            gettimeofday(&jump_time, NULL);
            long jump_duration_us = (jump_time.tv_sec - d.jump_start.tv_sec) * 1000000 + (jump_time.tv_usec - d.jump_start.tv_usec);

            if (jump_duration_us < 125000) {
                d.j_height = (float)jump_duration_us / 125000 * J_H;
                d.y = d.orig_y - d.j_height;
            } else if (jump_duration_us < 900000) {
                if (d.peak_time.tv_sec == 0) {
                    gettimeofday(&d.peak_time, NULL);
                }
                d.j_height = J_H;
                d.y = d.orig_y - d.j_height;
            } else {
                long descent_duration_us = jump_duration_us - 900000;
                if (descent_duration_us < 125000) {
                    d.j_height = (float)(125000 - descent_duration_us) / 125000 * J_H;
                    d.y = d.orig_y - d.j_height;
                } else {
                    d.jump = 0;
                    d.j_height = 0;
                    d.y = d.orig_y;
                }
            }
        }

        update_obs(o, &num);

        if (rand() % 100 < 5) {
            add_obs(o, &num);
        }

        if (!d.jump) {
            if (check_collision(&d, o, num)) {
                gameover(gameover_snd);
                break;
            }
        }

        refresh();
    }

    Mix_FreeChunk(jump_snd);
    Mix_FreeChunk(gameover_snd);
    Mix_FreeMusic(bg_music);
    Mix_CloseAudio();
    SDL_Quit();

    endwin();
    return 0;
}
