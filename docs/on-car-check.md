# VCU on-car check

Run after any VCU change, before the car is driven.
Car on stands, wheels off the ground, area clear, HV-qualified person present, someone at the e-stop.
Watch `VCU_State` on CAN (CANoe / PCAN) to see the state number.

States: 0 Parked · 1 Closing AIRs · 2 Precharging · 3 Ready to drive · 4 Driving · 5/6 Stopping drive ·
7/8 TS fault · 10 Pedal sensor fault · 11 Brake + throttle fault

| # | Do | Expect | Pass? |
|---|----|--------|-------|
| 1 | LV on | All dash LEDs light ~2 s, heartbeat LED blinks, state 0 | |
| 2 | Mode knob to Endurance (1) | Current mode = 1 | |
| 3 | Press and release TS | State 1 → 2 → 3 within 3 s | |
| 4 | Press R2D **without** brake | Stays in 3, no siren | |
| 5 | Hold brake, press R2D | Siren 2 s, state 4, R2D LED off | |
| 6 | Light throttle, then release | Torque request rises, wheels turn slowly, back to 0 | |
| 7 | Brake hard + throttle above 30% | State 11, torque 0 | |
| 8 | Release throttle fully | Back to 4 after about 0.1 s | |
| 9 | Unplug one throttle sensor | State 10, torque 0 | |
| 10 | Plug it back in | Back to 4 | |
| 11 | Press R2D | State 5/6, then 3 after 0.5 s | |
| 12 | Press TS | AIRs open, state 0 | |
| 13 | TS on again (state 3), press e-stop | State 7, TS LED blinks fast | |
| 14 | Reset e-stop, press TS | State 0 | |

Date: ____ · Branch/commit: ____ · Checked by: ____
