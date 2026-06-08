/* ==========================================================================
 * Project:       Matrix Digital Rain Splash Screen (OS Login & Welcome)
 * Version:       0.1-beta
 * Author:        Production Release Dev Team
 * License:       MIT License
 * Description:   A lightweight, ultra-precise Matrix-style text generation 
 * engine. Synchronizes procedural rain trails with frame-accurate 
 * character-reveals for terminal-based desktop environments.
 * ========================================================================== */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FPS 60
#define BUILD_DURATION_SEC 3
#define HOLD_DURATION_SEC 1
#define OUTRO_DURATION_SEC 1
#define RAIN_DENSITY 0.55f
#define TRAIL_LENGTH_MIN 10
#define TRAIL_LENGTH_MAX 25

typedef struct {
    int col;
    float head;
    float speed;
    int length;
    int z_depth; 
} Trail;

static const char *DEFAULT_TEXT = "WELCOME TO THE MATRIX";

static const char *CHARS_Z0 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const char *CHARS_Z1 = "abcdefghijklmnopqrstuvwxyz123456789";
static const char *CHARS_Z2 = ":;-=+*<>[]{}";
static const char *CHARS_Z3 = "....,,,,``''";

char rc(int depth) {
    if (depth == 0) return CHARS_Z0[rand() % 36];
    if (depth == 1) return CHARS_Z1[rand() % 35];
    if (depth == 2) return CHARS_Z2[rand() % 12];
    return CHARS_Z3[rand() % 12];
}

int main(int argc, char **argv) {
    srand(time(NULL));
    const char *target = (argc > 1) ? argv[1] : DEFAULT_TEXT;

    /* ==========================================
     * CONFIGURATION SECTION (HEX/RGB/SETTINGS)
     * ========================================== */
    short RGB_TEXT[3]   = {1000, 1000, 1000}; 
    short RGB_SHADOW[3] = {200, 200, 200};      
    
    short RGB_Z0[3]     = {900, 900, 900}; 
    short RGB_Z1[3]     = {600, 600, 600};      
    short RGB_Z2[3]     = {400, 400, 400};      
    short RGB_Z3[3]     = {200, 200, 200};      

    /* * SPEED_MODE: Control overall simulation step rates.
     * 1 = Low Speed (0.22f), 2 = Normal Speed (0.50f), 3 = High Speed (0.95f)
     */
    int SPEED_MODE = 3;

    /* * HOLD_EFFECT_MODE: Text behavior while string is fully unlocked.
     * 1 = Active Glitch (Random glyph updates inside decrypted text matrix)
     * 2 = Dynamic Neon Breathing Pulse (Cyclic dimming of localized screen spaces)
     * 3 = Wave Scanline Shimmer (Horizontal scanning highlight propagation)
     * 4 = Binary Flip Noise (Oscillating binary components flickering in background)
     * 5 = Static Frozen Integrity (Locked terminal render without glyph permutations)
     */
    int HOLD_EFFECT_MODE = 4;

    /* * OUTRO_EFFECT_MODE: Animation system applied during screen exit pipeline.
     * 1 = Hardware Instant Hard Cut Shutdown (Direct execution termination)
     * 2 = Vertical Dissolve Melt Downwards (Character terminal drop simulation)
     * 3 = Anti-Gravity Reverse Rain Evaporation (Ascending terminal trail lift)
     * 4 = High-Velocity Kinetic Dispersal Scatter (Multidirectional screen blast)
     * 5 = EMP Digital Noise Burn-out (Systematic decay to noise before blanking)
     * 6 = Horizontal Compression Split (Collapse text matrix outward from row index)
     */
    int OUTRO_EFFECT_MODE = 2; 
    /* ========================================== */

    float base_speed = 0.5f;
    if (SPEED_MODE == 1) base_speed = 0.22f;
    if (SPEED_MODE == 3) base_speed = 0.95f;

    initscr();
    noecho();
    curs_set(0);
    timeout(0);
    start_color();
    use_default_colors();

    if (has_colors() && can_change_color()) {
        init_color(16, RGB_TEXT[0], RGB_TEXT[1], RGB_TEXT[2]);
        init_color(18, RGB_SHADOW[0], RGB_SHADOW[1], RGB_SHADOW[2]);
        init_color(19, RGB_Z0[0], RGB_Z0[1], RGB_Z0[2]);
        init_color(20, RGB_Z1[0], RGB_Z1[1], RGB_Z1[2]);
        init_color(21, RGB_Z2[0], RGB_Z2[1], RGB_Z2[2]);
        init_color(22, RGB_Z3[0], RGB_Z3[1], RGB_Z3[2]);

        init_pair(1, 16, -1); 
        init_pair(3, 18, -1); 
        init_pair(4, 19, -1); 
        init_pair(5, 20, -1); 
        init_pair(6, 21, -1); 
        init_pair(7, 22, -1); 
        init_pair(8, COLOR_WHITE, -1); 
    } else {
        init_pair(1, COLOR_GREEN, -1);
        init_pair(3, COLOR_BLACK, -1);
        init_pair(4, COLOR_GREEN, -1);
        init_pair(5, COLOR_GREEN, -1);
        init_pair(6, COLOR_GREEN, -1);
        init_pair(7, COLOR_GREEN, -1);
        init_pair(8, COLOR_WHITE, -1);
    }

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int len = strlen(target);
    int text_row = rows / 2;
    int text_col = (cols - len) / 2;

    int trail_count = (int)(cols * RAIN_DENSITY);
    if (trail_count < 45) trail_count = 45;

    Trail *trails = calloc(trail_count, sizeof(Trail));

    for (int i=0; i<trail_count; i++) {
        trails[i].col = rand() % cols;
        trails[i].head = rand() % rows;
        trails[i].z_depth = rand() % 4; 
        
        float depth_mult = (trails[i].z_depth == 0) ? 1.00f : 
                           ((trails[i].z_depth == 1) ? 0.65f : 
                           ((trails[i].z_depth == 2) ? 0.42f : 0.20f));
        
        trails[i].speed = base_speed * depth_mult * (0.85f + ((float)rand()/RAND_MAX) * 0.3f);
        trails[i].length = (TRAIL_LENGTH_MIN + rand() % (TRAIL_LENGTH_MAX - TRAIL_LENGTH_MIN + 1)) * depth_mult;
        if (trails[i].length < 3) trails[i].length = 3;
    }

    int *revealed = calloc(len, sizeof(int));
    int *reveal_schedule = calloc(len, sizeof(int));
    int total_chars = 0;
    int *valid_indices = calloc(len, sizeof(int));
    
    for (int i=0; i<len; i++) {
        if (target[i] != ' ') {
            valid_indices[total_chars] = i;
            total_chars++;
        }
    }

    long build_frames = (long)(BUILD_DURATION_SEC * FPS);

    for (int i = total_chars - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = valid_indices[i];
        valid_indices[i] = valid_indices[j];
        valid_indices[j] = temp;
    }
    
    for (int i = 0; i < total_chars; i++) {
        int idx = valid_indices[i];
        reveal_schedule[idx] = (int)(((double)i / total_chars) * build_frames);
    }

    free(valid_indices);

    /* ==========================================
     * PHASE 1: BUILD SCENE 
     * ========================================== */
    for (long frame=0; frame<build_frames; frame++) {
        erase();

        attron(COLOR_PAIR(3) | A_DIM);
        for (int i = 0; i < len; i++) {
            if (target[i] != ' ' && revealed[i]) {
                mvaddch(text_row, text_col + i, target[i]);
            }
        }
        attroff(COLOR_PAIR(3) | A_DIM);

        for (int t=0; t<trail_count; t++) {
            float old_head = trails[t].head;
            trails[t].head += trails[t].speed;

            if (trails[t].z_depth == 0) {
                if (trails[t].col >= text_col && trails[t].col < text_col + len) {
                    int idx = trails[t].col - text_col;
                    if (target[idx] != ' ' && !revealed[idx] && frame >= reveal_schedule[idx]) {
                        if (old_head <= text_row && trails[t].head >= text_row) {
                            revealed[idx] = 1;
                        }
                    }
                }
            }

            if (trails[t].head - trails[t].length > rows) {
                trails[t].head = 0;
                trails[t].z_depth = rand() % 4;

                int forced_col = -1;
                for (int i=0; i<len; i++) {
                    if (target[i] != ' ' && !revealed[i] && frame >= reveal_schedule[i]) {
                        forced_col = text_col + i;
                        break;
                    }
                }

                if (forced_col != -1) {
                    trails[t].col = forced_col;
                    trails[t].z_depth = 0;
                    trails[t].speed = base_speed * 1.00f * (0.85f + ((float)rand()/RAND_MAX) * 0.3f);
                    
                    float distance_to_target = (float)text_row;
                    float frames_needed = distance_to_target / trails[t].speed;
                    long target_unlock_frame = frame + (long)frames_needed;
                    
                    for (int i=0; i<len; i++) {
                        if (text_col + i == forced_col) {
                            reveal_schedule[i] = target_unlock_frame;
                            break;
                        }
                    }
                } else {
                    trails[t].col = rand() % cols;
                }

                float depth_mult = (trails[t].z_depth == 0) ? 1.00f : 
                                   ((trails[t].z_depth == 1) ? 0.65f : 
                                   ((trails[t].z_depth == 2) ? 0.42f : 0.20f));
                if (forced_col == -1) {
                    trails[t].speed = base_speed * depth_mult * (0.85f + ((float)rand()/RAND_MAX) * 0.3f);
                }
                trails[t].length = (TRAIL_LENGTH_MIN + rand() % (TRAIL_LENGTH_MAX - TRAIL_LENGTH_MIN + 1)) * depth_mult;
                if (trails[t].length < 3) trails[t].length = 3;
            }

            for (int k=0; k<trails[t].length; k++) {
                int y = (int)trails[t].head - k;
                if (y < 0 || y >= rows) continue;

                if (trails[t].z_depth == 0) {
                    if (y == text_row && trails[t].col >= text_col && trails[t].col < text_col + len && revealed[trails[t].col - text_col]) continue;
                }

                if (trails[t].z_depth == 0) {
                    if (k == 0) attron(COLOR_PAIR(8) | A_BOLD);
                    else if (k <= 3) attron(COLOR_PAIR(4) | A_BOLD); 
                    else attron(COLOR_PAIR(5) | A_BOLD); 
                } else if (trails[t].z_depth == 1) {
                    if (k == 0) attron(COLOR_PAIR(4));
                    else attron(COLOR_PAIR(5));
                } else if (trails[t].z_depth == 2) {
                    attron(COLOR_PAIR(6));
                } else {
                    attron(COLOR_PAIR(7) | A_DIM);
                }

                mvaddch(y, trails[t].col, rc(trails[t].z_depth));

                if (trails[t].z_depth == 0) {
                    if (k == 0) attroff(COLOR_PAIR(8) | A_BOLD);
                    else if (k <= 3) attroff(COLOR_PAIR(4) | A_BOLD);
                    else attroff(COLOR_PAIR(5) | A_BOLD);
                } else if (trails[t].z_depth == 1) {
                    if (k == 0) attroff(COLOR_PAIR(4));
                    else attroff(COLOR_PAIR(5));
                } else if (trails[t].z_depth == 2) {
                    attroff(COLOR_PAIR(6));
                } else {
                    attroff(COLOR_PAIR(7) | A_DIM);
                }
            }
        }

        attron(COLOR_PAIR(1) | A_BOLD);
        for (int i=0; i<len; i++) {
            if (revealed[i]) {
                if (frame - reveal_schedule[i] < 12) {
                    attron(COLOR_PAIR(8) | A_BOLD);
                    mvaddch(text_row, text_col + i, rc(0));
                    attroff(COLOR_PAIR(8) | A_BOLD);
                } else {
                    mvaddch(text_row, text_col + i, target[i]);
                }
            }
        }
        attroff(COLOR_PAIR(1) | A_BOLD);

        refresh();
        usleep(1000000 / FPS);
    }

    for (int i = 0; i < len; i++) if (target[i] != ' ') revealed[i] = 1;

    /* ==========================================
     * PHASE 2: HOLD SCENE WITH EFFECTS
     * ========================================== */
    long hold_frames = (long)(HOLD_DURATION_SEC * FPS);
    for (long frame=0; frame<hold_frames; frame++) {
        erase();

        attron(COLOR_PAIR(3) | A_DIM);
        mvprintw(text_row, text_col, "%s", target);
        attroff(COLOR_PAIR(3) | A_DIM);

        for (int t=0; t<trail_count; t++) {
            trails[t].head += trails[t].speed;
            if (trails[t].head - trails[t].length > rows) {
                trails[t].head = 0;
                trails[t].col = rand() % cols;
            }

            for (int k=0; k<trails[t].length; k++) {
                int y = (int)trails[t].head - k;
                if (y < 0 || y >= rows) continue;

                if (trails[t].z_depth == 0) {
                    if (y == text_row && trails[t].col >= text_col && trails[t].col < text_col + len) continue;
                }

                if (trails[t].z_depth == 0) {
                    if (k == 0) attron(COLOR_PAIR(8) | A_BOLD);
                    else if (k <= 3) attron(COLOR_PAIR(4) | A_BOLD);
                    else attron(COLOR_PAIR(5) | A_BOLD);
                } else if (trails[t].z_depth == 1) {
                    if (k == 0) attron(COLOR_PAIR(4));
                    else attron(COLOR_PAIR(5));
                } else if (trails[t].z_depth == 2) {
                    attron(COLOR_PAIR(6));
                } else {
                    attron(COLOR_PAIR(7) | A_DIM);
                }

                mvaddch(y, trails[t].col, rc(trails[t].z_depth));

                if (trails[t].z_depth == 0) {
                    if (k == 0) attroff(COLOR_PAIR(8) | A_BOLD);
                    else if (k <= 3) attroff(COLOR_PAIR(4) | A_BOLD);
                    else attroff(COLOR_PAIR(5) | A_BOLD);
                } else if (trails[t].z_depth == 1) {
                    if (k == 0) attroff(COLOR_PAIR(4));
                    else attroff(COLOR_PAIR(5));
                } else if (trails[t].z_depth == 2) {
                    attroff(COLOR_PAIR(6));
                } else {
                    attroff(COLOR_PAIR(7) | A_DIM);
                }
            }
        }

        unsigned long main_attr = COLOR_PAIR(1) | A_BOLD;

        if (HOLD_EFFECT_MODE == 2) { 
            int pulse = frame % 40;
            if (pulse > 20) pulse = 40 - pulse;
            if (pulse < 5) {
                main_attr = COLOR_PAIR(1) | A_DIM;
            }
        }

        for (int i=0; i<len; i++) {
            char c = target[i];
            
            if (HOLD_EFFECT_MODE == 1 && c != ' ' && rand() % 35 == 0) {
                c = rc(0);
            }
            
            if (HOLD_EFFECT_MODE == 3) {
                int scan_line = (frame / 3) % (len + 6);
                if (i >= scan_line - 4 && i <= scan_line) {
                    attron(COLOR_PAIR(8) | A_BOLD);
                    mvaddch(text_row, text_col + i, c);
                    attroff(COLOR_PAIR(8) | A_BOLD);
                    continue;
                }
            }
            
            if (HOLD_EFFECT_MODE == 4 && c != ' ') {
                if (rand() % 40 == 0) {
                    c = (rand() % 2 == 0) ? '1' : '0';
                    attron(COLOR_PAIR(4) | A_DIM);
                    mvaddch(text_row, text_col + i, c);
                    attroff(COLOR_PAIR(4) | A_DIM);
                    continue;
                }
            }

            attron(main_attr);
            mvaddch(text_row, text_col + i, c);
            attroff(main_attr);
        }

        refresh();
        usleep(1000000 / FPS);
    }

    /* ==========================================
     * PHASE 3: OUTRO DYNAMIC SYSTEMS
     * ========================================== */
    long outro_frames = (long)(OUTRO_DURATION_SEC * FPS);
    
    int *disperse_x = calloc(len, sizeof(int));
    int *disperse_y = calloc(len, sizeof(int));

    int *rain_disp_x = calloc(trail_count, sizeof(int));
    int *rain_disp_y = calloc(trail_count, sizeof(int));

    for (int i=0; i<len; i++) {
        disperse_x[i] = (rand() % 7) - 3;
        disperse_y[i] = (rand() % 5) - 2;
    }
    for (int i=0; i<trail_count; i++) {
        rain_disp_x[i] = (rand() % 11) - 5; 
        rain_disp_y[i] = (rand() % 5) - 2; 
    }

    if (OUTRO_EFFECT_MODE == 1) goto shutdown;

    for (long frame=0; frame<outro_frames; frame++) {
        erase();
        float progress = (float)frame / outro_frames;

        if (OUTRO_EFFECT_MODE != 4 && OUTRO_EFFECT_MODE != 5 && OUTRO_EFFECT_MODE != 6 && progress < 0.5f) {
            attron(COLOR_PAIR(3) | A_DIM);
            mvprintw(text_row, text_col, "%s", target);
            attroff(COLOR_PAIR(3) | A_DIM);
        }

        for (int t=0; t<trail_count; t++) {
            if (OUTRO_EFFECT_MODE == 2 || OUTRO_EFFECT_MODE == 5 || OUTRO_EFFECT_MODE == 6) {
                trails[t].head += trails[t].speed;
            } else if (OUTRO_EFFECT_MODE == 3) {
                trails[t].head -= (trails[t].speed * 1.6f);
            } else if (OUTRO_EFFECT_MODE == 4) {
                trails[t].head += trails[t].speed; 
            }

            for (int k=0; k<trails[t].length; k++) {
                int y = (int)trails[t].head - k;
                int x = trails[t].col;

                if (OUTRO_EFFECT_MODE == 4) {
                    x += (int)(rain_disp_x[t] * (frame * 0.35f));
                    y += (int)(rain_disp_y[t] * (frame * 0.18f));
                }

                if (y < 0 || y >= rows || x < 0 || x >= cols) continue;

                if (OUTRO_EFFECT_MODE != 4 && OUTRO_EFFECT_MODE != 5 && OUTRO_EFFECT_MODE != 6 && trails[t].z_depth == 0) {
                    if (y == text_row && x >= text_col && x < text_col + len) continue;
                }

                if (trails[t].z_depth == 0) {
                    if (k == 0) attron(COLOR_PAIR(8) | A_BOLD);
                    else if (k <= 3) attron(COLOR_PAIR(4) | A_BOLD);
                    else attron(COLOR_PAIR(5) | A_BOLD);
                } else if (trails[t].z_depth == 1) {
                    attron(COLOR_PAIR(5));
                } else if (trails[t].z_depth == 2) {
                    attron(COLOR_PAIR(6));
                } else {
                    attron(COLOR_PAIR(7) | A_DIM);
                }

                if (OUTRO_EFFECT_MODE == 5 && rand() % 10 < (int)(progress * 9)) {
                    mvaddch(y, x, (rand() % 2 == 0) ? '1' : '0');
                } else {
                    mvaddch(y, x, rc(trails[t].z_depth));
                }

                if (trails[t].z_depth == 0) {
                    if (k == 0) attroff(COLOR_PAIR(8) | A_BOLD);
                    else if (k <= 3) attroff(COLOR_PAIR(4) | A_BOLD);
                    else attroff(COLOR_PAIR(5) | A_BOLD);
                } else if (trails[t].z_depth == 1) {
                    attroff(COLOR_PAIR(5));
                } else if (trails[t].z_depth == 2) {
                    attroff(COLOR_PAIR(6));
                } else {
                    attroff(COLOR_PAIR(7) | A_DIM);
                }
            }
        }

        if (OUTRO_EFFECT_MODE == 2 || OUTRO_EFFECT_MODE == 3) {
            if (progress < 0.65f) {
                unsigned long alpha = (progress > 0.35f) ? A_DIM : A_BOLD;
                int threshold = (int)(frame / 8);

                attron(COLOR_PAIR(1) | alpha);
                for (int i=0; i<len; i++) {
                    int offset_y = (OUTRO_EFFECT_MODE == 3) ? -(int)(base_speed * 1.6f * frame) : 0;
                    if (text_row + offset_y >= 0 && text_row + offset_y < rows) {
                        if (rand() % 16 > threshold) mvaddch(text_row + offset_y, text_col + i, target[i]);
                    }
                }
                attroff(COLOR_PAIR(1) | alpha);
            }
        } 
        else if (OUTRO_EFFECT_MODE == 4) {
            attron(COLOR_PAIR(1) | A_BOLD);
            for (int i=0; i<len; i++) {
                int cur_x = text_col + i + (int)(disperse_x[i] * (frame * 0.55f));
                int cur_y = text_row + (int)(disperse_y[i] * (frame * 0.28f));
                if (cur_y >= 0 && cur_y < rows && cur_x >= 0 && cur_x < cols) {
                    char c = (rand() % 5 == 0) ? rc(0) : target[i];
                    if (progress < 0.85f) mvaddch(cur_y, cur_x, c);
                }
            }
            attroff(COLOR_PAIR(1) | A_BOLD);
        }
        else if (OUTRO_EFFECT_MODE == 5) {
            if (progress < 0.90f) {
                attron(COLOR_PAIR(1) | A_BOLD);
                for (int i = 0; i < len; i++) {
                    if (rand() % 100 < (int)(progress * 100)) {
                        mvaddch(text_row, text_col + i, (rand() % 2 == 0) ? '1' : '0');
                    } else {
                        mvaddch(text_row, text_col + i, target[i]);
                    }
                }
                attroff(COLOR_PAIR(1) | A_BOLD);
            }
        }
        else if (OUTRO_EFFECT_MODE == 6) {
            float comp_offset = progress * (cols / 2.0f);
            attron(COLOR_PAIR(1) | A_BOLD);
            for (int i = 0; i < len; i++) {
                int orig_x = text_col + i;
                int cur_x = (orig_x < cols / 2) ? orig_x - (int)comp_offset : orig_x + (int)comp_offset;
                if (cur_x >= 0 && cur_x < cols) {
                    mvaddch(text_row, cur_x, target[i]);
                }
            }
            attroff(COLOR_PAIR(1) | A_BOLD);
        }

        refresh();
        usleep(1000000 / FPS);
    }

shutdown:
    free(trails);
    free(revealed);
    free(reveal_schedule);
    free(disperse_x);
    free(disperse_y);
    free(rain_disp_x);
    free(rain_disp_y);
    endwin();
    return 0;
}
