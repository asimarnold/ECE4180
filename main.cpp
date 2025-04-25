//Implementation
#include "mbed.h"
#include "uLCD.hpp"
#include <chrono>  // Add this include for duration_cast
#include <cmath> 

// Peripheral Initialization
uLCD uLCD(p9, p10, p11, uLCD::BAUD_115200); // TX, RX, RESET pins for uLCD
PwmOut paddle_pot(p21);       // PWM for paddle potentiometer
PwmOut leftSpeaker(p23);      // Speaker output
PwmOut rightSpeaker(p22);      // Speaker output
Timer game_timer;             // Timer for game duration
Timer   beep_timer;
bool    beep_state = false;

// Game variables
float ball_x = 0.5f;          // Normalized ball position (0-1)
float ball_y = 0.5f;
float ball_speed_x = 0.02f;   // Ball velocity
float ball_speed_y = 0.02f;
float paddle_pos = 0.5f;      // Paddle position (0-1)
float paddle_size = 0.1f;
int score = 0;
bool game_active = false;
// pick a tone frequency (2 kHz here)
const float TONE_FREQ = 2000.0f;
const float TONE_PERIOD = 1.0f / TONE_FREQ;


// Add button input (adjust pin as needed)
DigitalIn start_button(p15);  // Add a physical button for starting the game
DigitalIn mode_button(p16, PullDown);   // Button to cycle between modes


// Add analog input for paddle control
AnalogIn pot_input(p20);      // Analog input for potentiometer

// Define game modes
enum GameMode { EASY, MEDIUM, HARD, IMPOSSIBLE };
GameMode current_mode = MEDIUM; // Default mode

void update_speaker() {
    // compute beep interval
    float total_speed   = fabs(ball_speed_x) + fabs(ball_speed_y);
    float beep_interval = 1.0f / (total_speed * 50.0f);

    // check timer
    float elapsed = beep_timer.read();
    if (elapsed > beep_interval) {
        // toggle state
        beep_state = !beep_state;
        beep_timer.reset();
        beep_timer.start();
    }

    // drive PWM on one channel only
    if (ball_speed_x < 0.0f) {
        leftSpeaker.write(beep_state ? 0.5f : 0.0f);
        rightSpeaker.write(0.0f);
    } else {
        leftSpeaker.write(0.0f);
        rightSpeaker.write(beep_state ? 0.5f : 0.0f);
    }
}

void update_ball() {
    // Move ball
    ball_x += ball_speed_x;
    ball_y += ball_speed_y;
   
    // Wall collisions (left/right)
    if (ball_x <= 0 || ball_x >= 1.0f) {
        ball_speed_x *= -1;
    }
   
    // Top wall collision
    if (ball_y <= 0) {
        ball_speed_y *= -1;
    }
   
    // Paddle collision
    if (ball_y >= 0.9f && abs(ball_x - paddle_pos) < paddle_size) {
        ball_speed_y *= -1;
        score++;
       
        // Increase speed slightly
        ball_speed_x *= 1.05f;
        ball_speed_y *= 1.05f;
    }
   
    // Game over if ball goes below paddle
    if (ball_y >= 1.0f) {
        game_active = false;
    }
}


void update_display() {
    uLCD.cls();
   
    // Draw paddle (convert from normalized to screen coordinates)
    int paddle_start = (int)(paddle_pos * 128) - (int)(paddle_size * 128 / 2);
    int paddle_end = (int)(paddle_pos * 128) + (int)(paddle_size * 128 / 2);
    paddle_start = paddle_start < 0 ? 0 : paddle_start;
    paddle_end = paddle_end > 127 ? 127 : paddle_end;
    uLCD.drawLine(paddle_start, 120, paddle_end, 120, 0xFFFF); // White
   
    // Draw ball
    int ball_x_pos = (int)(ball_x * 128);
    int ball_y_pos = (int)(ball_y * 128);
    uLCD.drawCircleFilled(ball_x_pos, ball_y_pos, 2, 0xFFFF); // White
   
    // Draw score
    uLCD.locate(0, 0);
    char score_str[20];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    uLCD.print(score_str);
}


void read_paddle_input() {
    // Read potentiometer value
    paddle_pos = pot_input.read(); // Reads 0.0-1.0 from analog input
}

void reset_game() {
    // reset positions & velocities
    ball_x       = 0.5f;
    ball_y       = 0.5f;
    ball_speed_x = 0.02f;
    ball_speed_y = 0.02f;
    paddle_pos   = 0.5f;
    score        = 0;
    paddle_size  = 0.1f; // Default to medium size

    // Adjust for current game mode
    if (current_mode == EASY) {
        paddle_size = 0.3f; // Big paddle for easy
        ball_speed_x = 0.02f;
        ball_speed_y = 0.02f;
    } else if (current_mode == MEDIUM) {
        paddle_size = 0.2f; // Medium paddle
        ball_speed_x = 0.02f;
        ball_speed_y = 0.02f;
    } else if (current_mode == HARD) {
        paddle_size = 0.1f; // Small paddle for hard
        ball_speed_x = 0.03f;
        ball_speed_y = 0.03f;
    } else if (current_mode == IMPOSSIBLE) {
        paddle_size = 0.05f; // Very small paddle for impossible
        ball_speed_x = 0.04f;
        ball_speed_y = 0.04f;
    }

    // reset timers
    game_timer.reset();
    beep_timer.reset();

    // clear any lingering audio
    leftSpeaker = 0.0f;
    rightSpeaker = 0.0f;

    // start timers & activate
    game_timer.start();
    beep_timer.start();
    game_active = true;
}

void cycle_game_mode() {
    while (!start_button.read()){
    // Cycle through game modes with button press
    if (mode_button.read()) {
        if (current_mode == IMPOSSIBLE) {
            current_mode = EASY; // Cycle back to EASY
        } else {
            current_mode = static_cast<GameMode>(static_cast<int>(current_mode) + 1);
        }

        // Display current mode on screen
        uLCD.cls();
        uLCD.locate(1,1);
        switch (current_mode) {
            case EASY:
                uLCD.print("Mode: EASY");
                break;
            case MEDIUM:
                uLCD.print("Mode: MEDIUM");
                break;
            case HARD:
                uLCD.print("Mode: HARD");
                break;
            case IMPOSSIBLE:
                uLCD.print("Mode: IMPOSSIBLE");
                break;
        }
        ThisThread::sleep_for(1000ms); // Show mode for a second
    }
    }
}


int main() {
    // configure audio PWM once
    leftSpeaker.period(TONE_PERIOD);
    rightSpeaker.period(TONE_PERIOD);
// initial splash
    uLCD.setTextColor(0xFFFF);
    uLCD.setFontSize(1,1);
    uLCD.cls();
    uLCD.locate(1,1);
    uLCD.print("Blind Pong");
    uLCD.locate(1,3);
    uLCD.print("Press LEFT button");
    uLCD.locate(1,4);
    uLCD.print("to cycle mode");

    uLCD.locate(1,6);
    uLCD.print("Press RIGHT button");
    uLCD.locate(1,7);
    uLCD.print("to start");

    // forever play/reset loop
    while (true) {
        // wait for start button
        while (!start_button.read()) {
            cycle_game_mode(); // Allow mode cycling with the button
            ThisThread::sleep_for(50ms);
        }
        ThisThread::sleep_for(200ms);  // debounce

        // start/reset the game
        reset_game();

        // game session
        while (game_active) {
            read_paddle_input();
            update_ball();
            update_speaker();
            update_display();
            ThisThread::sleep_for(20ms);
        }

        // Game Over screen
        uLCD.cls();
        uLCD.locate(1,1);
        uLCD.print("Game Over!");
        char buf[32];
        snprintf(buf, sizeof(buf), "Score: %d", score);
        uLCD.locate(1,3);
        uLCD.print(buf);
        float game_time = game_timer.read();
        int secs   = (int)game_time;
        int tenths = (int)(game_time * 10.0f) % 10;
        // use game_timer.read() because it was reset at round start
        snprintf(buf, sizeof(buf), "Time: %d.%ds", secs, tenths);
        uLCD.locate(1,5);
        uLCD.print(buf);

        // mute audio
        leftSpeaker.write(0.0f);
        rightSpeaker.write(0.0f);

        ThisThread::sleep_for(1500ms);

        // prompt for restart
        uLCD.cls();
        uLCD.locate(1,2);
        uLCD.print("Press to restart");
    }
}
