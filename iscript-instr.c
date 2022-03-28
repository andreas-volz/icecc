/*
  IceCC. These are the definitions on how to dispatch on iscript instructions
  Copyright (C) 2000-2001 Jeffrey Pang <jp@magnus99.dhs.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

/* $Id: iscript-instr.c,v 1.8 2001/11/25 01:49:03 jp Exp $ */

#include "scdef.h"
#include "iscript-instr.h"

typedef int (getarg_fn)(register MFILE *, register uint16 *,
			register int, register uint16);

typedef int (writearg_fn)(register MFILE *, uint16,
			   register int, register uint16);

/* We probably want to be able to dynamically load these
   later, but hardcode them for now so we don't have to
   bother writing a handler for the format file. */

/* The naming convention for the getarg callback functions are the following:

   getar_fn:
     get_<arg0_size>_<arg1_size>_<arg2size>...
     get_none      (means there are no args)

   argN_size:
     byte          (means the arg is of size byte)
     uint16        (means the arg is of size uint16)
     uint32        (means the arg is of size uint32)
     v_<argN_size> (means the remaining args are of argN_size,
                    but there are a variable number of them,
		    depending on what arg0's value is)

   Here are a couple examples:

   get_uint16_byte_byte:
   arg0 is a unit16, arg1 is a byte, and arg2 is a byte.

   get_byte_v_uint16:
   arg0 is a byte and there are a variable number of remaining args,
   each of size uint16 (the actual number depends on the value of arg0).
   Similar convention for write_fn's. */

static getarg_fn get_none;
static getarg_fn get_byte;
static getarg_fn get_uint16;
static getarg_fn get_uint16_uint16;
static getarg_fn get_uint16_byte;
static getarg_fn get_byte_byte;
static getarg_fn get_uint16_byte_byte;
static getarg_fn get_byte_v_uint16;
static getarg_fn get_byte_uint16;
static getarg_fn get_uint16_uint16_uint16;

static writearg_fn write_none;
static writearg_fn write_byte;
static writearg_fn write_uint16;
static writearg_fn write_uint16_uint16;
static writearg_fn write_uint16_byte;
static writearg_fn write_byte_byte;
static writearg_fn write_uint16_byte_byte;
static writearg_fn write_byte_v_uint16;
static writearg_fn write_byte_uint16;
static writearg_fn write_uint16_uint16_uint16;

/* export this array */
#ifndef CLASSIC_ICECC
IsInstrFmt __iscript_instructions[] = {
  {"playfram",         INSTR_NORMAL,        get_uint16, write_uint16},
  {"playframtile",     INSTR_NORMAL,        get_uint16, write_uint16},
  {"sethorpos",        INSTR_NORMAL,        get_byte, write_byte},
  {"setvertpos",       INSTR_NORMAL,        get_byte, write_byte},
  {"setpos",           INSTR_NORMAL,        get_byte_byte, write_byte_byte},
  {"wait",             INSTR_NORMAL,        get_byte, write_byte},
  {"waitrand",         INSTR_NORMAL,        get_byte_byte, write_byte_byte},
  {"goto",             INSTR_JMP,           get_uint16, write_uint16},
  {"imgol",            INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"imgul",            INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"imgolorig",        INSTR_NORMAL,        get_uint16, write_uint16},
  {"switchul",         INSTR_NORMAL,        get_uint16, write_uint16},
  {"__0c",             INSTR_NORMAL,        get_none, write_none},
  {"imgoluselo",       INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"imguluselo",       INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprol",            INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"highsprol",        INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"lowsprul",         INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"uflunstable",      INSTR_NORMAL,        get_uint16, write_uint16},
  {"spruluselo",       INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprul",            INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sproluselo",       INSTR_NORMAL,        get_uint16_byte, write_uint16_byte},
  {"end",              INSTR_TERM,          get_none, write_none},
  {"setflipstate",     INSTR_NORMAL,        get_byte, write_byte},
  {"playsnd",          INSTR_NORMAL,        get_uint16, write_uint16},
  {"playsndrand",      INSTR_NORMAL,        get_byte_v_uint16, write_byte_v_uint16},
  {"playsndbtwn",      INSTR_NORMAL,        get_uint16_uint16, write_uint16_uint16},
  {"domissiledmg",     INSTR_NORMAL,        get_none, write_none},
  {"attackmelee",      INSTR_NORMAL,        get_byte_v_uint16, write_byte_v_uint16},
  {"followmaingraphic",INSTR_NORMAL,        get_none, write_none},
  {"randcondjmp",      INSTR_COND_JMP,      get_byte_uint16, write_byte_uint16},
  {"turnccwise",       INSTR_NORMAL,        get_byte, write_byte},
  {"turncwise",        INSTR_NORMAL,        get_byte, write_byte},
  {"turn1cwise",       INSTR_NORMAL,        get_none, write_none},
  {"turnrand",         INSTR_NORMAL,        get_byte, write_byte},
  {"setspawnframe",    INSTR_NORMAL,        get_byte, write_byte},
  {"sigorder",         INSTR_NORMAL,        get_byte, write_byte},
  {"attackwith",       INSTR_NORMAL,        get_byte, write_byte},
  {"attack",           INSTR_NORMAL,        get_none, write_none},
  {"castspell",        INSTR_NORMAL,        get_none, write_none},
  {"useweapon",        INSTR_NORMAL,        get_byte, write_byte},
  {"move",             INSTR_NORMAL,        get_byte, write_byte},
  {"gotorepeatattk",   INSTR_NORMAL,        get_none, write_none},
  {"engframe",         INSTR_NORMAL,        get_byte, write_byte},
  {"engset",           INSTR_NORMAL,        get_byte, write_byte},
  {"__2d",             INSTR_NORMAL,        get_none, write_none},
  {"nobrkcodestart",   INSTR_NORMAL,        get_none, write_none},
  {"nobrkcodeend",     INSTR_NORMAL,        get_none, write_none},
  {"ignorerest",       INSTR_NORMAL,        get_none, write_none},
  {"attkshiftproj",    INSTR_NORMAL,        get_byte, write_byte},
  {"tmprmgraphicstart",INSTR_NORMAL,        get_none, write_none},
  {"tmprmgraphicend",  INSTR_NORMAL,        get_none, write_none},
  {"setfldirect",      INSTR_NORMAL,        get_byte, write_byte},
  {"call",             INSTR_COND_JMP,      get_uint16, write_uint16},
  {"return",           INSTR_TERM,          get_none, write_none},
  {"setflspeed",       INSTR_NORMAL,        get_uint16, write_uint16},
  {"creategasoverlays",INSTR_NORMAL,        get_byte, write_byte},
  {"pwrupcondjmp",     INSTR_COND_JMP,      get_uint16, write_uint16},
  {"trgtrangecondjmp", INSTR_COND_JMP,      get_uint16_uint16, write_uint16_uint16},
  {"trgtarccondjmp",   INSTR_COND_JMP,      get_uint16_uint16_uint16, write_uint16_uint16_uint16},
  {"curdirectcondjmp", INSTR_COND_JMP,      get_uint16_uint16_uint16, write_uint16_uint16_uint16},
  {"imgulnextid",      INSTR_NORMAL,        get_byte_byte, write_byte_byte},
  {"__3e",             INSTR_NORMAL,        get_none, write_none},
  {"liftoffcondjmp",   INSTR_COND_JMP,      get_uint16, write_uint16},
  {"warpoverlay",      INSTR_NORMAL,        get_uint16, write_uint16},
  {"orderdone",        INSTR_NORMAL,        get_byte, write_byte},
  {"grdsprol",         INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"__43",             INSTR_NORMAL,        get_none, write_none},
  {"dogrddamage",      INSTR_NORMAL,        get_none, write_none},
};
#else
IsInstrFmt __iscript_instructions[] = {
  {"playfram",         INSTR_NORMAL,        get_uint16, write_uint16},
  {"playframfile",     INSTR_NORMAL,        get_uint16, write_uint16},
  {"__02",             INSTR_NORMAL,        get_byte, write_byte},
  {"shvertpos",        INSTR_NORMAL,        get_byte, write_byte},
  {"__04",             INSTR_NORMAL,        get_byte_byte, write_byte_byte},
  {"wait",             INSTR_NORMAL,        get_byte, write_byte},
  {"waitrand",         INSTR_NORMAL,        get_byte_byte, write_byte_byte},
  {"goto",             INSTR_JMP,           get_uint16, write_uint16},
  {"imgol08",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"imgul09",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"imgol0a",          INSTR_NORMAL,        get_uint16, write_uint16},
  {"switchul",         INSTR_NORMAL,        get_uint16, write_uint16},
  {"__0c",             INSTR_NORMAL,        get_none, write_none},
  {"imgol0d",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"imgol0e",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprol0f",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprol10",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprul11",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"__12",             INSTR_NORMAL,        get_uint16, write_uint16},
  {"sprol13",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprol14",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"sprol15",          INSTR_NORMAL,        get_uint16_byte, write_uint16_byte},
  {"end",              INSTR_TERM,          get_none, write_none},
  {"__17",             INSTR_NORMAL,        get_byte, write_byte},
  {"playsnd",          INSTR_NORMAL,        get_uint16, write_uint16},
  {"playsndrand",      INSTR_NORMAL,        get_byte_v_uint16, write_byte_v_uint16},
  {"playsndbtwn",      INSTR_NORMAL,        get_uint16_uint16, write_uint16_uint16},
  {"domissiledmg",     INSTR_NORMAL,        get_none, write_none},
  {"attack1c",         INSTR_NORMAL,        get_byte_v_uint16, write_byte_v_uint16},
  {"followmaingraphic",INSTR_NORMAL,        get_none, write_none},
  {"__1e_condjmp",     INSTR_COND_JMP,      get_byte_uint16, write_byte_uint16},
  {"turnccwise",       INSTR_NORMAL,        get_byte, write_byte},
  {"turncwise",        INSTR_NORMAL,        get_byte, write_byte},
  {"turn1cwise",       INSTR_NORMAL,        get_none, write_none},
  {"turnrand",         INSTR_NORMAL,        get_byte, write_byte},
  {"__23",             INSTR_NORMAL,        get_byte, write_byte},
  {"sigorder",         INSTR_NORMAL,        get_byte, write_byte},
  {"attack25",         INSTR_NORMAL,        get_byte, write_byte},
  {"attack26",         INSTR_NORMAL,        get_none, write_none},
  {"castspell",        INSTR_NORMAL,        get_none, write_none},
  {"useweapon",        INSTR_NORMAL,        get_byte, write_byte},
  {"move",             INSTR_NORMAL,        get_byte, write_byte},
  {"gotorepeatattk",   INSTR_NORMAL,        get_none, write_none},
  {"__2b",             INSTR_NORMAL,        get_byte, write_byte},
  {"__2c",             INSTR_NORMAL,        get_byte, write_byte},
  {"__2d",             INSTR_NORMAL,        get_none, write_none},
  {"nobrkcodestart",   INSTR_NORMAL,        get_none, write_none},
  {"nobrkcodeend",     INSTR_NORMAL,        get_none, write_none},
  {"ignorerest",       INSTR_NORMAL,        get_none, write_none},
  {"attkprojangle",    INSTR_NORMAL,        get_byte, write_byte},
  {"tmprmgraphicstart",INSTR_NORMAL,        get_none, write_none},
  {"tmprmgraphicend",  INSTR_NORMAL,        get_none, write_none},
  {"playframno",       INSTR_NORMAL,        get_byte, write_byte},
  {"__35_condjmp",     INSTR_COND_JMP,      get_uint16, write_uint16},
  {"__36",             INSTR_TERM,          get_none, write_none},
  {"__37",             INSTR_NORMAL,        get_uint16, write_uint16},
  {"__38",             INSTR_NORMAL,        get_byte, write_byte},
  {"pwrupcondjmp",     INSTR_COND_JMP,      get_uint16, write_uint16},
  {"trgtrangecondjmp", INSTR_COND_JMP,      get_uint16_uint16, write_uint16_uint16},
  {"trgtarccondjmp",   INSTR_COND_JMP,      get_uint16_uint16_uint16, write_uint16_uint16_uint16},
  {"__3c_condjmp",     INSTR_COND_JMP,      get_uint16_uint16_uint16, write_uint16_uint16_uint16},
  {"__3d",             INSTR_NORMAL,        get_byte_byte, write_byte_byte},
  {"__3e",             INSTR_NORMAL,        get_none, write_none},
  {"__3f_condjmp",     INSTR_COND_JMP,      get_uint16, write_uint16},
  {"__40",             INSTR_NORMAL,        get_uint16, write_uint16},
  {"__41",             INSTR_NORMAL,        get_byte, write_byte},
  {"sprol42",          INSTR_NORMAL,        get_uint16_byte_byte, write_uint16_byte_byte},
  {"__43",             INSTR_NORMAL,        get_none, write_none},
  {"__44",             INSTR_NORMAL,        get_none, write_none},
};
#endif

/* *** Reading argument functions *** */

/* need for speed :) macros for fun */
#define READ_BYTE { \
  byte   bytebuf; \
  if (mread((void *)&bytebuf, sizeof(byte), 1, iscript) != 1) { \
    sc_err_log("error reading opcode arg with size %d at %04x", \
	    sizeof(byte), (unsigned)mtell(iscript)); \
    return INSTR_READ_ERROR; \
  } \
  *buf = bytebuf; \
}

#define READ_UINT16 { \
  if (mread((void *)buf, sizeof(uint16), 1, iscript) != 1) { \
    sc_err_log("error reading opcode arg with size %d at %04x", \
	    sizeof(uint16), (unsigned)mtell(iscript)); \
    return INSTR_READ_ERROR; \
  } \
}

static int get_none(register MFILE *iscript, register uint16 *buf,
		    register int argno, register uint16 first_arg)
{
  return INSTR_READ_DONE;
}

static int get_byte(register MFILE *iscript, register uint16 *buf,
		    register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    READ_BYTE;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_uint16(register MFILE *iscript, register uint16 *buf,
		      register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    READ_UINT16;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_uint16_uint16(register MFILE * iscript, register uint16 *buf,
			     register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0: case 1:
    READ_UINT16;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_uint16_uint16_uint16(register MFILE * iscript, register uint16 *buf,
				    register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0: case 1: case 2:
    READ_UINT16;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_uint16_byte(register MFILE * iscript, register uint16 *buf,
			   register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    READ_UINT16;
    return 0;
  case 1:
    READ_BYTE;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_byte_byte(register MFILE *iscript, register uint16 *buf,
			 register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0: case 1:
    READ_BYTE;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_uint16_byte_byte(register MFILE * iscript, register uint16 *buf,
				register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    READ_UINT16;
    return 0;
  case 1: case 2:
    READ_BYTE;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_byte_uint16(register MFILE * iscript, register uint16 *buf,
			   register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    READ_BYTE;
    return 0;
  case 1:
    READ_UINT16;
    return 0;
  default:
    return INSTR_READ_DONE;
  }
}

static int get_byte_v_uint16(register MFILE * iscript, register uint16 *buf,
			     register int argno, register uint16 first_arg)
{
  if (argno==0) {
    READ_BYTE;
    return 0;
  } else if (argno <= first_arg) {
    READ_UINT16;
    return 0;
  } else
    return INSTR_READ_DONE;
}

/* *** Writing argument functions *** */

#define WRITE_BYTE { \
  byte   bytebuf; \
  bytebuf = arg; \
  if (mwrite((void *)&bytebuf, sizeof(byte), 1, iscript) != 1) { \
    sc_err_log("error writing opcode arg with size %d at %04x", \
	    sizeof(byte), (unsigned)mtell(iscript)); \
    return INSTR_WRITE_ERROR; \
  } \
}

#define WRITE_UINT16 { \
  if (mwrite((void *)&arg, sizeof(uint16), 1, iscript) != 1) { \
    sc_err_log("error writing opcode arg with size %d at %04x", \
	    sizeof(uint16), (unsigned)mtell(iscript)); \
    return INSTR_WRITE_ERROR; \
  } \
}

static int write_none(register MFILE *iscript, uint16 arg,
		      register int argno, register uint16 first_arg) 
{
  return INSTR_WRITE_DONE;
}

static int write_byte(register MFILE *iscript, uint16 arg,
		      register int argno, register uint16 first_arg) 
{
  switch (argno) {
  case 0:
    WRITE_BYTE;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_uint16(register MFILE *iscript, uint16 arg,
			register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    WRITE_UINT16;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_uint16_uint16(register MFILE * iscript, uint16 arg,
			       register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0: case 1:
    WRITE_UINT16;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_uint16_uint16_uint16(register MFILE * iscript, uint16 arg,
				      register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0: case 1: case 2:
    WRITE_UINT16;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_uint16_byte(register MFILE * iscript, uint16 arg,
			     register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    WRITE_UINT16;
    return 0;
  case 1:
    WRITE_BYTE;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_byte_byte(register MFILE *iscript, uint16 arg,
			   register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0: case 1:
    WRITE_BYTE;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_uint16_byte_byte(register MFILE * iscript, uint16 arg,
				register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    WRITE_UINT16;
    return 0;
  case 1: case 2:
    WRITE_BYTE;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_byte_uint16(register MFILE * iscript, uint16 arg,
			     register int argno, register uint16 first_arg)
{
  switch (argno) {
  case 0:
    WRITE_BYTE;
    return 0;
  case 1:
    WRITE_UINT16;
    return 0;
  default:
    return INSTR_WRITE_DONE;
  }
}

static int write_byte_v_uint16(register MFILE * iscript, uint16 arg,
			     register int argno, register uint16 first_arg)
{
  if (argno==0) {
    WRITE_BYTE;
    return 0;
  } else if (argno <= first_arg) {
    WRITE_UINT16;
    return 0;
  } else
    return INSTR_WRITE_DONE;
}
