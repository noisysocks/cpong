#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>

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

#define PARTICLE_MIN_SPEED 1.6f
#define PARTICLE_MAX_SPEED 5.0f
#define PARTICLE_COUNT 500
#define PARTICLE_RADIUS 1.0f

#define WAIT_DURATION 2.0

#define SCORE_FONT_SIZE 120
#define PASTEL_NAVY (Color){ 0x1A, 0x1B, 0x26, 0xFF }
#define PASTEL_NAVY_LIGHT  (Color){ 0x24, 0x25, 0x32, 0xFF }
#define PASTEL_PURPLE (Color){ 0xBA, 0xA2, 0xFF, 0xFF }
#define PASTEL_PINK (Color){ 0xFF, 0xB2, 0xE6, 0xFF }

typedef struct {
    float position;
    float velocity;
} Bat;

typedef struct {
    Vector2 position;
    float angle;
    float angularVelocity;
} Ball;

typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    struct Particle *prev;
    struct Particle *next;
} Particle;

typedef enum {
    BALL_MOVE_EVENT_NONE = 0,
    BALL_MOVE_EVENT_HIT,
    BALL_MOVE_EVENT_PLAYER_SCORE,
    BALL_MOVE_EVENT_COMPUTER_SCORE,
} BallMoveEvent;

typedef enum {
    PARTICLE_MOVE_EVENT_NONE = 0,
    PARTICLE_MOVE_EVENT_DIE,
} ParticleMoveEvent;

void ControlPlayerBat(Bat *playerBat) {
    if (IsKeyDown(KEY_UP)) {
        playerBat->velocity -= PLAYER_ACCELERATION;
    }
    if (IsKeyDown(KEY_DOWN)) {
        playerBat->velocity += PLAYER_ACCELERATION;
    }
}

void ControlComputerBat(Bat *computerBat, const Ball *ball) {
    if (cosf(ball->angle) > 0.0f) {
        if (ball->position.y < computerBat->position - COMPUTER_REACT_DISTANCE) {
            computerBat->velocity -= COMPUTER_SPEED;
        }
        else if (ball->position.y > computerBat->position + COMPUTER_REACT_DISTANCE) {
            computerBat->velocity += COMPUTER_SPEED;
        }
    }
}

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

BallMoveEvent MoveBall(Ball *ball, const Bat *playerBat, const Bat *computerBat) {
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
        ball->angularVelocity = 0.0f;
        return BALL_MOVE_EVENT_COMPUTER_SCORE;
    }
    if (ballLeft > WINDOW_WIDTH) {
        ball->position = BALL_INITIAL_POSITION;
        ball->angle = BALL_INITIAL_ANGLE;
        ball->angularVelocity = 0.0f;
        return BALL_MOVE_EVENT_PLAYER_SCORE;
    }

    if (
        ballLeft < playerBatRight &&
        ball->position.y > playerBatTop &&
        ball->position.y < playerBatBottom &&
        velocity.x < 0.0f
    ) {
        ball->position.x = playerBatRight + BALL_RADIUS;
        ball->angle = atan2f(velocity.y, -velocity.x);
        ball->angularVelocity += playerBat->velocity * BALL_FRICTION;
        return BALL_MOVE_EVENT_HIT;
    }
    if (
        ballRight > computerBatLeft &&
        ball->position.y > computerBatTop &&
        ball->position.y < computerBatBottom &&
        velocity.x > 0.0f
    ) {
        ball->position.x = computerBatLeft - BALL_RADIUS;
        ball->angle = atan2f(velocity.y, -velocity.x);
        ball->angularVelocity += computerBat->velocity * BALL_FRICTION;
        return BALL_MOVE_EVENT_HIT;
    }

    return BALL_MOVE_EVENT_NONE;
}

float GetRandomValueF(float min, float max) {
    const float amount = (float)rand() / RAND_MAX;
    return min + amount * (max - min);
}

Particle *CreateParticle(Vector2 position) {
    Particle *particle = (Particle *)malloc(sizeof(Particle));
    particle->position = position;
    particle->prev = NULL;
    particle->next = NULL;

    const float angle = GetRandomValueF(0.0f, PI * 2.0f);
    const Vector2 direction = (Vector2){ cosf(angle), sinf(angle) };
    const float speed = GetRandomValueF(PARTICLE_MIN_SPEED, PARTICLE_MAX_SPEED);
    particle->velocity = Vector2Scale(direction, speed);

    particle->color = (Color){
        128 + GetRandomValue(0, 127),
        128 + GetRandomValue(0, 127),
        128 + GetRandomValue(0, 127),
        255
    };

    return particle;
}

void DestroyParticle(Particle *particle) {
    free(particle);
}

void InsertParticle(Particle **head, Particle *particle) {
    particle->prev = NULL;
    particle->next = *head;
    if (*head != NULL) {
        (*head)->prev = particle;
    }
    *head = particle;
}

void RemoveParticle(Particle **head, Particle *particle) {
    if (particle->prev != NULL) {
        particle->prev->next = particle->next;
    }
    if (particle->next != NULL) {
        particle->next->prev = particle->prev;
    }
    if (*head == particle) {
        *head = particle->next;
    }
}

ParticleMoveEvent MoveParticle(Particle *particle) {
    particle->position = Vector2Add(particle->position, particle->velocity);

    if (
        particle->position.x < 0.0f ||
        particle->position.x > WINDOW_WIDTH ||
        particle->position.y < 0.0f ||
        particle->position.y > WINDOW_HEIGHT
    ) {
        return PARTICLE_MOVE_EVENT_DIE;
    }

    return PARTICLE_MOVE_EVENT_NONE;
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
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

    Particle *headParticle = NULL;

    double resumeTime = GetTime() + WAIT_DURATION;
    int playerScore = 0;
    int computerScore = 0;

    while (!WindowShouldClose()) {
        ControlPlayerBat(&playerBat);
        ControlComputerBat(&computerBat, &ball);

        MoveBat(&playerBat);
        MoveBat(&computerBat);

        if (GetTime() > resumeTime) {
            const BallMoveEvent ballMoveEvent = MoveBall(&ball, &playerBat, &computerBat);
            switch (ballMoveEvent) {
                case BALL_MOVE_EVENT_HIT:
                    for (int i = 0; i < PARTICLE_COUNT; i++) {
                        Particle *particle = CreateParticle(ball.position);
                        InsertParticle(&headParticle, particle);
                    }
                    break;
                case BALL_MOVE_EVENT_PLAYER_SCORE:
                    resumeTime = GetTime() + WAIT_DURATION;
                    playerScore++;
                    break;
                case BALL_MOVE_EVENT_COMPUTER_SCORE:
                    resumeTime = GetTime() + WAIT_DURATION;
                    computerScore++;
                    break;
                default:
                    break;
            }
        }

        for (Particle *particle = headParticle; particle != NULL; particle = particle->next ) {
            if (MoveParticle(particle)) {
                RemoveParticle(&headParticle, particle);
            }
        }

        BeginDrawing();
        {
            ClearBackground(PASTEL_NAVY);

            DrawLine(WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT, PASTEL_NAVY_LIGHT);

            const char *playerScoreText = TextFormat("%d", playerScore);
            const Vector2 playerScoreSize = MeasureTextEx(
                GetFontDefault(),
                playerScoreText,
                SCORE_FONT_SIZE,
                SCORE_FONT_SIZE / 10.0f
            );
            DrawTextEx(
                GetFontDefault(),
                playerScoreText,
                (Vector2) {
                    WINDOW_WIDTH * 0.25f - playerScoreSize.x / 2.0f,
                    WINDOW_HEIGHT / 2.0f - playerScoreSize.y / 2.0f
                },
                SCORE_FONT_SIZE,
                SCORE_FONT_SIZE / 10.f,
                PASTEL_NAVY_LIGHT
            );

            const char *computerScoreText = TextFormat("%d", computerScore);
            const Vector2 computerScoreSize = MeasureTextEx(
                GetFontDefault(),
                computerScoreText,
                SCORE_FONT_SIZE,
                SCORE_FONT_SIZE / 10.0f
            );
            DrawTextEx(
                GetFontDefault(),
                computerScoreText,
                (Vector2) {
                    WINDOW_WIDTH * 0.75f - computerScoreSize.x / 2.0f,
                    WINDOW_HEIGHT / 2.0f - computerScoreSize.y / 2.0f
                },
                SCORE_FONT_SIZE,
                SCORE_FONT_SIZE / 10.f,
                PASTEL_NAVY_LIGHT
            );

            for (Particle *particle = headParticle; particle != NULL; particle = particle->next ) {
                DrawCircleV(particle->position, PARTICLE_RADIUS, particle->color);
            }

            DrawRectangleRounded(
                (Rectangle){
                    .x = BAT_OFFSET,
                    .y = playerBat.position - BAT_ORIGIN.y,
                    .width = BAT_SIZE.x,
                    .height = BAT_SIZE.y
                },
                0.5f,
                8,
                PASTEL_PURPLE
            );

            DrawRectangleRounded(
                (Rectangle){
                    .x = WINDOW_WIDTH - BAT_OFFSET - BAT_SIZE.x,
                    .y = computerBat.position - BAT_ORIGIN.y,
                    .width = BAT_SIZE.x,
                    .height = BAT_SIZE.y
                },
                0.5f,
                8,
                PASTEL_PURPLE
            );

            rlPushMatrix();
            rlTranslatef(ball.position.x, ball.position.y, 0.0f);
            rlRotatef(ball.angle * RAD2DEG, 0.0f, 0.0f, 1.0f);
            DrawRectangleRounded(
                (Rectangle){
                    .x = -BALL_RADIUS,
                    .y = -BALL_RADIUS,
                    .width = BALL_RADIUS * 2.0f,
                    .height = BALL_RADIUS * 2.0f,
                },
                0.5f,
                8,
                PASTEL_PURPLE
            );
            rlPopMatrix();

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
