/*
 * Copyright (C) 2014, Jason S. McMullan
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef GCODE_H
#define GCODE_H

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX        256
#endif

#include <Stream.h>
#include <SD.h>

#include "Axis.h"
#include "ToolHead.h"
#include "StreamNull.h"
#include "Visualize.h"
#include "UserInterface.h"

#define DEBUG_ECHO      (1 << 0)
#define DEBUG_INFO      (1 << 1)
#define DEBUG_ERR       (1 << 2)

#define GCODE_LINE_MAX  255
#define GCODE_QUEUE_MAX 4

struct gcode_line {
    uint8_t len;
    char buff[GCODE_LINE_MAX];
};

struct gcode_io {
    struct gcode_line line;
    Stream *in, *out;
};

struct gcode_parameter {
    char label;
    float value;
};

#define GCODE_UPDATE_AXIS(x)    (1 << (x))
#define GCODE_UPDATE_F          (1 << (AXIS_MAX + 0))
#define GCODE_UPDATE_I          (1 << (AXIS_MAX + 1))
#define GCODE_UPDATE_J          (1 << (AXIS_MAX + 2))
#define GCODE_UPDATE_K          (1 << (AXIS_MAX + 3))
#define GCODE_UPDATE_P          (1 << (AXIS_MAX + 4))
#define GCODE_UPDATE_Q          (1 << (AXIS_MAX + 5))
#define GCODE_UPDATE_R          (1 << (AXIS_MAX + 6))
#define GCODE_UPDATE_S          (1 << (AXIS_MAX + 7))
#define GCODE_UPDATE_STRING     (1 << (AXIS_MAX + 8))

struct gcode_block {
    struct gcode_block *next;
    struct gcode_io *io;

    bool buffered;
    char code;
    int  cmd;
    int  num;
    uint16_t update_mask;
    float axis[AXIS_MAX];
    float f;        /* feed rate */
    float i;        /* x center of arc */
    float j;        /* y center of arc */
    float k;        /* z center of arc */
    float p;        /* parameter */
    float q;        /* parameter */
    float r;        /* parameter */
    float s;        /* parameter */
    char string[PATH_MAX];    /* For M20, M28, M29, M30, M32, M36, M117 */
};

class GCode {
    private:
        Visualize *_vis;
        Stream *_stream, *_debug;
        StreamNull _null;
        File _file;
        bool _file_enable;
        float _offset[AXIS_MAX];

        struct gcode_io _console, _program;
        struct {
            struct gcode_block ring[GCODE_QUEUE_MAX];
            struct gcode_block *free;
            struct gcode_block *active;
            struct gcode_block *pending, **pending_tail;
        } _block;
        enum { ABSOLUTE = 0, RELATIVE } _positioning;
        float _units_to_mm;
        float _feed_rate;
        enum { MODE_HALT = 0, MODE_PAUSE, MODE_RUN } _mode;
        UserInterface *_ui;
        CNC *_cnc;
    public:
        GCode(Stream *s, CNC *cnc,
                   UserInterface *ui = 0, Visualize *vis = 0)
        {
            _ui = ui;
            _vis = vis;
            _cnc = cnc;
            _positioning = ABSOLUTE;
            _units_to_mm = 1.0;
            _stream = s;
            _feed_rate = 1.0;
            _offset[AXIS_X] = 0;
            _offset[AXIS_Y] = 0;
            _offset[AXIS_Z] = 0;
            _offset[AXIS_E] = 0;

            _debug = &_null;

            for (int i = 0; i < GCODE_QUEUE_MAX - 1; i++) {
                _block.ring[i].next = &_block.ring[i+1];
            }

            _block.free = &_block.ring[0];
            _block.pending_tail = &_block.pending;

            _mode = MODE_RUN;
        }

        void begin()
        {
            _console.in = _stream;
            _console.out = _stream;

            _console.out->println("start");

            _file = SD.open("start.gco");
            _file_enable = _file;
            if (_file_enable)
                _ui->message_set(_file.name());

            _program.in = &_file;
            _program.out = &_null;
        }

        void update();

        void pause(bool check_switch = false)
        {
            if (check_switch && _ui)
                if (_ui->cnc_switch(UI_SWITCH_OPTIONAL_STOP) == 0)
                    return;

            _mode = MODE_PAUSE;
            if (_ui)
                _ui->on_pause();
        }

        void run()
        {
            if (_mode == MODE_PAUSE) {
                if (_ui)
                    _ui->on_run();
                _mode = MODE_RUN;
            }
        }

    private:
        void _block_do(struct gcode_block *blk);
        bool _line_parse(struct gcode_line *line, struct gcode_block *blk);
        void _process_io(struct gcode_io *io);
        void _process_block(struct gcode_block *blk);
};

#endif /* GCODE_H */
/* vim: set shiftwidth=4 expandtab:  */
