#ifndef RANDOMSOUND_ASOUNDRUNNER_H
#define RANDOMSOUND_ASOUNDRUNNER_H

#include "bitbuffer.h"

extern int arecord_read_fd;
extern int restart_arecord;

int start_arecord(BitBuffer read_into);

void asound_do_read(void);

#endif
