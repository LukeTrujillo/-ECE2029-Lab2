/************** ECE2049 DEMO CODE ******************/
/**************  9 January 2019   ******************/
/***************************************************/

#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"

// Function Prototypes
void swDelay(char numLoops);
void configureButtons();
unsigned int readButtons();

void showLED(unsigned int);

void configUserLED();
void writeLED(unsigned int);
void buzz(unsigned int);

void makeSong();
void welcomeScreen();
void win();
void lose();

// Declare globals here
unsigned int led = 0x00;

struct Note {
  int frequency, duration;
};

void makeNote(int position, int freq, int dur);
void playNote(struct Note not);
unsigned int getNoteNum(struct Note note);

struct Note song[33];

volatile int nextBeat = 0;
int songScore = 100;

// Main
void main() {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer. Always need to stop this!!
  _BIS_SR(GIE);

  configUserLED();
  configureButtons();
  makeSong();

  //---------------TIMER SETUP----------------------
  TA2CTL = TASSEL_1 | MC_1 | ID_0;  // clock: ACLK:32768 Hz, upmode, divider=1
  TA2CCR0 = 164;  // interrupt every 164 clicks (~0.005 seconds)

  TA2CCTL0 = CCIE;  // enable interrupt
  //-----------------------------------------------

  welcomeScreen();

  int index = 0;        // current note
  int songLength = 33;  // total note length in song
  int buttonPressed = 0;
  int timePassed = 0;
  int currentNoteActualDuration = song[index].duration;
  int noteScore = 0;
  nextBeat = 0;  // reset at start of actual game
  songScore = 100;
  while (index < songLength) {
    if (nextBeat) {  // this should run about once every 0.005 seconds
      timePassed = nextBeat;
      nextBeat = 0;  // reset timer flag

      if (song[index].duration > 0) {
        // if this note still has playing time left
        // decrease the amount of time left in this note
        song[index].duration -= timePassed;

        // check the buttons and if any are pressed, turn off led
        if (readButtons() == getNoteNum(song[index]) && buttonPressed == 0) {
          buttonPressed = 1;
          noteScore = 100- ((currentNoteActualDuration - song[index].duration) * 100 /
                      currentNoteActualDuration);
          songScore = (10 * (noteScore) + 90 * songScore) /
                      100;  // alpha smoothing = 0.1
        }
      } else {
        // end of note
        if (buttonPressed == 0)
          songScore =
              (90 * songScore) / 100;  // missed note, songScore drops 10%

        index++;  // next note
        // songScore /= index;
        buttonPressed = 0;
        currentNoteActualDuration = song[index].duration;
        if (songScore < 50) index = 1000;  // if 3 seconds off, lose
        char str[10];
        sprintf(str, "%d", songScore);
        Graphics_clearDisplay(&g_sContext);  // Clear the display
        Graphics_drawStringCentered(&g_sContext, str, AUTO_STRING_LENGTH, 48,
                                    15, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
      }
      if (buttonPressed) {
        writeLED(0);
      } else {
        writeLED(getNoteNum(song[index]));
      }
    }
    playNote(song[index]);  // actually play the needed sound
  }
  BuzzerOff();
  if (index == songLength) {
    win();
  } else {
    lose();
  }
  return main();
}
void playNote(struct Note note) { BuzzerOn(note.frequency, 100); }

unsigned int getNoteNum(struct Note note) {
  unsigned int num = 0;
  if (note.frequency <= 588) {
    num = 1;
  } else if (note.frequency <= 700) {
    num = 2;
  } else if (note.frequency <= 1000) {
    num = 4;
  } else {
    num = 8;
  }
  return num;
}

void makeSong() {
  makeNote(0, 659, 50);   // E
  makeNote(1, 587, 50);   // D
  makeNote(2, 523, 100);  // C
  makeNote(3, 659, 50);   // E
  makeNote(4, 587, 50);   // D
  makeNote(5, 523, 100);  // C

  makeNote(6, 784, 50);    // G
  makeNote(7, 698, 50);    // F
  makeNote(8, 659, 100);   // E
  makeNote(9, 784, 50);    // G
  makeNote(10, 698, 50);   // F
  makeNote(11, 659, 100);  // E

  makeNote(12, 1046, 50 + 17);
  makeNote(13, 880, 17);
  makeNote(14, 988, 17);
  makeNote(15, 1046, 17 * 2);
  makeNote(16, 784, 17);
  makeNote(17, 784, 50);

  makeNote(18, 1046, 50 + 17);
  makeNote(19, 880, 17);
  makeNote(20, 988, 17);
  makeNote(21, 1046, 17 * 2);
  makeNote(22, 784, 17);
  makeNote(23, 784, 50);

  makeNote(24, 1046, 50 + 17);
  makeNote(25, 880, 17);
  makeNote(26, 988, 17);
  makeNote(27, 1046, 17 * 2);
  makeNote(28, 784, 17);
  makeNote(29, 784, 50);

  makeNote(30, 659, 50);   // E
  makeNote(31, 587, 50);   // D
  makeNote(32, 523, 100);  // C
}

void makeNote(int position, int freq, int dur) {  // shorthand for note creation
  song[position].duration = dur * 4;
  song[position].frequency = freq;
};

// Example syntax for TimerA2 ISR
#pragma vector = TIMER2_A0_VECTOR
__interrupt void TIMERA2_ISR(void) { nextBeat++; }

void showLEDs(unsigned int val) {}
void configureButtons() {
  // buttons on P7.0, P7.4, P2.4, P3.6

  // configure switches for IO
  P7SEL &= ~(BIT0 | BIT4);
  P2SEL &= ~(BIT2);
  P3SEL &= ~(BIT6);

  // set them up for input
  P7DIR &= ~(BIT0 | BIT4);
  P2DIR &= ~(BIT2);
  P3DIR &= ~(BIT6);

  // enable the pull resistors
  P7REN |= (BIT0 | BIT4);
  P2REN |= (BIT2);
  P3REN |= (BIT6);

  // specify that they should be pull resistors
  P7OUT |= (BIT0 | BIT4);
  P2OUT |= (BIT2);
  P3OUT |= (BIT6);
}

unsigned int readButtons() {
  // buttons on P7.0(S1), P7.4 (S2), P2.4 (S3), P3.7 (S4)
  return (~P7IN & (BIT0)) | ((~P3IN & (BIT6)) >> 5) | (~P2IN & (BIT2)) |
         ((~P7IN & (BIT4)) >> 1);
}
void configUserLED() {
  P6SEL &= ~(BIT2 | BIT1 | BIT3 | BIT4);
  P6DIR |= (BIT2 | BIT1 | BIT3 | BIT4);
}
void writeLED(unsigned int val) {
  P6OUT &= ~(BIT2 | BIT1 | BIT3 | BIT4);
  // set all to off
  // if passed 0, will turn everything off
  if ((val & BIT0) != 0) {
    P6OUT |= BIT2;
  }
  if ((val & BIT1) != 0) {
    P6OUT |= BIT1;
  }
  if ((val & BIT2) != 0) {
    P6OUT |= BIT3;
  }
  if ((val & BIT3) != 0) {
    P6OUT |= BIT4;
  }
}

void welcomeScreen() {
  configDisplay();
  configKeypad();
  Graphics_clearDisplay(&g_sContext);  // Clear the display
  Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 48,
                              15, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, "to", AUTO_STRING_LENGTH, 48, 25,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, "MSP430 Hero", AUTO_STRING_LENGTH,
                              48, 35, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, "(* to start)", AUTO_STRING_LENGTH,
                              48, 45, TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);

  while (getKey() != '*') BuzzerOff();  // essentially do nothing

  Graphics_clearDisplay(&g_sContext);  // Clear the display
  nextBeat = 0;
  while (nextBeat < 200) {
    Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48, 15,
                                TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    writeLED((unsigned int)(rand() % 7) +
             1);  // chooses some random leds to light
  }

  Graphics_clearDisplay(&g_sContext);
  while (nextBeat < 400) {
    Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 15,
                                TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    writeLED((unsigned int)(rand() % 3) + 1);
  }

  Graphics_clearDisplay(&g_sContext);
  while (nextBeat < 600) {
    Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 15,
                                TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    writeLED((unsigned int)(rand() % 2));
  }

  Graphics_clearDisplay(&g_sContext);  // Clear the display
}

void win() {
  Graphics_clearDisplay(&g_sContext);  // Clear the display
  nextBeat = 0;
  char str[10];
  sprintf(str, "%d", songScore);
  Graphics_drawStringCentered(&g_sContext, "YOU WIN", AUTO_STRING_LENGTH, 48,
                              15, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, "score:", AUTO_STRING_LENGTH, 48,
                              25, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, str, AUTO_STRING_LENGTH, 48, 35,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
  while (getKey() != '#') {
    writeLED((unsigned int)(rand() % 15));  // glittery lights
  }
  writeLED(0);
}

void lose() {
  Graphics_clearDisplay(&g_sContext);  // Clear the display
  nextBeat = 0;
  char str[10];
  sprintf(str, "%d", songScore);
  Graphics_drawStringCentered(&g_sContext, "YOU LOSE", AUTO_STRING_LENGTH, 48,
                              15, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, "score:", AUTO_STRING_LENGTH, 48,
                              25, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, str, AUTO_STRING_LENGTH, 48, 35,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
  BuzzerOn(1000, 100);
  while (getKey() != '#') {
    // no fun lights for you
  }
  BuzzerOff();
  writeLED(0);
}

void buzzerDuration(int freq, int duration) {
  BuzzerOn(freq, 100);
  nextBeat = 0;
  while (nextBeat < duration) {  // wait
    // do nothing
  }
  BuzzerOff();
}
