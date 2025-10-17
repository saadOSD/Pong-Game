#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <sstream>

// Define M_PI if not available (for cross-platform compatibility)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Configuration Constants ---
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PADDLE_HEIGHT = 100.0f;
const float PADDLE_WIDTH = 15.0f;
const float PADDLE_SPEED = 10.0f; // Human paddle speed
// AI difficulty
const float AI_LAG_FACTOR = 0.75f; // Slightly harder AI
const float AI_SPEED = 8.0f; // Increased AI speed for better tracking
const float BALL_RADIUS = 10.0f;
const int GAME_SPEED_MS = 16;
const int WINNING_SCORE = 10;

// --- Game State Variables ---
// PADDLE 1 (Left) is AI
float paddle1Y = WINDOW_HEIGHT / 2.0f;
// PADDLE 2 (Right) is Human Player
float paddle2Y = WINDOW_HEIGHT / 2.0f;

// Ball position and velocity
float ballX = WINDOW_WIDTH / 2.0f;
float ballY = WINDOW_HEIGHT / 2.0f;
float ballVelX = 5.0f;
float ballVelY = 0.0f;
float ballSpeedIncrease = 0.5f;

// Scores
int score1 = 0; // AI's Score (Left)
int score2 = 0; // Player's Score (Right)

// Input handling flags (Now using special key flags for Paddle 2)
bool paddle2Up = false;
bool paddle2Down = false;

// Game state
bool gameRunning = true;

// --- Function Prototypes ---
void initGL();
void drawPaddle(float x, float y);
void drawBall();
void drawText(std::string text, float x, float y);
void resetBall(int scoringPlayer);
void updateGame(int value);
void updatePaddles();
void updateAIPaddle();
void checkBallCollisions();
void display();
void reshape(int w, int h);
void keyboardDown(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void specialDown(int key, int x, int y);
void specialUp(int key, int x, int y);


// --- Drawing Functions ---
void drawPaddle(float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(x, y + PADDLE_HEIGHT / 2.0f);
        glVertex2f(x + PADDLE_WIDTH, y + PADDLE_HEIGHT / 2.0f);
        glVertex2f(x + PADDLE_WIDTH, y - PADDLE_HEIGHT / 2.0f);
        glVertex2f(x, y - PADDLE_HEIGHT / 2.0f);
    glEnd();
}

void drawBall() {
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(ballX, ballY);
        int segments = 20;
        for (int i = 0; i <= segments; i++) {
            float angle = i * 2.0f * M_PI / segments;
            glVertex2f(ballX + std::cos(angle) * BALL_RADIUS,
                       ballY + std::sin(angle) * BALL_RADIUS);
        }
    glEnd();
}

void drawText(std::string text, float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (size_t i = 0; i < text.length(); ++i) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
    }
}

// ------------------------------------

// --- Game Logic ---

void resetBall(int scoringPlayer) {
    ballX = WINDOW_WIDTH / 2.0f;
    ballY = WINDOW_HEIGHT / 2.0f;
    ballVelY = 0.0f;

    // Player 1 is AI, Player 2 is Human
    if (scoringPlayer == 1) {
        // Ball moves towards Human Player (Paddle 2)
        ballVelX = 5.0f;
    } else {
        // Ball moves towards AI (Paddle 1)
        ballVelX = -5.0f;
    }

    float angle = (std::rand() % 90) - 45;
    angle = angle * (M_PI / 180.0f);
    float speed = std::abs(ballVelX);
    ballVelX = std::cos(angle) * (ballVelX > 0 ? speed : -speed);
    ballVelY = std::sin(angle) * speed;
}

void updatePaddles() {
    // Player movement (Paddle 2, Right side, Up/Down arrows)
    if (paddle2Up) {
        paddle2Y = std::min(paddle2Y + PADDLE_SPEED, WINDOW_HEIGHT - PADDLE_HEIGHT / 2.0f);
    }
    if (paddle2Down) {
        paddle2Y = std::max(paddle2Y - PADDLE_SPEED, PADDLE_HEIGHT / 2.0f);
    }

    // AI movement (Paddle 1, Left side)
    updateAIPaddle();
}

/**
 * @brief Corrected AI Paddle update function to eliminate jittering/shaking.
 * * It calculates the precise movement needed and limits the movement distance
 * to prevent overshooting the target position.
 */
void updateAIPaddle() {
    // AI only tracks the ball when it's coming towards it (ballVelX < 0)
    if (ballVelX < 0) {
        // Target Y is a weighted average of ballY and center (AI_LAG_FACTOR)
        float targetY = ballY * AI_LAG_FACTOR + (WINDOW_HEIGHT / 2.0f) * (1.0f - AI_LAG_FACTOR);

        // Calculate the difference/distance to the target
        float difference = targetY - paddle1Y;

        if (std::abs(difference) > 0.1f) { // Small threshold to stop movement near the target
            float moveDistance = AI_SPEED;

            // Check if the difference is smaller than the max speed (prevent overshoot)
            if (std::abs(difference) < AI_SPEED) {
                moveDistance = std::abs(difference);
            }

            // Apply the move distance in the correct direction
            if (difference > 0) { // Target is above the paddle
                paddle1Y += moveDistance;
            } else { // Target is below the paddle
                paddle1Y -= moveDistance;
            }
        }
    }

    // Ensure the paddle stays within bounds after the move
    paddle1Y = std::min(paddle1Y, WINDOW_HEIGHT - PADDLE_HEIGHT / 2.0f);
    paddle1Y = std::max(paddle1Y, PADDLE_HEIGHT / 2.0f);
}


void checkBallCollisions() {
    // 1. Wall Collision (Top/Bottom)
    if (ballY + BALL_RADIUS > WINDOW_HEIGHT || ballY - BALL_RADIUS < 0) {
        ballVelY *= -1.0f;
        ballY = std::max(BALL_RADIUS, std::min(ballY, WINDOW_HEIGHT - BALL_RADIUS));
    }

    // 2. Paddle 1 Collision (Left Side - AI)
    float paddle1X = PADDLE_WIDTH * 2.0f;
    if (ballX - BALL_RADIUS < paddle1X + PADDLE_WIDTH && ballVelX < 0) {
        if (ballY < paddle1Y + PADDLE_HEIGHT / 2.0f && ballY > paddle1Y - PADDLE_HEIGHT / 2.0f) {

            float relativeY = ballY - paddle1Y;
            float normalRelativeY = relativeY / (PADDLE_HEIGHT / 2.0f);

            float currentSpeed = std::sqrt(ballVelX * ballVelX + ballVelY * ballVelY);
            currentSpeed += ballSpeedIncrease;

            float maxAngle = M_PI / 4.0f;
            float angle = normalRelativeY * maxAngle;

            ballVelX = std::cos(angle) * currentSpeed;
            ballVelY = std::sin(angle) * currentSpeed;
            ballVelX = std::abs(ballVelX);
        }
    }

    // 3. Paddle 2 Collision (Right Side - Human)
    float paddle2X = WINDOW_WIDTH - PADDLE_WIDTH * 3.0f;
    if (ballX + BALL_RADIUS > paddle2X && ballVelX > 0) {
        if (ballY < paddle2Y + PADDLE_HEIGHT / 2.0f && ballY > paddle2Y - PADDLE_HEIGHT / 2.0f) {

            float relativeY = ballY - paddle2Y;
            float normalRelativeY = relativeY / (PADDLE_HEIGHT / 2.0f);

            float currentSpeed = std::sqrt(ballVelX * ballVelX + ballVelY * ballVelY);
            currentSpeed += ballSpeedIncrease;

            float maxAngle = M_PI / 4.0f;
            float angle = normalRelativeY * maxAngle;

            ballVelX = std::cos(angle) * currentSpeed;
            ballVelY = std::sin(angle) * currentSpeed;
            ballVelX = -std::abs(ballVelX);
        }
    }

    // 4. Goal/Score (Left side - Player 2/Human scores)
    if (ballX - BALL_RADIUS < 0) {
        score2++;
        if (score2 >= WINNING_SCORE) {
            gameRunning = false;
        } else {
            resetBall(2);
        }
    }

    // 5. Goal/Score (Right side - Player 1/AI scores)
    if (ballX + BALL_RADIUS > WINDOW_WIDTH) {
        score1++;
        if (score1 >= WINNING_SCORE) {
            gameRunning = false;
        } else {
            resetBall(1);
        }
    }
}


void updateGame(int value) {
    if (gameRunning) {
        updatePaddles();
        ballX += ballVelX;
        ballY += ballVelY;
        checkBallCollisions();
    }

    glutPostRedisplay();
    glutTimerFunc(GAME_SPEED_MS, updateGame, 0);
}

// ------------------------------------

// --- GLUT Callbacks ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Draw the court center line (dashed)
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineStipple(5, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
        glVertex2f(WINDOW_WIDTH / 2.0f, 0);
        glVertex2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT);
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // Draw paddles and ball
    if (gameRunning) {
        drawPaddle(PADDLE_WIDTH * 2.0f, paddle1Y);
        drawPaddle(WINDOW_WIDTH - PADDLE_WIDTH * 3.0f, paddle2Y);
        drawBall();
    }

    // Draw Score (AI - Player)
    std::ostringstream scoreText;
    scoreText << score1 << " - " << score2;
    drawText(scoreText.str(), WINDOW_WIDTH / 2.0f - 40.0f, WINDOW_HEIGHT - 30.0f);

    // Draw instructions / Game Over
    if (!gameRunning) {
        std::ostringstream gameOverText;
        // Display score as (Player Score - AI Score) for the user
        if (score2 > score1) {
            gameOverText << "PLAYER WINS! (Score: " << score2 << "-" << score1 << ")";
        } else {
            gameOverText << "AI WINS! (Score: " << score1 << "-" << score2 << ")";
        }
        drawText(gameOverText.str(), WINDOW_WIDTH / 2.0f - 180.0f, WINDOW_HEIGHT / 2.0f);
        drawText("Press SPACE to restart.", WINDOW_WIDTH / 2.0f - 110.0f, WINDOW_HEIGHT / 2.0f - 30.0f);
    } else {
        drawText("AI", 50.0f, 20.0f);
        drawText("PLAYER: Up/Down Arrows", WINDOW_WIDTH - 200.0f, 20.0f);
    }

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

// --- Input Handling ---

// Handles non-special keys (Space and ESC)
void keyboardDown(unsigned char key, int x, int y) {
    switch (key) {
        case 32: // Spacebar to restart
            if (!gameRunning) {
                score1 = 0;
                score2 = 0;
                gameRunning = true;
                // Start with a random direction
                resetBall(std::rand() % 2 + 1);
            }
            break;
        case 27: // ESC key to exit
            exit(0);
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    // No regular keys used for movement
}

// Handles special keys (Up/Down Arrows for Paddle 2 - Human)
void specialDown(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:
            paddle2Up = true;
            break;
        case GLUT_KEY_DOWN:
            paddle2Down = true;
            break;
    }
}

void specialUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:
            paddle2Up = false;
            break;
        case GLUT_KEY_DOWN:
            paddle2Down = false;
            break;
    }
}

// --- Initialization & Main ---

void initGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glShadeModel(GL_FLAT);
    std::srand(std::time(0));
    resetBall(std::rand() % 2 + 1);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("OpenGL Single-Player Pong (Human on Right vs AI)");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    // Register handlers for regular keys (Space, ESC)
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);

    // Register handlers for special keys (Up/Down Arrows)
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    glutTimerFunc(GAME_SPEED_MS, updateGame, 0);

    glutMainLoop();
    return 0;
}
