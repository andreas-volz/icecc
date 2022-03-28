/* iscript-instr.h */

#ifndef _INSTR_H_
#define _INSTR_H_

#define MAX_OPCODE      ((byte)0x44)

#define INSTR_READ_ERROR  -1
#define INSTR_WRITE_ERROR -1
#define INSTR_READ_DONE    1
#define INSTR_WRITE_DONE   1

#define INSTR_NORMAL    0
#define INSTR_TERM      1
#define INSTR_JMP       2
#define INSTR_COND_JMP  3

/* these will be indexed in an array by opcode */
typedef struct IsInstrFmt {
  char *name; /* cannonical name of this instruction */
  int  type;  /* INSTR_TERM     if a terminating instruction,
		 INSTR_JMP      if a unconditional jump,
		 INSTR_COND_JMP if a conditional jump,
		 otherwise      a 'regular' instruction */
  /* this callback function will determine what the next argument is,
     depending on the arg number, the current iscript file position,
     and the first arg value. If the function returns 0 then the arg 
     should be in buf, if the return is INSTR_READ_DONE all the arguments
     were read and the next instruction can start reading. You should check
     the instr.type to see if it is terminal, condjump, or jump. */
  int (*get_next_arg)(register MFILE *iscript,
		      register uint16 *buf,
		      register int argno,
		      register uint16 first_arg);
  /* this callback function writes the next argument back to a file */
  int (*write_next_arg)(register MFILE *iscript,
			uint16 arg,
			register int argno,
			register uint16 first_arg);
} IsInstrFmt;

/* array of instructions defined in instr.c */
extern IsInstrFmt __iscript_instructions[];

/* get the next argument for this instruction opcode from the MFILE *iscript.
   Assues the file position has been initialized to where to start reading.
   Returns it in buf unless INSTR_READ_DONE is returned, meaning there are
   no more arguments and nothing was read. This is a safe macro for anything. */
#define isinstr_get_next_arg(iscript, opcode, buf, argno, first_arg) \
        (__iscript_instructions[(opcode)].get_next_arg((iscript), (buf), (argno), (first_arg)))

/* returns the iscript opcode type: either INSTR_NORMAL, INSTR_JMP, INSTR_CONDJMP,
   or INSTR_TERM. Safe macro. */
#define isinstr_get_type(opcode) \
        (__iscript_instructions[(opcode)].type)

/* returns a char * to the iscript instruction's name. Safe macro. */
#define isinstr_get_name(opcode) \
        (__iscript_instructions[(opcode)].name)

/* writes the next argument similar as isinstr_get_next_arg gets the next arg. 
   iscript is the mem-mapped file to write to (assumed to be in the corect
   position to write), opcode is the instruction opcode, arg is the arg to
   write (actual value, not a buffer pointer), argno is the argument number,
   and first arg is the first argument. Returns INSTR_WRITE_DONE when there
   are no more arguments to write (you shouldn't have to get there, since you
   should know how many args your bytecode has, but for error checking anyway) */
#define isinstr_write_next_arg(iscript, opcode, arg, argno, first_arg) \
        (__iscript_instructions[(opcode)].write_next_arg((iscript), (arg), (argno), (first_arg)))
#endif
