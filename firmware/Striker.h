#ifndef STRIKER
#define STRIKER 1

/* returns 1 on success */
int   striker_init(int zap_id);
void  striker_strike(float height /* 0 ~ 1*/);
void  striker_strike_force_mode(float force /* 0 ~ 1*/);
void  striker_lower_and_wait_with_magnet_off_5_seconds();
void  striker_set_should_stream_motor_force(int should_stream);
void  striker_set_calibrated_bot_pos(float pos /* 0 ~ 1*/);
float striker_get_calibrated_bot_pos();
void  striker_set_calibrated_top_pos(float pos /* 0 ~ 1*/);
float striker_get_calibrated_top_pos();
void  striker_kill(int with_error /*0 for no error, 1 for error*/);
int   striker_is_busy();

#endif //STRIKER
