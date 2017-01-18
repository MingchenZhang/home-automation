// This program allows an Arduino to power on/off a Duraflame heater using infrared
//
// Usage: hook up an IR led with Annode on pin 13.  Then send serial data "POWER"
// to the arduino.  It should flash the LED code.
//
// By Rick Osgood with code borrowed from adafruit.com
//

#define IRledPin PB5
#define NumIRsignals 148

// This is the code I determined works for my Duraflame heater
int IRsignal[] = {
  // ON, OFF (in 10's of microseconds)
  1166, 500,
  58, 42,
  58, 190,
  60, 42,
  56, 190,
  58, 42,
  58, 190,
  58, 42,
  58, 190,
  56, 190,
  58, 44,
  56, 190,
  58, 190,
  58, 42,
  58, 44,
  56, 190,
  60, 42,
  56, 44,
  58, 44,
  56, 44,
  58, 44,
  56, 44,
  60, 42,
  56, 44,
  58, 44,
  56, 44,
  58, 44,
  58, 42,
  56, 190,
  58, 44,
  58, 188,
  60, 42,
  56, 190,
  58, 44,
  60, 40,
  58, 44,
  60, 40,
  58, 44,
  58, 42,
  58, 44,
  58, 42,
  60, 42,
  58, 42,
  60, 42,
  58, 42,
  60, 42,
  58, 42,
  60, 42,
  58, 42,
  60, 42,
  56, 44,
  60, 42,
  56, 190,
  58, 44,
  56, 44,
  58, 42,
  58, 190,
  60, 40,
  60, 42,
  58, 42,
  60, 42,
  56, 190,
  62, 40,
  60, 40,
  58, 44,
  60, 40,
  58, 44,
  56, 190,
  58, 188,
  62, 40,
  58, 190,
  58, 42,
  58, 188,
  62, 0
};

void setup(void) {
  digitalWrite(13, LOW);   //Make sure LED starts "off"
  Serial.begin(9600);            //Initialize Serial port
}

void loop() {
  delay(1000);
  for (int i = 0; i < NumIRsignals; i += 2) {       //Loop through all of the IR timings
    pulseIR(IRsignal[i] * 10);            //Flash IR LED at 38khz for the right amount of time
    delayMicroseconds((IRsignal[i + 1]+10) * 10); //Then turn it off for the right amount of time
  }
}

// This function allows us to PWM the IR LED at about 38khz for the sensor
// Borrowed from Adafruit!
void pulseIR(long microsecs) {
  // we'll count down from the number of microseconds we are told to wait

  cli();  // this turns off any background interrupts

  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    //digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
    PORTB |= _BV(5);
    delayMicroseconds(11);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    //digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
    PORTB &= ~_BV(5);
    delayMicroseconds(11);         // hang out for 10 microseconds, you can also change this to 9 if its not working

    // so 26 microseconds altogether
    microsecs -= 26;
  }

  sei();  // this turns them back on
}
