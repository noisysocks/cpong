#include "raylib.h"
#include "raymath.h"
#include <math.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400

#define BAT_OFFSET 20.0f
#define BAT_SIZE (Vector2){ 10.0f, 70.0f }
#define BAT_ORIGIN (Vector2){ 5.0f, 35.0f }
#define BAT_DAMPING 0.95f

#define PLAYER_ACCELERATION 0.6f

#define COMPUTER_SPEED 0.3f
#define COMPUTER_REACT_DISTANCE 25.0f

#define BALL_RADIUS 10.0f
#define BALL_INITIAL_POSITION (Vector2){ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f }
#define BALL_INITIAL_ANGLE (PI / 9.0f)
#define BALL_SPEED 6.6f
#define BALL_MAX_ANGULAR_VELOCITY 0.03f
#define BALL_FRICTION 0.002f

typedef struct {
    float position;
    float velocity;
} Bat;

typedef struct {
    Vector2 position;
    float angle;
    float angularVelocity;
} Ball;

void MoveBat(Bat *bat) {
    bat->position += bat->velocity;
    bat->velocity *= BAT_DAMPING;

    const float top = bat->position - BAT_ORIGIN.y;
    const float bottom = bat->position + BAT_ORIGIN.y;

    if (top < 0.0f) {
        bat->position = BAT_ORIGIN.y;
        bat->velocity = -bat->velocity;
    }
    else if (bottom > WINDOW_HEIGHT) {
        bat->position = WINDOW_HEIGHT - BAT_ORIGIN.y;
        bat->velocity = -bat->velocity;
    }
}

void MoveBall(Ball *ball, const Bat *playerBat, const Bat *computerBat) {
    const Vector2 direction = (Vector2){ cosf(ball->angle), sinf(ball->angle) };
    const Vector2 velocity = Vector2Scale(direction, BALL_SPEED);
    ball->position = Vector2Add(ball->position, velocity);

    ball->angle = fmodf(ball->angle + ball->angularVelocity, PI * 2.0f);
    ball->angularVelocity = Clamp(ball->angularVelocity, -BALL_MAX_ANGULAR_VELOCITY, BALL_MAX_ANGULAR_VELOCITY);

    const float ballLeft = ball->position.x - BALL_RADIUS;
    const float ballRight = ball->position.x + BALL_RADIUS;
    const float ballTop = ball->position.y - BALL_RADIUS;
    const float ballBottom = ball->position.y + BALL_RADIUS;

    if (ballTop < 0.0f) {
        ball->position.y = BALL_RADIUS;
        ball->angle = atan2f(-velocity.y, velocity.x);
    }
    else if (ballBottom > WINDOW_HEIGHT) {
        ball->position.y = WINDOW_HEIGHT - BALL_RADIUS;
        ball->angle = atan2f(-velocity.y, velocity.x);
    }

    const float playerBatRight = BAT_OFFSET + BAT_SIZE.x;
    const float playerBatTop = playerBat->position - BAT_ORIGIN.y;
    const float playerBatBottom = playerBat->position + BAT_ORIGIN.y;
    const float computerBatLeft = WINDOW_WIDTH - BAT_OFFSET - BAT_SIZE.x;
    const float computerBatTop = computerBat->position - BAT_ORIGIN.y;
    const float computerBatBottom = computerBat->position + BAT_ORIGIN.y;

    if (ballRight < 0.0f) {
        ball->position = BALL_INITIAL_POSITION;
        ball->angle = BALL_INITIAL_ANGLE + PI;
    }
    else if (ballLeft > WINDOW_WIDTH) {
        ball->position = BALL_INITIAL_POSITION;
        ball->angle = BALL_INITIAL_ANGLE;
    }
    else if (
        ballLeft < playerBatRight &&
        ball->position.y > playerBatTop &&
        ball->position.y < playerBatBottom &&
        velocity.x < 0.0f
    ) {
        ball->position.x = playerBatRight + BALL_RADIUS;
        ball->angle = atan2f(velocity.y, -velocity.x);
        ball->angularVelocity += playerBat->velocity * BALL_FRICTION;
    }
    else if (
        ballRight > computerBatLeft &&
        ball->position.y > computerBatTop &&
        ball->position.y < computerBatBottom &&
        velocity.x > 0.0f
    ) {
        ball->position.x = computerBatLeft - BALL_RADIUS;
        ball->angle = atan2f(velocity.y, -velocity.x);
        ball->angularVelocity += computerBat->velocity * BALL_FRICTION;
    }
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CPong");
    SetTargetFPS(60);

    Bat playerBat = {
        .position = WINDOW_HEIGHT / 2.0f,
    };
    Bat computerBat = {
        .position = WINDOW_HEIGHT / 2.0f,
    };

    Ball ball = {
        .position = BALL_INITIAL_POSITION,
        .angle = BALL_INITIAL_ANGLE,
    };

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_UP)) {
            playerBat.velocity -= PLAYER_ACCELERATION;
        }
        if (IsKeyDown(KEY_DOWN)) {
            playerBat.velocity += PLAYER_ACCELERATION;
        }

        if (cosf(ball.angle) > 0.0f) {
            if (ball.position.y < computerBat.position - COMPUTER_REACT_DISTANCE) {
                computerBat.velocity -= COMPUTER_SPEED;
            }
            else if (ball.position.y > computerBat.position + COMPUTER_REACT_DISTANCE) {
                computerBat.velocity += COMPUTER_SPEED;
            }
        }

        MoveBat(&playerBat);
        MoveBat(&computerBat);

        MoveBall(&ball, &playerBat, &computerBat);

        BeginDrawing();
        {
            ClearBackground(BLACK);

            DrawRectangleV(
                (Vector2){ BAT_OFFSET, playerBat.position - BAT_ORIGIN.y },
                BAT_SIZE,
                WHITE
            );

            DrawRectangleV(
                (Vector2){ WINDOW_WIDTH - BAT_OFFSET - BAT_SIZE.x, computerBat.position - BAT_ORIGIN.y },
                BAT_SIZE,
                WHITE
            );

            DrawRectanglePro(
                (Rectangle) {
                    .x = ball.position.x,
                    .y = ball.position.y,
                    .width = BALL_RADIUS * 2.0f,
                    .height = BALL_RADIUS * 2.0f,
                },
                (Vector2){ BALL_RADIUS, BALL_RADIUS },
                ball.angle * RAD2DEG,
                WHITE
            );

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
