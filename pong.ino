#include <TVout.h>
#include <font8x8.h>
#include <font4x6.h>
#include "title.h"

#define button_A 2
#define button_B 3
#define button_C 4
#define button_D 5
#define left_pot A0
#define right_pot A1

TVout TV;

const int resolution_x = 128;
const int resolution_y = 64;
//const int image_h_offset = 0;


const int paddle_width = 4;
const int paddle_height = 8;
const int ball_width = 3;
const int ball_height = 2;
const float ball_speed_x = 1;
const float ball_speed_y = 1;

const int hidden_tolerance = ball_height;

const float cpu_min_speed = 0.5;
const float cpu_max_speed = 2;

const int serve_timeout = 1000;
const int attract_timeout = 15000;
const int win_timeout = 3000;

int game_state = 0;
/* 0 - main menu
 * 1 - 1P game
 * 2 - 2P game
 * 3 - CPU game
 * 4 - attract mode
 */
 
const int score_options = 6;
int max_score_options[score_options] = { 0, 1, 3, 5, 7, 9};
int max_score_index = 3;
int max_score = max_score_options[max_score_index];

const int button_sound[] = {440, 20};
const int paddle_sound[] = {1000, 20};
const int bounce_sound[] = {500, 20};
const int score_sound[] = {250, 250};
const int game_over_sound[] = {750, 500};

//some precalculations
float double_ball_speed_x = ball_speed_x * 2;
int half_res_x = resolution_x / 2;
int half_res_y = resolution_y / 2;
int half_pad_h = paddle_height / 2;
int quarter_pad_h = paddle_height / 4;
int half_ball_w = ball_width / 2 + 1;
int half_ball_h = ball_height / 2 + 1;

float left_paddle_position, right_paddle_position;
int left_score, right_score;
float ball_vel_x, ball_vel_y;
float ball_x, ball_y;

unsigned long time_last_lost;
unsigned long time_last_input = millis();

bool button_A_pressed;
bool button_B_pressed;
bool button_C_pressed;
bool button_D_pressed;

bool game_over_sound_played;

int left_cpu_preferred_side, right_cpu_preferred_side;
float left_cpu_paddle_speed, right_cpu_paddle_speed;

int left_score_digits;

void setup(){
    TV.begin(NTSC, resolution_x, resolution_y);
    TV.clear_screen();
    pinMode(button_A, INPUT);
    pinMode(button_B, INPUT);
    pinMode(button_C, INPUT);
    pinMode(button_D, INPUT);
    //display.output_delay = image_h_offset;
}

void play_sound(int* sound){
    TV.noTone();
    TV.tone(*sound, *(sound + 1));
}

void update_paddles(){
    if(game_state == 0 or game_state == 1 or game_state == 2){
        left_paddle_position = (analogRead(left_pot) / 1023.0) * resolution_y;
    }
    
    if(game_state == 0 or game_state == 2){
        right_paddle_position = (analogRead(right_pot) / 1023.0)  * resolution_y;
    }

    if(game_state == 3 or game_state == 4){
        if(ball_vel_y == 0 and left_cpu_preferred_side == 0){
            left_cpu_preferred_side = random(2) ? 1 : -1;
        }
        if(ball_vel_y != 0){
            left_cpu_preferred_side = 0;
        }
        if(ball_y + left_cpu_preferred_side * (quarter_pad_h + 1) - ball_height < left_paddle_position){
            left_paddle_position -= left_cpu_paddle_speed;
        }
        if(ball_y + left_cpu_preferred_side * (quarter_pad_h + 1) > left_paddle_position){
            left_paddle_position += left_cpu_paddle_speed;
        }
    }

    if(game_state == 1 or game_state == 3 or game_state == 4){
        if(ball_vel_y == 0 and right_cpu_preferred_side == 0){
            right_cpu_preferred_side = random(2) ? 1 : -1;
        }
        if(ball_vel_y != 0){
            right_cpu_preferred_side = 0;
        }
        if(ball_y + right_cpu_preferred_side * (quarter_pad_h + 1) - ball_height < right_paddle_position){
            right_paddle_position -= right_cpu_paddle_speed;
        }
        if(ball_y + right_cpu_preferred_side * (quarter_pad_h + 1) > right_paddle_position){
            right_paddle_position += right_cpu_paddle_speed;
        }
    }
    
//    TV.select_font(font4x6);
//    TV.set_cursor(0,0);
//    TV.println(left_cpu_paddle_speed);
//    TV.println(right_cpu_paddle_speed);

    if(left_paddle_position < half_pad_h){
        left_paddle_position = half_pad_h;
    }
    if(left_paddle_position > resolution_y - half_pad_h){
        left_paddle_position = resolution_y - half_pad_h;
    }
    if(right_paddle_position < half_pad_h){
        right_paddle_position = half_pad_h;
    }
    if(right_paddle_position > resolution_y - half_pad_h){
        right_paddle_position = resolution_y - half_pad_h;
    }
    
    TV.draw_rect(0, (float)left_paddle_position - half_pad_h, paddle_width, paddle_height, 1, 1);
    TV.draw_rect(resolution_x - paddle_width - 1, (float)right_paddle_position - half_pad_h, paddle_width, paddle_height, 1, 1);
}

void draw_score(){

    if(left_score < 10){
        left_score_digits = 1;
    }
    if(left_score >= 10){
        left_score_digits = 2;
    }
    
    TV.select_font(font8x8);
    TV.set_cursor(half_res_x - 5 - left_score_digits * 8, 2);
    TV.print(left_score);
    TV.set_cursor(half_res_x + 4, 2);
    TV.print(right_score);
    if(game_state == 4){
        TV.select_font(font4x6);
        TV.set_cursor(half_res_x - 8, 12);
        TV.print("DEMO");
    }
}

void draw_field(){
    TV.draw_row(0, 0, resolution_x, 1);
    TV.draw_row(resolution_y - 1, 0, resolution_x, 1);
    
    for(int i = 1; i < resolution_y - 1; i += 2){
        TV.set_pixel(half_res_x - 1, i, 1);
    }
}

void draw_title(){
    TV.bitmap(half_res_x - 32, half_res_y - 24, title_bitmap);
    TV.select_font(font4x6);
    TV.set_cursor(half_res_x - 20, half_res_y + 16);
    TV.print(max_score);
}

void parse_buttons(){
    if(digitalRead(button_A) and not button_A_pressed){
        play_sound(button_sound);
        game_state = 1;
        initialize_game();
    }
    button_A_pressed = digitalRead(button_A);
    if(digitalRead(button_B) and not button_B_pressed){
        play_sound(button_sound);
        game_state = 2;
        initialize_game();
    }
    button_B_pressed = digitalRead(button_B);
    if(digitalRead(button_C) and not button_C_pressed){
        play_sound(button_sound);
        game_state = 3;
        initialize_game();
    }
    button_C_pressed = digitalRead(button_C);
    if(digitalRead(button_D) and not button_D_pressed){
        play_sound(button_sound);
        max_score_index++;
        if(max_score_index >= score_options){
            max_score_index %= score_options;
        }
        max_score = max_score_options[max_score_index];
        time_last_input = millis();
    }
    button_D_pressed = digitalRead(button_D);
}

void update_ball(){
    if(left_score == max_score and max_score != 0){
        TV.select_font(font4x6);
        TV.set_cursor(half_res_x - 20, half_res_y - 3);
        TV.print("LEFT WINS!");
        if(not game_over_sound_played){
            play_sound(game_over_sound);
            game_over_sound_played = true;
        }
        if(millis() - time_last_lost > win_timeout){
            game_state = 0;
            time_last_input = millis();
            max_score = max_score_options[max_score_index];
        }
    }else if(right_score == max_score and max_score != 0){
        TV.select_font(font4x6);
        TV.set_cursor(half_res_x - 22, half_res_y - 3);
        TV.print("RIGHT WINS!");
        if(not game_over_sound_played){
            play_sound(game_over_sound);
            game_over_sound_played = true;
        }
        if(millis() - time_last_lost > win_timeout){
            game_state = 0;
            time_last_input = millis();
            max_score = max_score_options[max_score_index];
        }
    }else{
        if(millis() - time_last_lost < serve_timeout){
            randomSeed(millis());
            ball_x = half_res_x - half_ball_w;
            ball_y = half_res_y - half_ball_h;
            ball_vel_x = random(2) ? ball_speed_x : -ball_speed_x;
            ball_vel_y = random(2) ? ball_speed_y : -ball_speed_y;
        }else{
            ball_x += ball_vel_x;
            if(ball_x <= paddle_width){
                if(ball_y + hidden_tolerance > left_paddle_position - half_pad_h and ball_y + ball_height - hidden_tolerance < left_paddle_position + half_pad_h){
                    ball_x = paddle_width + 1;
                    ball_vel_x = ball_speed_x;
                    if(ball_y - ball_height + 1 <= left_paddle_position - quarter_pad_h){
                        ball_vel_y = -ball_speed_y;
                    }
                    if(ball_y - ball_height + 1 > left_paddle_position - quarter_pad_h and ball_y < left_paddle_position + quarter_pad_h){
                        ball_vel_y = 0;
                        ball_vel_x = double_ball_speed_x;
                    }
                    if(ball_y >= left_paddle_position + quarter_pad_h){
                        ball_vel_y = ball_speed_y;
                    }
                    play_sound(paddle_sound);
                    left_cpu_preferred_side = 0;
                    right_cpu_preferred_side = 0;
                    left_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                    right_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                }
                if(ball_x - ball_width + 1 < 0){
                    time_last_lost = millis();
                    right_score++;
                    play_sound(score_sound);
                    left_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                    right_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                }
            }
            if(ball_x + ball_width - 1 > resolution_x - 2 - paddle_width){
                if(ball_y + hidden_tolerance > right_paddle_position - half_pad_h and ball_y + ball_height - hidden_tolerance < right_paddle_position + half_pad_h){
                    ball_x = resolution_x - 2 - paddle_width - ball_width;
                    ball_vel_x = -ball_speed_x;
                    if(ball_y - ball_height + 1 <= right_paddle_position - quarter_pad_h){
                        ball_vel_y = -ball_speed_y;
                    }
                    if(ball_y - ball_height + 1 > right_paddle_position - quarter_pad_h and ball_y < right_paddle_position + quarter_pad_h){
                        ball_vel_y = 0;
                        ball_vel_x = -double_ball_speed_x;
                    }
                    if(ball_y >= right_paddle_position + quarter_pad_h){
                        ball_vel_y = ball_speed_y;
                    }
                    play_sound(paddle_sound);
                    left_cpu_preferred_side = 0;
                    right_cpu_preferred_side = 0;
                    left_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                    right_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                }
                if(ball_x > resolution_x - 1){
                    time_last_lost = millis();
                    left_score++;
                    play_sound(score_sound);
                    left_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                    right_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
                }
            }
            ball_y += ball_vel_y;
            if(ball_y - ball_height + 1 < 1){
                ball_y = 1;
                ball_vel_y *= -1;
                play_sound(bounce_sound);
            }
            if(ball_y + ball_height - 1 > resolution_y - 2){
                ball_y = resolution_y - 2 - ball_height;
                ball_vel_y *= -1;
                play_sound(bounce_sound);
            }
        }
        TV.draw_rect(ball_x, ball_y, ball_width - 1, ball_height - 1, 1, 1);
    }
    
}

void initialize_game(){
    left_score = 0;
    right_score = 0;
    time_last_lost = millis();
    game_over_sound_played = false;
    left_cpu_preferred_side = 0;
    right_cpu_preferred_side = 0;
    left_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
    right_cpu_paddle_speed = random(cpu_min_speed * 100, cpu_max_speed * 100) / 100.0;
}

void attract_check_inputs(){
    if(digitalRead(button_A) and not button_A_pressed){
        game_state = 0;
        time_last_input = millis();
        max_score = max_score_options[max_score_index];
    }
    button_A_pressed = digitalRead(button_A);
    if(digitalRead(button_B) and not button_B_pressed){
        game_state = 0;
        time_last_input = millis();
        max_score = max_score_options[max_score_index];
    }
    button_B_pressed = digitalRead(button_B);
    if(digitalRead(button_C) and not button_C_pressed){
        game_state = 0;
        time_last_input = millis();
        max_score = max_score_options[max_score_index];
    }
    button_C_pressed = digitalRead(button_C);
    if(digitalRead(button_D) and not button_D_pressed){
        game_state = 0;
        time_last_input = millis();
        max_score = max_score_options[max_score_index];
    }
    button_D_pressed = digitalRead(button_D);
}

void loop(){
    TV.clear_screen();
    update_paddles();
    draw_field();
    switch(game_state){
        case 0:
            parse_buttons();
            draw_title();
            break;
        case 4:
            attract_check_inputs();
        case 1:
        case 2:
        case 3:
            draw_score();
            update_ball();
            break;
        
    }

    if(millis() - time_last_input > attract_timeout and game_state == 0){
        game_state = 4;
        max_score = 1;
        initialize_game();
    }
    
    TV.delay_frame(1);
}
