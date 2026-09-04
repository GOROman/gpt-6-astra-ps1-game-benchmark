#ifndef FACET_GAME_H
#define FACET_GAME_H
#include <stdint.h>
#define FPS 60
#define RING_EDGE 760
#define MAX_HP 100
#define INTRO_TICKS 60
#define RESULT_TICKS 90
#define ROUND_TICKS (30 * FPS)

enum Input { IN_LEFT=1, IN_RIGHT=2, IN_UP=4, IN_DOWN=8,
 IN_PUNCH=16, IN_KICK=32, IN_GUARD=64, IN_JUMP=128,
 IN_STEP_L=256, IN_STEP_R=512, IN_SHOULDER=1024, IN_BACKSTEP=2048 };
enum Action { IDLE, PUNCH, KICK, LOW_KICK, LAUNCH_KICK, THROW, SHOULDER, HURT, DOWN, JUMP, BACKSTEP };
enum Phase { INTRO, FIGHT, ROUND_OVER, MATCH_OVER };
enum Reason { NONE, KO, RING_OUT, TIME_UP, DRAW };
enum Event { EV_SWING=1, EV_HIT=2, EV_BLOCK=4, EV_THROW=8, EV_ROUND=16 };
enum Level { HIGH, MID, LOW, GRAB };
typedef struct { int startup, active, recovery, damage, reach, push, stun, level; } Move;
typedef struct {
 int x,z,y, vx,vz, hp, wins, character;
 int dx,dz; /* unit facing vector, 1024 == 1 */
 int action,tick,stun,landed,crouch,guard, walk,back_tap;
 uint16_t previous;
} Fighter;
typedef struct {
 Fighter f[2]; int phase,tick,remaining,round,winner,reason,events;
 int hitstop,effect_life[2],effect_x[2],effect_z[2],effect_y[2],effect_block[2];
 uint16_t freeze_input[2];
 uint32_t rng;
} Game;
extern const Move game_moves[7];
void game_init(Game *g,int character1,int character2,uint32_t seed);
void game_tick(Game *g,uint16_t p1,uint16_t p2);
uint16_t game_cpu(Game *g,int player,int difficulty);
int game_distance(const Fighter *a,const Fighter *b);
const char *game_character_name(int c);
#endif
