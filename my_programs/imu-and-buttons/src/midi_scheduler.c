#include "midi_scheduler.h"
#include "midi_sdi.h"

typedef enum { EV_ON, EV_OFF } EvType;

typedef struct {
  uint32_t t;
  EvType type;
  uint8_t ch, note, vel;
  uint8_t used;
} MidiEv;

#define QSIZE 32
static MidiEv q[QSIZE];

void midiSchedInit(void){
  for(int i=0;i<QSIZE;i++) q[i].used = 0;
}

static int allocSlot(void){
  for(int i=0;i<QSIZE;i++) if(!q[i].used) return i;
  return -1;
}

bool midiSchedNoteOn(uint32_t t_ms, uint8_t ch, uint8_t note, uint8_t vel){
  int k = allocSlot(); if(k < 0) return false;
  q[k] = (MidiEv){ .t=t_ms, .type=EV_ON, .ch=ch, .note=note, .vel=vel, .used=1 };
  return true;
}

bool midiSchedNoteOff(uint32_t t_ms, uint8_t ch, uint8_t note, uint8_t vel){
  int k = allocSlot(); if(k < 0) return false;
  q[k] = (MidiEv){ .t=t_ms, .type=EV_OFF, .ch=ch, .note=note, .vel=vel, .used=1 };
  return true;
}

void midiSchedProcess(uint32_t now_ms){
  for(int i=0;i<QSIZE;i++){
    if(!q[i].used) continue;

    if((int32_t)(now_ms - q[i].t) >= 0){
      if(q[i].type == EV_ON)  midiNoteOn(q[i].ch,  q[i].note, q[i].vel);
      else                   midiNoteOff(q[i].ch, q[i].note, q[i].vel);
      q[i].used = 0;
    }
  }
}
