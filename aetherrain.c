
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define FPS 60

typedef enum {
    MENU_MAIN,
    MENU_CONFIG,
    MENU_TEXT_EDIT,
    MENU_COLOR_SELECT,
    MENU_EFFECT_SELECT,
    MENU_MORE_SELECT,
    MENU_TIME_SELECT,
    MENU_SPEED_SELECT,
    MENU_DENSITY_SELECT
} MenuPage;

typedef enum {
    INSTALL_IDLE,
    INSTALL_RUNNING,
    INSTALL_SUCCESS,
    INSTALL_FAILED
} InstallState;

typedef struct {
    int grid_col;      
    float y_pos;       
    float speed;       
    int length;        
    float z_depth;      
    int target_char_idx; 
    gboolean consuming;
} RainStream;

typedef struct {
    float y_pos;        
    float velocity_y;   
    int outro_delay;    
    int trail_length;   
    float opacity;      
} TextOutroState;

typedef struct {
    char target_text[256];
    int text_len;
    RainStream *streams;
    int stream_count;
    int max_stream_alloc;
    
    int *reveal_state;   
    int *reveal_order;   
    int *glitch_state;
    float *neon_opacity;  
    TextOutroState *text_outro; 
    
    int wave_current_pos;
    int wave_delay_counter;
    
    int current_target_ptr; 
    int font_size;
    float col_width;   
    int total_cols;    
    int text_start_col; 
    int text_center_y;
    int current_frame;
    int hold_start_frame;
    gboolean text_complete;
    gboolean outro_started;
    
    int rain_outro_start_frame; 
    gboolean rain_outro_triggered;
    
    float rain_global_opacity; 
    float text_global_opacity; 
    float dynamic_speed_factor; 

    int monitor_width;
    int monitor_height;

    float color_r;
    float color_g;
    float color_b;

    int effect_type;
    int outro_text_type;
    int outro_rain_type;

    float build_time_sec;
    float hold_time_sec;

    int rain_speed_setting;   
    int rain_density_setting; 

    MenuPage current_page;
    int main_selected;        
    int config_selected;      
    int color_selected;       
    int effect_row_selected;  
    int more_selected;
    int time_row_selected;
    int speed_row_selected;
    int density_row_selected;
    
    int custom_rgb_inside;    
    int custom_rgb_active;    
    int rgb_row_selected;     
    char rgb_manual_buffer[32];

    char input_buffer[256];
    int cursor_pos;           

    InstallState install_status;
    gboolean is_already_installed;
    char source_file_path[512];
} AppState;

static const char *MATRIX_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$*&+%?!";

static void get_config_path(char *path, size_t max_len) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, max_len, "%s/.config/aetherrain", home);
}

static void get_config_file_path(char *path, size_t max_len) {
    char dir[512];
    get_config_path(dir, sizeof(dir));
    snprintf(path, max_len, "%s/config.conf", dir);
}

static gboolean check_if_installed(void) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    char path[512];
    snprintf(path, sizeof(path), "%s/.local/bin/aetherrain-splash", home);
    return (access(path, F_OK) == 0);
}

static void save_config_to_disk(AppState *s) {
    char dir_path[512];
    char file_path[512];
    get_config_path(dir_path, sizeof(dir_path));
    get_config_file_path(file_path, sizeof(file_path));

    mkdir(dir_path, 0755);

    FILE *f = fopen(file_path, "w");
    if (!f) return;

    fprintf(f, "TEXT=%s\n", s->target_text);
    fprintf(f, "COLOR_R=%.2f\n", s->color_r);
    fprintf(f, "COLOR_G=%.2f\n", s->color_g);
    fprintf(f, "COLOR_B=%.2f\n", s->color_b);
    fprintf(f, "EFFECT_TYPE=%d\n", s->effect_type);
    fprintf(f, "OUTRO_TEXT_TYPE=%d\n", s->outro_text_type);
    fprintf(f, "OUTRO_RAIN_TYPE=%d\n", s->outro_rain_type);
    fprintf(f, "BUILD_TIME=%.1f\n", s->build_time_sec);
    fprintf(f, "HOLD_TIME=%.1f\n", s->hold_time_sec);
    fprintf(f, "SPEED=%d\n", s->rain_speed_setting);
    fprintf(f, "DENSITY=%d\n", s->rain_density_setting);

    fclose(f);
}

static void load_config_from_disk(AppState *s) {
    char file_path[512];
    get_config_file_path(file_path, sizeof(file_path));

    FILE *f = fopen(file_path, "r");
    if (!f) {
        strcpy(s->target_text, "SYSTEM INITIALIZATION");
        s->color_r = 1.0f; s->color_g = 1.0f; s->color_b = 1.0f;
        s->effect_type = 2;
        s->outro_text_type = 1;
        s->outro_rain_type = 1;
        s->build_time_sec = 2.0f;
        s->hold_time_sec = 3.0f;
        s->rain_speed_setting = 5;
        s->rain_density_setting = 5;
        save_config_to_disk(s);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *key = strtok(line, "=");
        char *val = strtok(NULL, "\n");
        if (!key || !val) continue;

        if (strcmp(key, "TEXT") == 0) {
            strcpy(s->target_text, val);
        }
        else if (strcmp(key, "COLOR_R") == 0) s->color_r = strtof(val, NULL);
        else if (strcmp(key, "COLOR_G") == 0) s->color_g = strtof(val, NULL);
        else if (strcmp(key, "COLOR_B") == 0) s->color_b = strtof(val, NULL);
        else if (strcmp(key, "EFFECT_TYPE") == 0) s->effect_type = atoi(val);
        else if (strcmp(key, "OUTRO_TEXT_TYPE") == 0) s->outro_text_type = atoi(val);
        else if (strcmp(key, "OUTRO_RAIN_TYPE") == 0) s->outro_rain_type = atoi(val);
        else if (strcmp(key, "BUILD_TIME") == 0) s->build_time_sec = strtof(val, NULL);
        else if (strcmp(key, "HOLD_TIME") == 0) s->hold_time_sec = strtof(val, NULL);
        else if (strcmp(key, "SPEED") == 0) s->rain_speed_setting = atoi(val);
        else if (strcmp(key, "DENSITY") == 0) s->rain_density_setting = atoi(val);
    }
    fclose(f);
}

static void* async_install_thread(void *arg) {
    AppState *s = (AppState*)arg;
    const char *home = getenv("HOME");
    if (!home) home = ".";

    char bin_dir[512], autostart_dir[512];
    snprintf(bin_dir, sizeof(bin_dir), "%s/.local", home); mkdir(bin_dir, 0755);
    snprintf(bin_dir, sizeof(bin_dir), "%s/.local/bin", home); mkdir(bin_dir, 0755);
    snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config", home); mkdir(autostart_dir, 0755);
    snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config/autostart", home); mkdir(autostart_dir, 0755);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc -O3 %s -o %s/.local/bin/aetherrain-splash $(pkg-config --cflags --libs gtk4) -lm -DPRODUCTION_MODE", s->source_file_path, home);
    
    int compile_res = system(cmd);
    if (compile_res != 0) {
        s->install_status = INSTALL_FAILED;
        return NULL;
    }

    char desktop_file_path[512];
    snprintf(desktop_file_path, sizeof(desktop_file_path), "%s/.config/autostart/aetherrain.desktop", home);
    FILE *df = fopen(desktop_file_path, "w");
    if (df) {
        fprintf(df, "[Desktop Entry]\n");
        fprintf(df, "Type=Application\n");
        fprintf(df, "Exec=%s/.local/bin/aetherrain-splash\n", home);
        fprintf(df, "Hidden=false\n");
        fprintf(df, "NoDisplay=true\n");
        fprintf(df, "X-GNOME-Autostart-enabled=true\n");
        fprintf(df, "Name=AetherRain Splash\n");
        fclose(df);
    }
    s->is_already_installed = TRUE;
    s->install_status = INSTALL_SUCCESS;
    return NULL;
}

static void trigger_system_installation(AppState *s) {
    s->install_status = INSTALL_RUNNING;
    save_config_to_disk(s);
    
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, async_install_thread, s);
    pthread_detach(thread_id);
}

static void shuffle_order(int *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

static void init_grid_engine(AppState *s, int width, int height) {
    s->font_size = 32; 
    s->col_width = s->font_size * 0.62f;
    s->total_cols = (int)(width / s->col_width);
    s->text_center_y = height / 4; 

    s->text_len = strlen(s->target_text);
    s->text_start_col = (s->total_cols - s->text_len) / 2;
    if (s->text_start_col < 0) s->text_start_col = 0;

    s->current_frame = 0;
    s->hold_start_frame = -1;
    s->text_complete = FALSE;
    s->outro_started = FALSE;
    s->rain_outro_triggered = FALSE;
    s->rain_outro_start_frame = -1;
    s->wave_current_pos = -10; 
    s->wave_delay_counter = 0;
    s->rain_global_opacity = 1.0f;
    s->text_global_opacity = 1.0f;
    s->dynamic_speed_factor = 1.0f;
    s->is_already_installed = check_if_installed();

    if (!s->reveal_state) s->reveal_state = calloc(256, sizeof(int));
    else memset(s->reveal_state, 0, 256 * sizeof(int));
    if (!s->reveal_order) s->reveal_order = calloc(256, sizeof(int));
    if (!s->glitch_state) s->glitch_state = calloc(256, sizeof(int));
    if (!s->neon_opacity) s->neon_opacity = calloc(256, sizeof(float));
    if (!s->text_outro) s->text_outro = calloc(256, sizeof(TextOutroState));
    
    int valid_chars_count = 0;
    for (int i = 0; i < s->text_len; i++) {
        s->neon_opacity[i] = 1.0f;
        s->glitch_state[i] = 0;
        s->text_outro[i].y_pos = s->text_center_y;
        s->text_outro[i].velocity_y = 0.0f;
        s->text_outro[i].trail_length = 4 + (rand() % 6); 
        s->text_outro[i].opacity = 1.0f;
        s->text_outro[i].outro_delay = rand() % 15; 
        
        if (s->target_text[i] == ' ') s->reveal_state[i] = 16;
        else {
            s->reveal_order[valid_chars_count] = i;
            valid_chars_count++;
        }
    }
    shuffle_order(s->reveal_order, valid_chars_count);
    s->current_target_ptr = 0;

    float density_multiplier = 0.4f + (s->rain_density_setting * 0.12f);
    int target_stream_count = (int)(s->total_cols * 1.20f * density_multiplier);
    if (target_stream_count < 10) target_stream_count = 10;

    if (!s->streams || target_stream_count > s->max_stream_alloc) {
        free(s->streams);
        s->max_stream_alloc = target_stream_count * 2;
        s->streams = calloc(s->max_stream_alloc, sizeof(RainStream));
    }
    s->stream_count = target_stream_count;

    float speed_multiplier = 0.7f + (s->rain_speed_setting * 0.06f);

    for (int i = 0; i < s->stream_count; i++) {
        s->streams[i].grid_col = rand() % s->total_cols;
        s->streams[i].y_pos = (rand() % height) * -1.0f;
        s->streams[i].z_depth = 0.1f + ((rand() % 90) / 100.0f);
        s->streams[i].speed = (10.0f + (rand() % 10)) * s->streams[i].z_depth * speed_multiplier; 
        s->streams[i].length = 6 + (rand() % 12);
        s->streams[i].target_char_idx = -1;
        s->streams[i].consuming = FALSE;

        if (s->streams[i].grid_col >= s->text_start_col && s->streams[i].grid_col < s->text_start_col + s->text_len) {
            s->streams[i].y_pos = -height * 1.5f;
        }
    }
}

static void draw_rain_stream(cairo_t *cr, AppState *s, int stream_idx, int height) {
    RainStream *stream = &s->streams[stream_idx];
    if (stream->length <= 0) return; 
    
    float dynamic_font_size = s->font_size * (0.3f + 0.7f * stream->z_depth);
    cairo_set_font_size(cr, dynamic_font_size);
    float x_pixel = stream->grid_col * s->col_width;

    for (int k = 0; k < stream->length; k++) {
        float y_pixel = stream->y_pos - (k * dynamic_font_size);
        if (y_pixel < -50 || y_pixel > height + dynamic_font_size + 50) continue;
        float fade = (1.0f - ((float)k / stream->length)) * (0.2f + 0.8f * stream->z_depth) * s->rain_global_opacity;
        if (fade <= 0.0f) continue;
        
        if (k == 0 && !stream->consuming) {
            cairo_set_source_rgba(cr, s->color_r, s->color_g, s->color_b, fade);
            if (!s->rain_outro_triggered && stream->target_char_idx != -1) {
                float next_y = stream->y_pos + stream->speed;
                if (stream->y_pos <= s->text_center_y && next_y >= s->text_center_y) {
                    int idx = stream->target_char_idx;
                    if (s->reveal_state[idx] == 0) { 
                        s->reveal_state[idx] = 1;
                        stream->consuming = TRUE; 
                    }
                }
            }
        } else {
            cairo_set_source_rgba(cr, s->color_r * 0.7f * stream->z_depth, s->color_g * 0.7f * stream->z_depth, s->color_b * 0.7f * stream->z_depth, fade);
        }
        
        char buf[2] = { MATRIX_CHARS[rand() % 45], '\0' };
        cairo_move_to(cr, x_pixel, y_pixel);
        cairo_show_text(cr, buf);
    }
}

static void draw_text_with_shadow(cairo_t *cr, const char *text, float x, float y, float r, float g, float b, float a) {
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, a * 0.85f);
    cairo_move_to(cr, x + 2, y + 2);
    cairo_show_text(cr, text);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);
}

static void draw_interactive_tui(cairo_t *cr, AppState *s, int width, int height) {
#ifndef PRODUCTION_MODE
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 22);
    int start_y = height - 400;
    int line_spacing = 45;
    cairo_text_extents_t extents;
    if (s->current_page == MENU_MAIN) {
        char install_label[64];
        if (s->install_status == INSTALL_RUNNING) {
            strcpy(install_label, "[ COMPILING & INSTALLING... ]");
        } else if (s->install_status == INSTALL_SUCCESS) {
            strcpy(install_label, "[ SUCCESS / INSTALLED ]");
        } else if (s->install_status == INSTALL_FAILED) {
            strcpy(install_label, "[ ERROR / INSTALL FAILED ]");
        } else {
            if (s->is_already_installed) {
                strcpy(install_label, "UPDATE CONFIG / REINSTALL");
            } else {
                strcpy(install_label, "INSTALL TO SYSTEM");
            }
        }

        const char *menus[3];
        menus[0] = "CONFIG";
        menus[1] = install_label;
        menus[2] = "EXIT";

        for (int i = 0; i < 3; i++) {
            int current_y = start_y + (i * line_spacing);
            float alpha = (s->main_selected == i) ? 1.0f : 0.35f;
            if (i == 1 && s->install_status == INSTALL_RUNNING) alpha = 1.0f;
            cairo_text_extents(cr, menus[i], &extents);
            float centered_x = (width - extents.width) / 2.0f;
            draw_text_with_shadow(cr, menus[i], centered_x, current_y, s->color_r, s->color_g, s->color_b, alpha);
        }
    }
    else if (s->current_page == MENU_CONFIG) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);

        int sub_y = start_y + line_spacing;
        const char *sub_menus[] = { "Text", "Color", "Effect", "More", "Back" };
        
        int total_menu_width = 0;
        int spacings[5];
        for (int i = 0; i < 5; i++) {
            cairo_text_extents(cr, sub_menus[i], &extents);
            spacings[i] = extents.width;
            total_menu_width += extents.width + (i < 4 ? 40 : 0);
        }

        float current_item_x = (width - total_menu_width) / 2.0f;
        for (int i = 0; i < 5; i++) {
            float alpha = (s->config_selected == i) ? 1.0f : 0.35f;
            draw_text_with_shadow(cr, sub_menus[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
            current_item_x += spacings[i] + 40;
        }
    }
    else if (s->current_page == MENU_TEXT_EDIT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;
        cairo_text_extents(cr, s->input_buffer, &extents);
        float text_base_x = (width - extents.width) / 2.0f;
        
        draw_text_with_shadow(cr, s->input_buffer, text_base_x, sub_y, s->color_r, s->color_g, s->color_b, 1.0f);
        gboolean cursor_visible = (s->current_frame % 30 < 15);
        if (cursor_visible) {
            char temp_buf[256] = {0};
            strncpy(temp_buf, s->input_buffer, s->cursor_pos);
            cairo_text_extents(cr, temp_buf, &extents);
            float cursor_x = text_base_x + extents.x_advance;

            cairo_set_source_rgba(cr, s->color_r, s->color_g, s->color_b, 1.0f);
            cairo_move_to(cr, cursor_x, sub_y);
            cairo_line_to(cr, cursor_x, sub_y - 20); 
            cairo_set_line_width(cr, 2.0);
            cairo_stroke(cr);
        }
    }
    else if (s->current_page == MENU_COLOR_SELECT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;
        if (s->custom_rgb_inside == 0) {
            const char *colors[] = { "White", "Green", "Red", "Purple", "Custom", "Back" };
            int total_menu_width = 0;
            int spacings[6];
            for (int i = 0; i < 6; i++) {
                cairo_text_extents(cr, colors[i], &extents);
                spacings[i] = extents.width;
                total_menu_width += extents.width + (i < 5 ? 35 : 0);
            }

            float current_item_x = (width - total_menu_width) / 2.0f;
            for (int i = 0; i < 6; i++) {
                float alpha = (s->color_selected == i) ? 1.0f : 0.35f;
                draw_text_with_shadow(cr, colors[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
                current_item_x += spacings[i] + 35;
            }
        } else {
            char items_str[4][128];
            if (s->rgb_row_selected == 0 && s->custom_rgb_active == 1) {
                snprintf(items_str[0], 128, "R: %s", s->rgb_manual_buffer);
            } else {
                snprintf(items_str[0], 128, "R: %.2f", s->color_r);
            }

            if (s->rgb_row_selected == 1 && s->custom_rgb_active == 1) {
                snprintf(items_str[1], 128, "G: %s", s->rgb_manual_buffer);
            } else {
                snprintf(items_str[1], 128, "G: %.2f", s->color_g);
            }

            if (s->rgb_row_selected == 2 && s->custom_rgb_active == 1) {
                snprintf(items_str[2], 128, "B: %s", s->rgb_manual_buffer);
            } else {
                snprintf(items_str[2], 128, "B: %.2f", s->color_b);
            }
            
            strcpy(items_str[3], "Back");
            int total_rgb_width = 0;
            int item_widths[4];
            for (int i = 0; i < 4; i++) {
                cairo_text_extents(cr, items_str[i], &extents);
                item_widths[i] = extents.width;
                total_rgb_width += extents.width + (i < 3 ? 50 : 0);
            }

            float current_item_x = (width - total_rgb_width) / 2.0f;
            for (int i = 0; i < 4; i++) {
                float alpha = (s->rgb_row_selected == i) ? 1.0f : 0.35f;
                draw_text_with_shadow(cr, items_str[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
                if (s->rgb_row_selected == i && s->custom_rgb_active == 1) {
                    cairo_text_extents(cr, items_str[i], &extents);
                    gboolean rgb_cursor_visible = (s->current_frame % 30 < 15);
                    if (rgb_cursor_visible) {
                        float cx = current_item_x + extents.x_advance + 2;
                        cairo_set_source_rgba(cr, s->color_r, s->color_g, s->color_b, 1.0f);
                        cairo_move_to(cr, cx, sub_y);
                        cairo_line_to(cr, cx, sub_y - 20);
                        cairo_set_line_width(cr, 2.0);
                        cairo_stroke(cr);
                    }
                }
                current_item_x += item_widths[i] + 50;
            }
        }
    }
    else if (s->current_page == MENU_EFFECT_SELECT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;

        char items_str[4][128];
        const char *effect_names[] = { "None", "Matrix Rand", "Neon Blink", "H-Glitch" };
        const char *outro_text_names[] = { "Fade", "Gravity Fall", "Neon Flicker" };
        const char *outro_rain_names[] = { "Fade", "Chaos Burst" };

        snprintf(items_str[0], 128, "Text: %s", effect_names[s->effect_type]);
        snprintf(items_str[1], 128, "Text Out: %s", outro_text_names[s->outro_text_type]);
        snprintf(items_str[2], 128, "Rain Out: %s", outro_rain_names[s->outro_rain_type]);
        strcpy(items_str[3], "Back");

        int total_effect_width = 0;
        int item_widths[4];
        for (int i = 0; i < 4; i++) {
            cairo_text_extents(cr, items_str[i], &extents);
            item_widths[i] = extents.width;
            total_effect_width += extents.width + (i < 3 ? 40 : 0);
        }

        float current_item_x = (width - total_effect_width) / 2.0f;
        for (int i = 0; i < 4; i++) {
            float alpha = (s->effect_row_selected == i) ? 1.0f : 0.35f;
            draw_text_with_shadow(cr, items_str[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
            current_item_x += item_widths[i] + 40;
        }
    }
    else if (s->current_page == MENU_MORE_SELECT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;
        const char *more_menus[] = { "Time", "Speed", "Density", "Back" };
        
        int total_more_width = 0;
        int spacings[4];
        for (int i = 0; i < 4; i++) {
            cairo_text_extents(cr, more_menus[i], &extents);
            spacings[i] = extents.width;
            total_more_width += extents.width + (i < 3 ? 45 : 0);
        }

        float current_item_x = (width - total_more_width) / 2.0f;
        for (int i = 0; i < 4; i++) {
            float alpha = (s->more_selected == i) ? 1.0f : 0.35f;
            draw_text_with_shadow(cr, more_menus[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
            current_item_x += spacings[i] + 45;
        }
    }
    else if (s->current_page == MENU_TIME_SELECT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;

        char items_str[3][128];
        snprintf(items_str[0], 128, "Build: %.1fs", s->build_time_sec);
        snprintf(items_str[1], 128, "Hold: %.1fs", s->hold_time_sec);
        strcpy(items_str[2], "Back");

        int total_time_width = 0;
        int item_widths[3];
        for (int i = 0; i < 3; i++) {
            cairo_text_extents(cr, items_str[i], &extents);
            item_widths[i] = extents.width;
            total_time_width += extents.width + (i < 2 ? 50 : 0);
        }

        float current_item_x = (width - total_time_width) / 2.0f;
        for (int i = 0; i < 3; i++) {
            float alpha = (s->time_row_selected == i) ? 1.0f : 0.35f;
            draw_text_with_shadow(cr, items_str[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
            current_item_x += item_widths[i] + 50;
        }
    }
    else if (s->current_page == MENU_SPEED_SELECT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;

        char items_str[2][128];
        snprintf(items_str[0], 128, "Rain Speed: %d", s->rain_speed_setting);
        strcpy(items_str[1], "Back");

        int total_speed_width = 0;
        int item_widths[2];
        for (int i = 0; i < 2; i++) {
            cairo_text_extents(cr, items_str[i], &extents);
            item_widths[i] = extents.width;
            total_speed_width += extents.width + (i < 1 ? 60 : 0);
        }

        float current_item_x = (width - total_speed_width) / 2.0f;
        for (int i = 0; i < 2; i++) {
            float alpha = (s->speed_row_selected == i) ? 1.0f : 0.35f;
            draw_text_with_shadow(cr, items_str[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
            current_item_x += item_widths[i] + 60;
        }
    }
    else if (s->current_page == MENU_DENSITY_SELECT) {
        cairo_text_extents(cr, "CONFIG", &extents);
        draw_text_with_shadow(cr, "CONFIG", (width - extents.width) / 2.0f, start_y, s->color_r, s->color_g, s->color_b, 0.5f);
        int sub_y = start_y + line_spacing;

        char items_str[2][128];
        snprintf(items_str[0], 128, "Rain Density: %d", s->rain_density_setting);
        strcpy(items_str[1], "Back");

        int total_density_width = 0;
        int item_widths[2];
        for (int i = 0; i < 2; i++) {
            cairo_text_extents(cr, items_str[i], &extents);
            item_widths[i] = extents.width;
            total_density_width += extents.width + (i < 1 ? 60 : 0);
        }

        float current_item_x = (width - total_density_width) / 2.0f;
        for (int i = 0; i < 2; i++) {
            float alpha = (s->density_row_selected == i) ? 1.0f : 0.35f;
            draw_text_with_shadow(cr, items_str[i], current_item_x, sub_y, s->color_r, s->color_g, s->color_b, alpha);
            current_item_x += item_widths[i] + 60;
        }
    }
#endif
}

static void render_grid_pipeline(GtkDrawingArea *da, cairo_t *cr, int width, int height, gpointer data) {
    AppState *s = data;
    s->current_frame++;

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.25);
    cairo_paint(cr);
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    float total_allowed_hold_frames = s->hold_time_sec * FPS;
    if (s->hold_time_sec <= 1.0f) {
        s->dynamic_speed_factor = 2.4f;
    } else if (s->hold_time_sec <= 1.5f) {
        s->dynamic_speed_factor = 1.8f;
    } else {
        s->dynamic_speed_factor = 1.0f;
    }

    int total_valid = 0;
    for(int i = 0; i < s->text_len; i++) if(s->target_text[i] != ' ') total_valid++;
    gboolean all_revealed_now = TRUE;
    for (int i = 0; i < s->text_len; i++) {
        if (s->target_text[i] != ' ' && s->reveal_state[i] == 0) {
            all_revealed_now = FALSE;
            break;
        }
    }

    if (all_revealed_now && !s->text_complete) { 
        s->text_complete = TRUE;
        s->hold_start_frame = s->current_frame; 
    }

    if (s->text_complete && s->hold_start_frame != -1) {
        int frames_since_complete = s->current_frame - s->hold_start_frame;
        if (frames_since_complete >= total_allowed_hold_frames) s->outro_started = TRUE;
        
        if (s->hold_time_sec <= 1.2f) {
            s->rain_outro_triggered = TRUE;
            if (s->rain_outro_start_frame == -1) s->rain_outro_start_frame = s->current_frame;
        } else {
            if (frames_since_complete >= 1.6f * FPS) {
                s->rain_outro_triggered = TRUE;
                if (s->rain_outro_start_frame == -1) s->rain_outro_start_frame = s->current_frame;
            }
        }
    }

    gboolean should_stop_incoming_rain = FALSE;
    if (s->current_target_ptr >= total_valid || s->text_complete || s->rain_outro_triggered) {
        should_stop_incoming_rain = TRUE;
    }

    if (!s->rain_outro_triggered && !should_stop_incoming_rain && s->current_target_ptr < total_valid && s->install_status != INSTALL_RUNNING) {
        int target_frame = (s->current_target_ptr * (s->build_time_sec * FPS)) / total_valid;
        if (s->current_frame >= target_frame) {
            int next_char = s->reveal_order[s->current_target_ptr];
            for (int i = 0; i < s->stream_count; i++) {
                if (s->streams[i].target_char_idx == -1 && s->streams[i].z_depth > 0.7f && s->streams[i].z_depth < 0.9f && !s->streams[i].consuming) {
                    s->streams[i].target_char_idx = next_char;
                    s->streams[i].grid_col = s->text_start_col + next_char;
                    s->streams[i].y_pos = -50.0f;
                    
                    float speed_multiplier = 0.7f + (s->rain_speed_setting * 0.06f);
                    s->streams[i].speed = (18.0f + (rand() % 4)) * speed_multiplier;
                    s->streams[i].length = 10 + (rand() % 6); 
                    s->streams[i].consuming = FALSE;
                    s->current_target_ptr++;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < s->stream_count; i++) {
        if (s->streams[i].consuming) {
            s->streams[i].y_pos = s->text_center_y;
            s->streams[i].length -= 2; 
            if (s->streams[i].length <= 0) { 
                s->streams[i].y_pos = -200.0f; 
                s->streams[i].target_char_idx = -1; 
                s->streams[i].consuming = FALSE;
            }
        } else {
            if (s->rain_outro_triggered) {
                int elapsed_rain_outro = s->current_frame - s->rain_outro_start_frame;
                float current_step_speed = s->streams[i].speed;
                
                if (s->outro_rain_type == 0) {
                    s->streams[i].y_pos += current_step_speed;
                    s->rain_global_opacity = 1.0f - ((float)elapsed_rain_outro / (0.6f * FPS));
                    if (s->rain_global_opacity < 0.0f) s->rain_global_opacity = 0.0f;
                }
                else if (s->outro_rain_type == 1) {
                    if (elapsed_rain_outro < 40) {
                        current_step_speed = s->streams[i].speed * cosf(((float)elapsed_rain_outro / 40.0f) * M_PI);
                    } else {
                        current_step_speed = -s->streams[i].speed * (1.0f + ((float)(elapsed_rain_outro - 40) * 0.12f)) * s->dynamic_speed_factor;
                    }
                    s->streams[i].y_pos += current_step_speed;
                }
                
                if (s->streams[i].y_pos + (s->streams[i].length * s->font_size) < -150 || s->streams[i].y_pos - (s->streams[i].length * s->font_size) > height + 150) s->streams[i].length = 0;
                continue;
            }
            
            if (s->install_status != INSTALL_RUNNING) {
                s->streams[i].y_pos += s->streams[i].speed;
            }
            
            if (s->streams[i].y_pos - (s->streams[i].length * (s->font_size * 0.5f)) > height) {
                if (s->rain_outro_triggered || should_stop_incoming_rain) { 
                    s->streams[i].y_pos = -500.0f;
                    s->streams[i].length = 0; 
                    continue; 
                }
                s->streams[i].y_pos = -40.0f;
                s->streams[i].grid_col = rand() % s->total_cols;
                if (!s->rain_outro_triggered && s->streams[i].grid_col >= s->text_start_col && s->streams[i].grid_col < s->text_start_col + s->text_len) {
                    s->streams[i].grid_col = (rand() % 2 == 0) ?
                    (rand() % s->text_start_col) : (s->text_start_col + s->text_len + (rand() % (s->total_cols - (s->text_start_col + s->text_len))));
                }
                s->streams[i].z_depth = 0.1f + ((rand() % 90) / 100.0f);
                
                float speed_multiplier = 0.7f + (s->rain_speed_setting * 0.06f);
                s->streams[i].speed = (10.0f + (rand() % 10)) * s->streams[i].z_depth * speed_multiplier;
            }
        }
    }

    for (int i = 0; i < s->stream_count; i++) if (s->streams[i].z_depth < 0.6f) draw_rain_stream(cr, s, i, height);
    s->wave_delay_counter++;
    if (s->wave_delay_counter >= 4) { 
        s->wave_delay_counter = 0; 
        s->wave_current_pos += 1;
        if (s->wave_current_pos >= s->text_len + 6) s->wave_current_pos = -10; 
    }

    gboolean text_animations_done = FALSE;
    if (s->outro_started) {
        int elapsed_outro_frames = s->current_frame - (s->hold_start_frame + total_allowed_hold_frames);
        gboolean all_gone = TRUE;
        int text_trigger_delay = (int)(0.02f * FPS);
        
        for (int i = 0; i < s->text_len; i++) {
            if (s->target_text[i] != ' ') {
                if (s->outro_text_type == 0) {
                    float fade_progress = (float)elapsed_outro_frames / (0.4f * FPS);
                    s->text_global_opacity = 1.0f - fade_progress;
                    if (s->text_global_opacity <= 0.0f) {
                        s->text_global_opacity = 0.0f;
                    } else {
                        all_gone = FALSE;
                    }
                }
                else if (s->outro_text_type == 1) {
                    if (elapsed_outro_frames >= text_trigger_delay && (elapsed_outro_frames - text_trigger_delay) > s->text_outro[i].outro_delay) {
                        int frames_in_motion = (elapsed_outro_frames - text_trigger_delay - s->text_outro[i].outro_delay);
                        float text_step_vel = (frames_in_motion < 40) ? (-8.0f * sinf(((float)frames_in_motion / 40.0f) * M_PI * 0.5f)) : (-8.0f - ((float)(frames_in_motion - 40) * 0.85f));
                        s->text_outro[i].velocity_y = text_step_vel * s->dynamic_speed_factor;
                        s->text_outro[i].y_pos += s->text_outro[i].velocity_y;
                        if (s->text_outro[i].y_pos > -150) all_gone = FALSE;
                    } else { all_gone = FALSE; }
                }
                else if (s->outro_text_type == 2) {
                    if (elapsed_outro_frames > s->text_outro[i].outro_delay) {
                        if (s->text_outro[i].opacity > 0.0f) {
                            if (rand() % 100 < 15) s->text_outro[i].opacity = (rand() % 100 < 10) ? 0.0f : (0.2f + (rand() % 30) / 100.0f);
                            else s->text_outro[i].opacity -= 0.03f;
                            if (s->text_outro[i].opacity < 0.0f) s->text_outro[i].opacity = 0.0f;
                        }
                    }
                    if (s->text_outro[i].opacity > 0.0f) all_gone = FALSE;
                }
            }
        }
        if (all_gone) text_animations_done = TRUE;
        if (text_animations_done) {
#ifdef PRODUCTION_MODE
            g_application_quit(g_application_get_default());
            return;
#else
            init_grid_engine(s, s->monitor_width, s->monitor_height);
#endif
        }
    }

    cairo_set_font_size(cr, s->font_size);
    
    int matrix_wave_width = 2 + (s->text_len / 10);
    if (matrix_wave_width < 2) matrix_wave_width = 2;
    if (matrix_wave_width > 4) matrix_wave_width = 4;

    for (int i = 0; i < s->text_len; i++) {
        if (s->target_text[i] == ' ') continue;
        if (s->reveal_state[i] > 0 && s->text_outro[i].y_pos < height + 150 && s->text_outro[i].y_pos > -150) {
            float text_x_pixel = (s->text_start_col + i) * s->col_width;
            gboolean is_glitch_bright = FALSE;

            if (s->effect_type == 3 && !s->outro_started) {
                if (s->wave_current_pos >= 0 && i >= s->wave_current_pos && i < s->wave_current_pos + matrix_wave_width) {
                    is_glitch_bright = TRUE;
                    if (rand() % 100 < 55) {
                        s->glitch_state[i] = (rand() % 3 == 0) ? (rand() % 45 - 22) : 0;
                    }
                } else {
                    if (rand() % 100 < 4) {
                        s->glitch_state[i] = (rand() % 3 == 0) ? (rand() % 15 - 7) : 0;
                    }
                }
                text_x_pixel += s->glitch_state[i];
            }

            if (s->outro_started && s->text_outro[i].velocity_y < 0 && s->outro_text_type == 1) {
                for (int t = 1; t <= s->text_outro[i].trail_length; t++) {
                    float trail_y = s->text_outro[i].y_pos + (t * s->font_size * 0.75f);
                    if (trail_y > s->text_center_y + 200) continue;
                    cairo_set_source_rgba(cr, s->color_r, s->color_g, s->color_b, (1.0f - ((float)t / s->text_outro[i].trail_length)) * 0.2f);
                    cairo_move_to(cr, text_x_pixel, trail_y);
                    char trail_char[2] = { MATRIX_CHARS[rand() % 45], '\0' };
                    cairo_show_text(cr, trail_char);
                }
            }

            char disp = s->target_text[i];
            float current_opacity = s->text_global_opacity;
            
            if (s->outro_started && s->outro_text_type == 2) {
                current_opacity *= s->text_outro[i].opacity;
            }

            if (s->reveal_state[i] < 16) {
                disp = MATRIX_CHARS[rand() % 45];
                s->reveal_state[i]++;
                if (!s->outro_started) {
                    if (s->effect_type == 2) {
                        s->neon_opacity[i] = (rand() % 100 < 30) ? 0.35f : 0.65f;
                        current_opacity *= s->neon_opacity[i];
                    }
                }
            } else if (!s->outro_started) {
                if (s->effect_type == 1) {
                    if (rand() % 100 < 16) disp = MATRIX_CHARS[rand() % 45];
                }
                else if (s->effect_type == 2) {
                    if (rand() % 100 < 8) s->neon_opacity[i] = (rand() % 100 < 30) ? 0.25f : 0.55f;
                    else s->neon_opacity[i] += (1.0f - s->neon_opacity[i]) * 0.05f;
                    current_opacity *= s->neon_opacity[i];
                }
                else if (s->effect_type == 3 && is_glitch_bright) {
                    if (rand() % 100 < 75) disp = MATRIX_CHARS[rand() % 45];
                }
            }

            if (current_opacity > 0.0f) {
                if (s->effect_type == 3 && is_glitch_bright && !s->outro_started) {
                    cairo_set_source_rgba(cr, s->color_r * 0.3f + 0.7f, s->color_g * 0.3f + 0.7f, s->color_b * 0.3f + 0.7f, current_opacity);
                } else {
                    cairo_set_source_rgba(cr, s->color_r, s->color_g, s->color_b, current_opacity);
                }
                cairo_move_to(cr, text_x_pixel, s->text_outro[i].y_pos);
                char buf[2] = { disp, '\0' };
                cairo_show_text(cr, buf);
            }
        }
    }
    
    for (int i = 0; i < s->stream_count; i++) if (s->streams[i].z_depth >= 0.6f) draw_rain_stream(cr, s, i, height);
    draw_interactive_tui(cr, s, width, height);
}

static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
#ifndef PRODUCTION_MODE
    GtkApplication *app = GTK_APPLICATION(user_data);
    AppState *s = g_object_get_data(G_OBJECT(app), "app_state");

    if (s->install_status == INSTALL_RUNNING) return TRUE;

    if (keyval == GDK_KEY_q || keyval == GDK_KEY_Q) {
        g_application_quit(G_APPLICATION(app));
        return TRUE;
    }

    if (s->current_page == MENU_MAIN) {
        if (keyval == GDK_KEY_Up) {
            s->main_selected = (s->main_selected - 1 + 3) % 3;
            return TRUE;
        } else if (keyval == GDK_KEY_Down) {
            s->main_selected = (s->main_selected + 1) % 3;
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->main_selected == 0) s->current_page = MENU_CONFIG;
            else if (s->main_selected == 1) {
                trigger_system_installation(s);
            }
            else if (s->main_selected == 2) g_application_quit(G_APPLICATION(app));
            return TRUE;
        }
    }
    else if (s->current_page == MENU_CONFIG) {
        if (keyval == GDK_KEY_Left) {
            s->config_selected = (s->config_selected - 1 + 5) % 5;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            s->config_selected = (s->config_selected + 1) % 5;
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->config_selected == 0) {
                s->current_page = MENU_TEXT_EDIT;
                strcpy(s->input_buffer, s->target_text);
                s->cursor_pos = strlen(s->input_buffer);
            } else if (s->config_selected == 1) {
                s->current_page = MENU_COLOR_SELECT;
                s->color_selected = 0;
                s->custom_rgb_inside = 0;
                s->custom_rgb_active = 0;
                s->rgb_row_selected = 0;
            } else if (s->config_selected == 2) {
                s->current_page = MENU_EFFECT_SELECT;
                s->effect_row_selected = 0;
            } else if (s->config_selected == 3) {
                s->current_page = MENU_MORE_SELECT;
                s->more_selected = 0;
            } else if (s->config_selected == 4) {
                save_config_to_disk(s);
                s->current_page = MENU_MAIN;
            }
            return TRUE;
        }
    }
    else if (s->current_page == MENU_TEXT_EDIT) {
        int len = strlen(s->input_buffer);
        if (keyval == GDK_KEY_Return) {
            if (len > 0) {
                strcpy(s->target_text, s->input_buffer);
                save_config_to_disk(s);
                init_grid_engine(s, s->monitor_width, s->monitor_height);
            }
            s->current_page = MENU_CONFIG;
            return TRUE;
        } else if (keyval == GDK_KEY_Left) {
            if (s->cursor_pos > 0) s->cursor_pos--;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            if (s->cursor_pos < len) s->cursor_pos++;
            return TRUE;
        } else if (keyval == GDK_KEY_BackSpace) {
            if (s->cursor_pos > 0) {
                memmove(&s->input_buffer[s->cursor_pos - 1], &s->input_buffer[s->cursor_pos], len - s->cursor_pos + 1);
                s->cursor_pos--;
            }
            return TRUE;
        } else {
            guint32 unicode = gdk_keyval_to_unicode(keyval);
            if (unicode >= 32 && unicode <= 126) {
                if (len < 40) {
                    memmove(&s->input_buffer[s->cursor_pos + 1], &s->input_buffer[s->cursor_pos], len - s->cursor_pos + 1);
                    s->input_buffer[s->cursor_pos] = (char)unicode;
                    s->cursor_pos++;
                }
                return TRUE;
            }
        }
    }
    else if (s->current_page == MENU_COLOR_SELECT) {
        if (s->custom_rgb_inside == 0) {
            if (keyval == GDK_KEY_Left) {
                s->color_selected = (s->color_selected - 1 + 6) % 6;
                return TRUE;
            } else if (keyval == GDK_KEY_Right) {
                s->color_selected = (s->color_selected + 1) % 6;
                return TRUE;
            } else if (keyval == GDK_KEY_Return) {
                if (s->color_selected == 0) { s->color_r = 1.0f;
                s->color_g = 1.0f; s->color_b = 1.0f; }
                else if (s->color_selected == 1) { s->color_r = 0.0f;
                s->color_g = 1.0f; s->color_b = 0.0f; }
                else if (s->color_selected == 2) { s->color_r = 1.0f;
                s->color_g = 0.0f; s->color_b = 0.0f; }
                else if (s->color_selected == 3) { s->color_r = 0.7f;
                s->color_g = 0.0f; s->color_b = 1.0f; }
                else if (s->color_selected == 4) { s->custom_rgb_inside = 1;
                s->rgb_row_selected = 0; s->custom_rgb_active = 0; }
                else if (s->color_selected == 5) { 
                    save_config_to_disk(s);
                    s->current_page = MENU_CONFIG;
                }
                if (s->color_selected < 4) save_config_to_disk(s);
                return TRUE;
            }
        } else {
            if (s->custom_rgb_active == 1) {
                if (keyval == GDK_KEY_Return) {
                    float manual_val = strtof(s->rgb_manual_buffer, NULL);
                    if (manual_val < 0.0f) manual_val = 0.0f;
                    if (manual_val > 1.0f) manual_val = 1.0f;
                    if (s->rgb_row_selected == 0) s->color_r = manual_val;
                    else if (s->rgb_row_selected == 1) s->color_g = manual_val;
                    else if (s->rgb_row_selected == 2) s->color_b = manual_val;

                    s->custom_rgb_active = 0; 
                    save_config_to_disk(s);
                    return TRUE;
                } else if (keyval == GDK_KEY_BackSpace) {
                    int len = strlen(s->rgb_manual_buffer);
                    if (len > 0) s->rgb_manual_buffer[len - 1] = '\0';
                    return TRUE;
                } else {
                    guint32 unicode = gdk_keyval_to_unicode(keyval);
                    if ((unicode >= '0' && unicode <= '9') || unicode == '.') {
                        int len = strlen(s->rgb_manual_buffer);
                        if (len < 10) {
                            s->rgb_manual_buffer[len] = (char)unicode;
                            s->rgb_manual_buffer[len + 1] = '\0';
                        }
                    }
                    return TRUE;
                }
            } else {
                if (keyval == GDK_KEY_Left) {
                    s->rgb_row_selected = (s->rgb_row_selected - 1 + 4) % 4;
                    return TRUE;
                } else if (keyval == GDK_KEY_Right) {
                    s->rgb_row_selected = (s->rgb_row_selected + 1) % 4;
                    return TRUE;
                } else if (keyval == GDK_KEY_Up) {
                    if (s->rgb_row_selected == 0) { s->color_r += 0.05f;
                    if (s->color_r > 1.0f) s->color_r = 1.0f; }
                    else if (s->rgb_row_selected == 1) { s->color_g += 0.05f;
                    if (s->color_g > 1.0f) s->color_g = 1.0f; }
                    else if (s->rgb_row_selected == 2) { s->color_b += 0.05f;
                    if (s->color_b > 1.0f) s->color_b = 1.0f; }
                    save_config_to_disk(s);
                    return TRUE;
                } else if (keyval == GDK_KEY_Down) {
                    if (s->rgb_row_selected == 0) { s->color_r -= 0.05f;
                    if (s->color_r < 0.0f) s->color_r = 0.0f; }
                    else if (s->rgb_row_selected == 1) { s->color_g -= 0.05f;
                    if (s->color_g < 0.0f) s->color_g = 0.0f; }
                    else if (s->rgb_row_selected == 2) { s->color_b -= 0.05f;
                    if (s->color_b < 0.0f) s->color_b = 0.0f; }
                    save_config_to_disk(s);
                    return TRUE;
                } else if (keyval == GDK_KEY_Return) {
                    if (s->rgb_row_selected == 3) {
                        s->custom_rgb_inside = 0;
                        save_config_to_disk(s);
                    } else {
                        s->custom_rgb_active = 1;
                        s->rgb_manual_buffer[0] = '\0';
                    }
                    return TRUE;
                }
            }
        }
    }
    else if (s->current_page == MENU_EFFECT_SELECT) {
        if (keyval == GDK_KEY_Left) {
            s->effect_row_selected = (s->effect_row_selected - 1 + 4) % 4;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            s->effect_row_selected = (s->effect_row_selected + 1) % 4;
            return TRUE;
        } else if (keyval == GDK_KEY_Up) {
            if (s->effect_row_selected == 0) s->effect_type = (s->effect_type - 1 + 4) % 4;
            else if (s->effect_row_selected == 1) s->outro_text_type = (s->outro_text_type - 1 + 3) % 3;
            else if (s->effect_row_selected == 2) s->outro_rain_type = (s->outro_rain_type - 1 + 2) % 2;
            save_config_to_disk(s);
            return TRUE;
        } else if (keyval == GDK_KEY_Down) {
            if (s->effect_row_selected == 0) s->effect_type = (s->effect_type + 1) % 4;
            else if (s->effect_row_selected == 1) s->outro_text_type = (s->outro_text_type + 1) % 3;
            else if (s->effect_row_selected == 2) s->outro_rain_type = (s->outro_rain_type + 1) % 2;
            save_config_to_disk(s);
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->effect_row_selected == 3) {
                save_config_to_disk(s);
                s->current_page = MENU_CONFIG;
            }
            return TRUE;
        }
    }
    else if (s->current_page == MENU_MORE_SELECT) {
        if (keyval == GDK_KEY_Left) {
            s->more_selected = (s->more_selected - 1 + 4) % 4;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            s->more_selected = (s->more_selected + 1) % 4;
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->more_selected == 0) {
                s->current_page = MENU_TIME_SELECT;
                s->time_row_selected = 0;
            } else if (s->more_selected == 1) {
                s->current_page = MENU_SPEED_SELECT;
                s->speed_row_selected = 0;
            } else if (s->more_selected == 2) {
                s->current_page = MENU_DENSITY_SELECT;
                s->density_row_selected = 0;
            } else if (s->more_selected == 3) {
                save_config_to_disk(s);
                s->current_page = MENU_CONFIG;
            }
            return TRUE;
        }
    }
    else if (s->current_page == MENU_TIME_SELECT) {
        if (keyval == GDK_KEY_Left) {
            s->time_row_selected = (s->time_row_selected - 1 + 3) % 3;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            s->time_row_selected = (s->time_row_selected + 1) % 3;
            return TRUE;
        } else if (keyval == GDK_KEY_Up) {
            if (s->time_row_selected == 0) {
                s->build_time_sec += 0.5f;
                if (s->build_time_sec > 33.0f) s->build_time_sec = 1.0f;
            } else if (s->time_row_selected == 1) {
                s->hold_time_sec += 0.5f;
                if (s->hold_time_sec > 33.0f) s->hold_time_sec = 1.0f;
            }
            save_config_to_disk(s);
            return TRUE;
        } else if (keyval == GDK_KEY_Down) {
            if (s->time_row_selected == 0) {
                s->build_time_sec -= 0.5f;
                if (s->build_time_sec < 1.0f) s->build_time_sec = 33.0f;
            } else if (s->time_row_selected == 1) {
                s->hold_time_sec -= 0.5f;
                if (s->hold_time_sec < 1.0f) s->hold_time_sec = 33.0f;
            }
            save_config_to_disk(s);
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->time_row_selected == 2) {
                save_config_to_disk(s);
                s->current_page = MENU_MORE_SELECT;
            }
            return TRUE;
        }
    }
    else if (s->current_page == MENU_SPEED_SELECT) {
        if (keyval == GDK_KEY_Left) {
            s->speed_row_selected = (s->speed_row_selected - 1 + 2) % 2;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            s->speed_row_selected = (s->speed_row_selected + 1) % 2;
            return TRUE;
        } else if (keyval == GDK_KEY_Up) {
            if (s->speed_row_selected == 0) {
                s->rain_speed_setting++;
                if (s->rain_speed_setting > 10) s->rain_speed_setting = 0;
                save_config_to_disk(s);
                init_grid_engine(s, s->monitor_width, s->monitor_height);
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Down) {
            if (s->speed_row_selected == 0) {
                s->rain_speed_setting--;
                if (s->rain_speed_setting < 0) s->rain_speed_setting = 10;
                save_config_to_disk(s);
                init_grid_engine(s, s->monitor_width, s->monitor_height);
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->speed_row_selected == 1) {
                save_config_to_disk(s);
                s->current_page = MENU_MORE_SELECT;
            }
            return TRUE;
        }
    }
    else if (s->current_page == MENU_DENSITY_SELECT) {
        if (keyval == GDK_KEY_Left) {
            s->density_row_selected = (s->density_row_selected - 1 + 2) % 2;
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            s->density_row_selected = (s->density_row_selected + 1) % 2;
            return TRUE;
        } else if (keyval == GDK_KEY_Up) {
            if (s->density_row_selected == 0) {
                s->rain_density_setting++;
                if (s->rain_density_setting > 10) s->rain_density_setting = 0;
                save_config_to_disk(s);
                init_grid_engine(s, s->monitor_width, s->monitor_height);
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Down) {
            if (s->density_row_selected == 0) {
                s->rain_density_setting--;
                if (s->rain_density_setting < 0) s->rain_density_setting = 10;
                save_config_to_disk(s);
                init_grid_engine(s, s->monitor_width, s->monitor_height);
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Return) {
            if (s->density_row_selected == 1) {
                save_config_to_disk(s);
                s->current_page = MENU_MORE_SELECT;
            }
            return TRUE;
        }
    }
#endif
    return FALSE;
}

static void activate(GtkApplication *app, gpointer data) {
    AppState *s = data;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    
    GtkEventController *motion_controller = gtk_event_controller_motion_new();
    gtk_widget_add_controller(window, motion_controller);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, "window, decoration { background-color: rgba(0,0,0,0); }");
    gtk_style_context_add_provider_for_display(gtk_widget_get_display(window), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);
    
    GdkDisplay *display = gtk_widget_get_display(window);
    GListModel *monitors = gdk_display_get_monitors(display);
    s->monitor_width = 1920; s->monitor_height = 1080;
    if (g_list_model_get_n_items(monitors) > 0) {
        GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_item(monitors, 0));
        GdkRectangle geometry;
        gdk_monitor_get_geometry(monitor, &geometry);
        s->monitor_width = geometry.width; s->monitor_height = geometry.height;
        g_object_unref(monitor);
    }

    gtk_window_set_default_size(GTK_WINDOW(window), s->monitor_width, s->monitor_height);
    init_grid_engine(s, s->monitor_width, s->monitor_height);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), render_grid_pipeline, s, NULL);
    gtk_widget_add_tick_callback(drawing_area, (GtkTickCallback)gtk_widget_queue_draw, NULL, NULL);

#ifndef PRODUCTION_MODE
    GtkEventController *controller = gtk_event_controller_key_new();
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_pressed), app);
    gtk_widget_add_controller(window, controller);
#endif

    gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    srand(time(NULL));
    AppState state = {0};
    
    strncpy(state.source_file_path, argv[0], sizeof(state.source_file_path) - 1);
    char *dot_c = strrchr(state.source_file_path, '.');
    if (!dot_c || strcmp(dot_c, ".c") != 0) {
        char base_path[512];
        strncpy(base_path, argv[0], sizeof(base_path) - 1);
        snprintf(state.source_file_path, sizeof(state.source_file_path), "%s.c", base_path);
        if (access(state.source_file_path, F_OK) != 0) {
            strcpy(state.source_file_path, "aetherrain.c");
        }
    }

    load_config_from_disk(&state);
    
    if (argc > 1) {
        strcpy(state.target_text, argv[1]);
    } else {
#ifdef PRODUCTION_MODE
        char saved_text[256];
        strcpy(saved_text, state.target_text);
        if (strcmp(saved_text, "SYSTEM INITIALIZATION") == 0 || strlen(saved_text) == 0) {
            const char *user_env = getenv("USER");
            if (user_env && strlen(user_env) > 0) {
                snprintf(state.target_text, sizeof(state.target_text), "WELCOME BACK %s", user_env);
            }
        } else {
            strcpy(state.target_text, saved_text);
        }
#endif
    }

    state.current_page = MENU_MAIN;
    state.install_status = INSTALL_IDLE;

    GtkApplication *app = gtk_application_new("com.mrprog.aetherrain", G_APPLICATION_DEFAULT_FLAGS);
    g_object_set_data(G_OBJECT(app), "app_state", &state);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    free(state.reveal_state); free(state.reveal_order); free(state.glitch_state); free(state.neon_opacity); free(state.text_outro); free(state.streams);
    return status;
}
