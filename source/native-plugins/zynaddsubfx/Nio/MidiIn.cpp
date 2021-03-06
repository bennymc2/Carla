/*
  ZynAddSubFX - a software synthesizer

  MidiIn.C - This class is inherited by all the Midi input classes
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "MidiIn.h"
#include "../globals.h"
#include "InMgr.h"

namespace zyncarla {

void MidiIn::midiProcess(unsigned char head,
                         unsigned char num,
                         unsigned char value)
{
    MidiEvent     ev;
    unsigned char chan = head & 0x0f;
    switch(head & 0xf0) {
        case 0x80: //Note Off
            ev.type    = M_NOTE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = 0;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0x90: //Note On
            ev.type    = M_NOTE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xA0: /* pressure, aftertouch */
            ev.type    = M_PRESSURE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xb0: //Controller
            ev.type    = M_CONTROLLER;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = value;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xc0: //Program Change
            ev.type    = M_PGMCHANGE;
            ev.channel = chan;
            ev.num     = num;
            ev.value   = 0;
            InMgr::getInstance().putEvent(ev);
            break;
        case 0xe0: //Pitch Wheel
            ev.type    = M_CONTROLLER;
            ev.channel = chan;
            ev.num     = C_pitchwheel;
            ev.value   = (num + value * (int) 128) - 8192;
            InMgr::getInstance().putEvent(ev);
            break;
    }
}

}
