The test kernel module registers ISR for GPIO2_8 pin (S2 button).
When user presses S2 button the module prints "*" character to 
dmesg queue and toggles on/off USR3 LED.
