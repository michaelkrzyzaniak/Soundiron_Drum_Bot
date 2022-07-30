#ifndef ELECTROMAGNET
#define ELECTROMAGNET 1

/* only one motor driver (B) is used and the other is left unconnected */
void electromagnet_init(int vin_pin, int standby_pin, int in_1_pin, int in_2_pin, int pwm_pin);
void electromagnet_on (float strength); /*-1 to 1 where negative is reverse direction */
void electromagnet_off();


#endif //ELECTROMAGNET
