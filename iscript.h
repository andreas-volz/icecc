/* iscript.h */

#ifndef _ISCRIPT_H_
#define _ISCRIPT_H_

#include "scdef.h"

/* The maximum size of an iscript file, 2^16-1 is generally the max */
#define MAX_ISCRIPT_FILE_SIZE 0xFFFF

/* these definitions refer to the last argument that the
   symtbl functions take in. They don't really effect anything
   except for the name of the label the functions give to the
   returning symtbl entry */
#define HEADER            1
#define LOCALJMP          2
#define LONGJMP           3

/* max number of args an instruction is allowed to have */
#define MAX_INSTR_ARGS  64
/* max number of anim entries allowed per header */
#define MAX_ANIM_ENTRIES  28 

/* control hashtable sizes in iscript */
#define SYMTBL_HASH_SIZE  1024 
#define HEADERS_HASH_SIZE 512

struct ObjList;
struct IsAnim;
struct IsSymTblEnt;
struct IsHeader;
struct IsInstr;

/* holds information about an animation entry */
typedef struct IsAnim {
  struct IsHeader *header; /* the header which this animation is from */
  uint16             animno;  /* the number of the IsAnim in the header that
			         is related to us (the header that brought us
			         to this label */
  int                type;    /* the type of 'jump' that brought us here for
				 this header, either HEADER, LOCALJMP, or LONGJMP */
} IsAnim;

/* Holds the value in each offset entry in the sym_tbl.
   Index this in a hashtable with the offset as the key.
   Add more info about each offset as apporpriate */
typedef struct IsSymTblEnt {
  long   id;             /* unique identifier */
  struct IsInstr *bc;  /* the byte-code this symbol points to */
  struct ObjList *anims; /* the animations(s) this offset corresponds to, if any */
  struct ObjList *jumps; /* jump or cond jump instructions which point back to this
			    symbol entry (not including header entries */
} IsSymTblEnt;

/* Holds the info for each animation header. No header
   will use all 32 animation entries (offsets), but just
   to be on the safe side make the array a bit bigger.
   Index this in a hashtable using its corresponding
   images.Dat iscript ID value as the key */
typedef struct IsHeader {
  addr         offset;       /* location of this header in the iscript --
			        this is only used in construction. Do not
			        rely on it at any other time */
  uint16       id;           /* the iscript ID (used from images.Dat) */
  uint16       type;         /* numerical type of entry */
  IsSymTblEnt **st_entries; /* the pointers to symtbl entries which this header
			       uses as animations (internal abstraction of 'offsets') 
			       (this is an array of pointers, to-be-allocated) */
  addr         *offsets;     /* offsets to this entry's animations, may not use
				all of them, check header_types[type] to see how many,
				this array should only be used for temporary loading
				or unloading from/to an iscript.bin file, do not rely
				on these offsets internally. (array of pointers, tba) 
			        -- this should ONLY BE USED DURING CONSTRUCTION. The
			        array should be freed before the iscript object is
			        returned to the user. */
} IsHeader;

/* This structure olds all the info for each individual
   iscript instruction (opcode + args) */
typedef struct IsInstr {
  struct IsInstr *next;     /* next instr; NULL if unconditional goto or terminal */
  struct IsInstr *prev;     /* prev instr; NULL if no instruction just before us, or
				 the instruction just before us is an unconditional
				 jump or terminal */
  IsSymTblEnt     *st_entry; /* possible pointer to an associated symtbl entry that
			         points to us */
  IsSymTblEnt     *jump;     /* possible pointer to a symtbl entry that we jump to
				 if we are a jmp or cndjmp instr */
  byte             opcode;    /* the opcode value of the instr */
  int              numargs;   /* number of arguments to this opcode */
  uint16           *args;     /* args for this opcode, to be allocated 
				 (some args are only 1 byte long, but we'll just
				 extend it for now and truncate it later)
			         MAKE SURE this is allocated or set to NULL
                                 before you attempt to free the iscript. */
#ifdef __DEBUG
  addr              offset;   /* save original address, for debugging */
#endif
} IsInstr;

/* this is the full iscript abstract type when fully built.
   there is no direct way to access the byte-code because they
   are really just linked-lists of instructions pointed to by
   each symbl table */
typedef struct Iscript {
  HashTable *headers; /* pointers to our headers */
  HashTable *symtbl;  /* all the symtbl entries hashed by uniq_id,
		         for easy lookup */
  int       version;  /* either STARCRAFT or BROODWAR, their formats are
			 a little different; reading (iscript_new)
                         work on both types, but writing for the starcraft
                         type may not work properly yet; see iscript_save
                         for details */
  long      uniq_id;  /* a uniq id that can be used for adding additional
			 unique symtbl entries */
} Iscript;

/* an enumeration object/stream that is used to iterate through
   bytecode instructions in order */
typedef struct IsInstrEnum {
  IsInstr *next;
} IsInstrEnum;

/* A IsIdEnum (enumeration used over the iscript ids in an Iscript
   object) is really a Hashtable enumeration, but we distinguish the
   two for cleanliness */
typedef HashEnum IsIdEnum;

extern uint16 __header_types[];
#define MAX_HEADER_TYPE     31 /* index starts at 0 */
#define isheader_type_to_numanims(type) (__header_types[(type)])

extern Iscript *iscript_new(char *file_name, int version);
extern Iscript *iscript_new_empty();
extern int iscript_save(char *file_name, Iscript *iscript);
extern void iscript_free(Iscript *iscript);
extern void iscript_set_version(Iscript *iscript, int version);
extern int iscript_isvalid_anim(Iscript *iscript, 
		      uint16 iscript_id, 
		      int animno);
extern int iscript_numberof_anims(Iscript *iscript, uint16 id);
extern IsSymTblEnt *iscript_get_symtblent(Iscript *iscript, long id);
extern IsSymTblEnt *iscript_get_symtblent_by_animno(Iscript *iscript, 
						    uint16 id, int animno);
extern Iscript *iscript_extract_header(Iscript *iscript, uint16 id);
extern void iscript_remove_header(Iscript *iscript, uint16 id);
extern Iscript *iscript_merge(Iscript *dest, Iscript *source);
extern Iscript *iscript_separate_headers(Iscript *ip);

extern IsInstrEnum isinstrenum_create(Iscript *iscript,
					uint16 iscript_id,
					int animno);
extern IsInstrEnum isinstrenum_create_from_symtblent(IsSymTblEnt *st_entry);
extern IsInstr *isinstrenum_next(IsInstrEnum *bcenum);
extern int isinstrenum_rewind(IsInstrEnum *bcenum);

extern IsSymTblEnt *isinstr_get_jump(IsInstr *jumpinstr);
extern IsSymTblEnt *isinstr_get_label(IsInstr *bc);
extern char *isinstr_name(IsInstr *bc);
extern List *isinstr_reached_by(IsInstr *ip);

extern ObjList *symtblent_get_anim_list(IsSymTblEnt *st_entry);
extern long symtblent_get_id(IsSymTblEnt *st_entry);

extern IsIdEnum isidenum_create(Iscript *iscript);
extern uint16 isidenum_next(IsIdEnum *idenum);

#ifdef __DEBUG
extern void load__hash();
extern void free__hash();
extern void print_headers(FILE *handle);
extern int super_debug_function();
#endif

#endif
