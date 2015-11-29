# snowblossoms
instruments for Snow

## the flowers
Four spinning gear-toothed flowers play notes when the performer scrapes the gears with a card. A MIDI keyboard controls the notes that the flowers play by adjusting their speeds.

Raspberry Pi makes it pretty easy to read the keys pressed on a USB MIDI keyboard plugged into one of the Pi's USB ports. The code (midimotors.c) looks up the note numbers in a frequency table and adjusts the motor speeds accordingly. It sends pulses at the appropriate frequency out four of the GPIO pins, which can be hooked up to the step inputs of four stepper motor drivers.
