
// midi code from https://ccrma.stanford.edu/~craig/articles/linuxmidi/alsa-1.0/nonblockinginput.c

#include <stdio.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>     /* Interface to the ALSA system */


// gcc -o midimotors midimotors.c -l wiringPi -l asound



// http://www.phy.mtu.edu/~suits/notefreqs.html
const float baseScale[] = {130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94};
const int IDLE_SLOW_DIVIDER = 60;


// try to play notes on 4 gpio pins simultaneously
const int N = 4;
int pins[] = {18, 27, 22, 23};

double actualSpeeds[] = {0,0,0,0};
int halfPeriods[] = {0,0,0,0};
unsigned int times[] = {0,0,0,0};
int states[] = {0,0,0,0};

int multiplier = 180 / 24; // 180 steps per revolution, 24 teeth per gear

// keep track of midi keys that are down
// monitor 37 notes starting at note 60
#define KEYLOW 60
#define KEYRANGE 37
int keys[KEYRANGE];
unsigned int scaleHalfPeriods[KEYRANGE];



int main(int argc, char **argv) {
  wiringPiSetupGpio();
  //piHiPri(99); // increase scheduling priority (made no difference, try nice instead?)


  int status, count;
  int mode = SND_RAWMIDI_NONBLOCK;
  snd_rawmidi_t* midiin = NULL;
  const char* portname = "hw:1,0,0";  // see alsarawportlist.c example program
  if ((argc > 1) && (strncmp("hw:", argv[1], 3) == 0)) {
    portname = argv[1];
  }
  if ((status = snd_rawmidi_open(&midiin, NULL, portname, mode)) < 0) {
    printf("Problem opening MIDI input: %s\n", snd_strerror(status));
    exit(1);
  }





  int i;
  
  for (i=0; i<KEYRANGE; i++) {
    keys[i] = 0;

    scaleHalfPeriods[i] = 1000000 / baseScale[i % 12] / 2; //32;
    // octave up
    scaleHalfPeriods[i] >>= (i / 12);
  }

  for (i=0; i<N; i++) {
    pinMode(pins[i], OUTPUT);
    halfPeriods[i] = 1000000 / IDLE_SLOW_DIVIDER; // start slow
    times[i] = micros() + halfPeriods[i];
    states[i] = 0;
    digitalWrite(pins[i], 0);
  }

  printf("hey\n");
  
  
  snd_midi_event_t *midiParser;
  snd_midi_event_new(10, &midiParser);
  snd_seq_event_t seqEvent;
  char buffer[1]; // raw midi buffer

  while (1) {
    unsigned int m = micros();

    for (i=0; i<N; i++) {
      if (m > times[i]) {
        times[i] += actualSpeeds[i];
        states[i] = 1-states[i];
        digitalWrite(pins[i], states[i]);
      }
      // note glide!
      actualSpeeds[i] = actualSpeeds[i]*0.9997 + halfPeriods[i] * 0.0003;
    }
    
  

    status = 0;
    while (status != -EAGAIN) {
      status = snd_rawmidi_read(midiin, buffer, 1);
      if ((status < 0) && (status != -EBUSY) && (status != -EAGAIN)) {
        printf("Problem reading MIDI input: %s\n",snd_strerror(status));
      } else if (status >= 0) {
        if (snd_midi_event_encode_byte(midiParser, buffer[0], &seqEvent) == 1) {
          if (seqEvent.type==SND_SEQ_EVENT_NOTEON) {
            //printf("NOTE %d %d\n", seqEvent.data.note.note, seqEvent.data.note.velocity);
            int noteNum = seqEvent.data.note.note;
            if (noteNum < KEYLOW || noteNum >= KEYLOW+KEYRANGE) break;
            if (seqEvent.data.note.velocity > 0) {
              keys[noteNum-KEYLOW] = 1;
            }
            else {
              keys[noteNum-KEYLOW] = 0;
            }
          }
          else if (seqEvent.type==SND_SEQ_EVENT_NOTEOFF) {
            int noteNum = seqEvent.data.note.note;
            if (noteNum < KEYLOW || noteNum >= KEYLOW+KEYRANGE) break;
            keys[noteNum-KEYLOW] = 0;
          }
          
          int n = 0;
          for (i=0; i<KEYRANGE; i++) {
            if (keys[i]) {
              printf("%d ", i);
              if (n < N) {
                halfPeriods[n] = scaleHalfPeriods[i];
                printf("(%d) ", scaleHalfPeriods[i]);
                n++;
              }
            }
          }
          for (;n < N; n++) {
            halfPeriods[n] = 1000000 / IDLE_SLOW_DIVIDER;
          }

          printf("\n");
        }
      }
    }
  }

  printf("whoa\n");

}
