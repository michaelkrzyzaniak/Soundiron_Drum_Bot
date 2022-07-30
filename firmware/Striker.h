#ifndef STRIKER
#define STRIKER 1

/* returns 1 on success */
int  striker_init(int zap_id);
void striker_strike(float height /* 0 ~ 1*/);
void striker_strike_force_mode(float force);
void striker_kill();
int  striker_is_busy();

#endif //STRIKER
