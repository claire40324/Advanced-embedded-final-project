# Advanced-embedded-final-project

This is a real-time embedded system game implemented on the TM4C123GH6PM microcontroller, designed to test a player’s reflexes through dynamic tile interaction. The project showcases key concepts in RTOS-based scheduling, multithreading, semaphore-based synchronization, and hardware-level control of LCD display, buttons, sound output, and ADC/DAC peripherals.

Features
1. RTOS-based multithreading: Each cube is an independent thread with custom behavior and termination condition.
2. Semaphore grid system: A 6×6 matrix uses semaphores to manage cube positions and collisions safely.
3. Context switching: Carefully managed to preserve shared resources during concurrent rendering and logic.
4. Sound & scoring: Real-time sound effects and scoring feedback triggered by successful hits.
5. Debounced user input: Button inputs use debouncing and control game state transitions.
6. ADC/DAC loopback: Used to verify signal integrity and study waveform fidelity and distortion under constrained sampling rates.

🎮 Game Flow
1. Player navigates a control area, attempting to “hit” moving cubes that appear randomly.
2. The game uses color-coded blocks and visual feedback via an LCD display.
3. Cubes move until either they’re hit or a timer expires.
4. Two game types: multiple-cube (chaos mode) and single-cube (focus mode).

Technologies
1. C language
2. TM4C123 + RTOS
3. LCD display + joystick/buttons
4. Timers, ADC/DAC, SysTick
5. Semaphore & thread control logic
