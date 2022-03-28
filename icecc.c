/*
  IceCC. The main part of the actual compiler.
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

/* $Id: icecc.c,v 1.15 2002/06/09 08:11:55 jp Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "scdef.h"
#include "iscript-instr.h"
#include "tokenizer.h"
#include "icecc-share.h"

#define DEFAULT_OUTFILE "iscript.bin"

static ObjHashTable *table_animname_to_num  = NULL;
static ObjHashTable *table_instrname_to_num = NULL;

#define animname_to_num(name)  ((uint16 *)objhashtable_find(table_animname_to_num, name))
#define instrname_to_num(name) ((byte *)objhashtable_find(table_instrname_to_num, name))

static HashTable    *table_symtbl   = NULL; /* id -> symtblent */
static HashTable    *table_headers  = NULL; /* id -> header */
static ObjHashTable *table_names    = NULL; /* tokname -> symtblent */
static IsInstr      *current_instr  = NULL; /* last read instr */ 
static ObjQueue     *relocq         = NULL; /* tokname -> **symtblent */
static ObjQueue     *instr_ptrs     = NULL; /* keep track of all the instructions,
					       just incase we need to free them */
static long         uniqid          = 0;

/* holds the name string and the
   line where it appeared so error
   info can be given later if relocation
   info is not found */
typedef struct TokName {
  char *string;
  long lineno;
} TokName;

/* relocation information for
   the relocation queue */
typedef struct RelocPair {
  IsHeader    *hp;
  uint16      animno;
  IsInstr     *jump;
  TokName     *name;
} RelocPair;

typedef struct IsHeaderTok {
  uint16  id;
  uint16  type;
  TokName *labels[MAX_ANIM_ENTRIES];
} IsHeaderTok;

typedef struct IsSymTblEntTok {
  ObjQueue *names;
} IsSymTblEntTok;

typedef struct IsInstrTok {
  char    *opcode;
  int     numargs;
  TokName *args[MAX_INSTR_ARGS];
} IsInstrTok;

#define ERROR_TOK   -1
#define EOF_TOK      0
#define HEADER_TOK   1
#define SYMTBL_TOK   2
#define INSTR_TOK    3

typedef struct ParsedTok {
  int type; /* one of the above *_TOK thingies */
  IsHeaderTok *hp;
  IsSymTblEntTok *sp;
  IsInstrTok  *ip;
} ParsedTok;

static int init_resource_tables();
static void free_resource_tables();

static TokName *tokname_new(char *name, long lineno);
static void tokname_free(TokName *t);

static ParsedTok *parsedtok_new();
static void parsedtok_free(ParsedTok *pt);

static void set_filename(char *name);
static void set_verboseness(int v);
static void parse_error(long lineno, char *fmt, ...);
static void parse_warning(long lineno, char *fmt, ...);
static ObjQueue *parse_line(Tokenizer *tok, long *lineno);
static int parse_header(ParsedTok *pt, Tokenizer *tok, long *lineno);
static int parse_symtblent(ParsedTok *pt, Tokenizer *tok, char *name, 
			   ObjQueue *rest_tokens, long *lineno);
static int parse_instr(ParsedTok *pt, char *name, ObjQueue *rest_tokens, long *lineno);
static int parse_next(ParsedTok *pt, Tokenizer *tok, long *lineno);

static int islegal_labelname(char *name);
static int verify_headertok(IsHeaderTok *htok, long lineno);
static int verify_symtblenttok(IsSymTblEntTok *stok, IsInstrTok *itok, long lineno);
static int verify_instrtok(IsInstrTok *itok, long lineno, int label);
static void read_headertok(IsHeaderTok *htok);
static void read_symtblenttok(IsSymTblEntTok *stok, IsInstrTok *itok);
static IsInstr *read_instrtok(IsInstrTok *itok);

static int parser(FILE *stream);
static void err_remove_label(long id, HashTable *done, ObjHashTable *freeinstrs);

static void err_free_headers(HashTable *headers);
static void err_free_symtbl(HashTable *symtbl);
static void err_free_instrs(ObjQueue *instrs);
static void free_names(ObjHashTable *names);

static int tokname_hash_fn(void *a);
static int tokname_eq_fn(void *a, void *b);

static int is_compiled(char *filename);
static void err_display_help();
static void get_args(int argc, char **argv, ObjQueue *infiles, char *outfile, int *verbose);

typedef struct InstrInfo {
  int vararg;  /* the index of the arg that determines the # if true, -1 if false */
  int jumparg; /* the index of the jump arg if true, -1 if false */
  int numargs; /* the number of expected args, -1 if vararg */
} InstrInfo;

#define instr_vararg(op)   (instr_info[(op)].vararg)
#define instr_jumparg(op)  (instr_info[(op)].jumparg)
#define instr_numargs(op)  (instr_info[(op)].numargs)

static InstrInfo instr_info[] = {
  /* 00 */ {-1,-1,1},
  /* 01 */ {-1,-1,1},
  /* 02 */ {-1,-1,1},
  /* 03 */ {-1,-1,1},
  /* 04 */ {-1,-1,2},
  /* 05 */ {-1,-1,1},
  /* 06 */ {-1,-1,2},
  /* 07 */ {-1,0,1},
  /* 08 */ {-1,-1,3},
  /* 09 */ {-1,-1,3},
  /* 0a */ {-1,-1,1},
  /* 0b */ {-1,-1,1},
  /* 0c */ {-1,-1,0},
  /* 0d */ {-1,-1,3},
  /* 0e */ {-1,-1,3},
  /* 0f */ {-1,-1,3},
  /* 10 */ {-1,-1,3},
  /* 11 */ {-1,-1,3},
  /* 12 */ {-1,-1,1},
  /* 13 */ {-1,-1,3},
  /* 14 */ {-1,-1,3},
  /* 15 */ {-1,-1,2},
  /* 16 */ {-1,-1,0},
  /* 17 */ {-1,-1,1},
  /* 18 */ {-1,-1,1},
  /* 19 */ {0,-1,-1},
  /* 1a */ {-1,-1,2},
  /* 1b */ {-1,-1,0},
  /* 1c */ {0,-1,-1},
  /* 1d */ {-1,-1,0},
  /* 1e */ {-1,1,2},
  /* 1f */ {-1,-1,1},
  /* 20 */ {-1,-1,1},
  /* 21 */ {-1,-1,0},
  /* 22 */ {-1,-1,1},
  /* 23 */ {-1,-1,1},
  /* 24 */ {-1,-1,1},
  /* 25 */ {-1,-1,1},
  /* 26 */ {-1,-1,0},
  /* 27 */ {-1,-1,0},
  /* 28 */ {-1,-1,1},
  /* 29 */ {-1,-1,1},
  /* 2a */ {-1,-1,0},
  /* 2b */ {-1,-1,1},
  /* 2c */ {-1,-1,1},
  /* 2d */ {-1,-1,0},
  /* 2e */ {-1,-1,0},
  /* 2f */ {-1,-1,0},
  /* 30 */ {-1,-1,0},
  /* 31 */ {-1,-1,1},
  /* 32 */ {-1,-1,0},
  /* 33 */ {-1,-1,0},
  /* 34 */ {-1,-1,1},
  /* 35 */ {-1,0,1},
  /* 36 */ {-1,-1,0},
  /* 37 */ {-1,-1,1},
  /* 38 */ {-1,-1,1},
  /* 39 */ {-1,0,1},
  /* 3a */ {-1,1,2},
  /* 3b */ {-1,2,3},
  /* 3c */ {-1,2,3},
  /* 3d */ {-1,-1,2},
  /* 3e */ {-1,-1,0},
  /* 3f */ {-1,0,1},
  /* 40 */ {-1,-1,1},
  /* 41 */ {-1,-1,1},
  /* 42 */ {-1,-1,3},
  /* 43 */ {-1,-1,0},
  /* 44 */ {-1,-1,0},
};

static int init_resource_tables() {
  uint16 *num;
  byte *n;
  int i;

  table_animname_to_num = objhashtable_new(32, string_hash, string_eq);

  for (i=0; i<128; i++) {
    if (animno_to_name(i) == NULL)
      break;
    num = malloc(sizeof(uint16));
    *num = i;
    objhashtable_insert(table_animname_to_num, animno_to_name(i), num);
  }

  table_instrname_to_num = objhashtable_new(64, string_hash, string_eq);

  for (i=0; i<MAX_OPCODE+1; i++) {
    n = malloc(sizeof(byte));
    *n = i;
    objhashtable_insert(table_instrname_to_num, isinstr_get_name(i), n);
  }

  return 0;
}

static void free_resource_tables() {
  ObjHashEnum e = objhashenum_create(table_animname_to_num);
  uint16 *num;
  byte *n;
  
  while ((num = objhashenum_next(&e)) != NULL)
    free(num);
  objhashtable_free(table_animname_to_num);
  table_animname_to_num = NULL;

  e = objhashenum_create(table_instrname_to_num);
  while ((n = objhashenum_next(&e)) != NULL)
    free(n);
  objhashtable_free(table_instrname_to_num);
  table_instrname_to_num = NULL;
}

/* assymes name is allocated and is passed off to the tokname
   to free */
static TokName *tokname_new(char *name, long lineno) {
  TokName *t = malloc(sizeof(TokName));
  t->string = name;
  t->lineno = lineno;
  return t;
}
/* will free the internal string */
static void tokname_free(TokName *t) {
  free(t->string);
  free(t);
}

/* This object is passed into the parsing functions
   to get filled out */
static ParsedTok *parsedtok_new() {
  ParsedTok *pt = malloc(sizeof(ParsedTok));

  pt->type = ERROR_TOK;
  pt->hp = calloc(1, sizeof(IsHeaderTok));
  pt->hp->id = 0xFFFF;
  pt->hp->type = 0xFFFF;
  pt->sp = calloc(1, sizeof(IsSymTblEntTok));
  pt->ip = calloc(1, sizeof(IsInstrTok));

  return pt;
}

static void parsedtok_free(ParsedTok *pt) {
  int i;

  for (i=0; i<MAX_ANIM_ENTRIES; i++)
    if (pt->hp->labels[i] != NULL)
      tokname_free(pt->hp->labels[i]);
  free(pt->hp);
  if (pt->sp->names != NULL) {
    while (!objqueue_isempty(pt->sp->names))
      tokname_free(objqueue_remove(pt->sp->names));
    objqueue_free(pt->sp->names);
  }
  free(pt->sp);
  free(pt->ip->opcode);
  for (i=0; i<pt->ip->numargs; i++)
    if (pt->ip->args[i] != NULL)
      tokname_free(pt->ip->args[i]);
  free(pt->ip);
  free(pt);
}

#define DEFAULT_WARNINGS 10
static char compiler_filename[256];
static int  warnings_so_far = 0;
static char compiler_warning_verbose = 0;

static void set_filename(char *name) {
  compiler_filename[255] = '\0';
  strncpy(compiler_filename, name, 255);
}

static void set_verboseness(int v) {
  compiler_warning_verbose = v;
}

static void parse_error(long lineno, char *fmt, ...) {
  va_list ap;

  if (warnings_so_far++ > DEFAULT_WARNINGS && !compiler_warning_verbose)
    return;

  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: error: ", compiler_filename, (int)lineno);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

static void parse_warning(long lineno, char *fmt, ...) {
  va_list ap;

  if (warnings_so_far++ > DEFAULT_WARNINGS && !compiler_warning_verbose)
    return;

  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: warning: ", compiler_filename, (int)lineno);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

/* Parses a line and returns all the tokens in a queue;
   you are expected to free the queue and the strings
   inside (objqueue_freeall). Empty queue if EOF or error */
static ObjQueue *parse_line(Tokenizer *tok, long *lineno) {
  ObjQueue *tokens = objqueue_new();
  char *token, *buf;

  /* find the first token that isn't on a blank line */
  while ((token = tokenizer_next(tok)) != NULL) {

    if (!streq(token,"\n") && !streq(token, COMMENT_START))
      break;
    else if (streq(token,"\n"))
      ++(*lineno);

    /* skip comments */
    if (streq(token, COMMENT_START)) {
      while (1) {
	token = tokenizer_next(tok);
	if (token == NULL)
	  break;
	if (streq(token,"\n")) {
	  ++(*lineno);
	  break;
	}
      }
    }
  }

  while (token != NULL) {
    /* EOL */
    if (streq(token,"\n")) {
      ++(*lineno);
      return tokens;
    }
    /* skip comments */
    if (streq(token, COMMENT_START)) {
      while (1) {
	token = tokenizer_next(tok);
	if (token == NULL)
	  return tokens;
	if (streq(token,"\n")) {
	  ++(*lineno);
	  return tokens;
	}
      }
    }
    buf = malloc((strlen(token)+1)*sizeof(char));
    strcpy(buf, token);
    objqueue_insert(tokens, buf);

    token = tokenizer_next(tok);
  }
  
  return tokens;
}

/* helper for parse next */
static int parse_header(ParsedTok *pt, Tokenizer *tok, long *lineno) {
  ObjQueue *tokens;
  char *var, *val;
  int error = 0;
  int i;
  
  pt->type = HEADER_TOK;
  /* clear out old strings */
  pt->hp->id   = 0xFFFF;
  pt->hp->type = 0xFFFF;
  for (i=0; i<MAX_ANIM_ENTRIES; i++) {
    free(pt->hp->labels[i]);
    pt->hp->labels[i] = NULL;
  }
  
  tokens = parse_line(tok, lineno);

  while (1) {
    uint16 *animno;

    if (objqueue_isempty(tokens)) {
      parse_error(*lineno, "unexpected EOF while reading header");
      error = 1;
      return error;
    }
    var = objqueue_remove(tokens);
    /* end header */
    if (streq(var, HEADER_TAG_END)) {
      free(var);
      if (!objqueue_isempty(tokens))
	parse_warning(*lineno, "ignoring extraneous text after '" HEADER_TAG_END "' header tag");
      objqueue_freeall(tokens);
      break;
    }
    val = objqueue_remove(tokens);

    if (val == NULL) {
      parse_error(*lineno, "header variable '%s' is missing its value", var);
      free(var);
      error = 1;
      /* check iscript id */
    } else if (streq(var, ISCRIPT_ID_TAG)) {
      char   *e;
      uint16 id = (uint16)strtol(val, &e, 0);
      if (*e != '\0') {
	parse_error(*lineno, "'%s' is not a valid " ISCRIPT_ID_TAG " value", val);
	error = 1;
      } else {
	if (pt->hp->id != 0xFFFF)
	  parse_warning(*lineno, "redefining '" ISCRIPT_ID_TAG "' in header");
	pt->hp->id = id;
      }
      free(var);
      free(val);
      /* check type tag */
    } else if (streq(var, ANIM_TYPE_TAG)) {
      char   *e;
      uint16 type = (uint16)strtol(val, &e, 0);
      if (*e != '\0') {
	parse_error(*lineno, "'%s' is not a valid " ANIM_TYPE_TAG " value", val);
	error = 1;
      } else {
	if (pt->hp->type != 0xFFFF)
	  parse_warning(*lineno, "redefining '" ANIM_TYPE_TAG "' in header");
	pt->hp->type = type;
      }
      free(var);
      free(val);
      /* check if its a label */
    } else if ((animno = animname_to_num(var)) != NULL) {
      if (pt->hp->labels[*animno]) {
	parse_warning(*lineno, "redefining animation label '%s' in header", var);
	tokname_free(pt->hp->labels[*animno]);
      }
      pt->hp->labels[*animno] = tokname_new(val, *lineno);
      /* don't free the val, keep it in the struct */
      free(var);
    } else {
      /* dunno what the hell it is */
      parse_error(*lineno, "unknown animation name '%s'", var);
      error = 1;
      free(var);
      free(val);
    }

    if (!objqueue_isempty(tokens))
      parse_warning(*lineno, "ignoring extraneous text in header");
    objqueue_freeall(tokens);

    tokens = parse_line(tok, lineno);
  }
  
  if (pt->hp->id == 0xFFFF) {
    parse_error(*lineno, "header is missing " ISCRIPT_ID_TAG " value");
    error = 1;
  }
  if (pt->hp->type == 0xFFFF) {
    parse_error(*lineno, "header is missing " ANIM_TYPE_TAG " value");
    error = 1;
  }

  return error;
}

/* helper for parse_next */
static int parse_symtblent(ParsedTok *pt, Tokenizer *tok, char *name, 
			   ObjQueue *rest_tokens, long *lineno)
{
  int error = 0;
  char *next, *save, *opcode = NULL;

  
  if (pt->sp->names != NULL) {
    while (!objqueue_isempty(pt->sp->names))
      tokname_free(objqueue_remove(pt->sp->names));
    objqueue_free(pt->sp->names);
  }
  pt->sp->names = objqueue_new();

  /* insert the first name we found before */
  objqueue_insert(pt->sp->names, tokname_new(name, *lineno));

  /* for a symtbl, we want to read all the label
     names corresponding to this point AND the
     next instruction so we can connect them */
  while (1) {
    while (!objqueue_isempty(rest_tokens)) {
      next = objqueue_remove(rest_tokens);
      save = objqueue_remove(rest_tokens);
      /* look for an instruction -- that is, a token
	 without a ":" token after it */
      if (save == NULL || !streq(save, LABEL_TERMINATOR)) {
	if (save != NULL)
	  objqueue_putback(rest_tokens, save);
	opcode = next;
	break;
      }
      /* not an instruction, so i must be another label */
      free(save);
      objqueue_insert(pt->sp->names, tokname_new(next, *lineno));
    }
    if (opcode != NULL)
      break;

    objqueue_free(rest_tokens);
    rest_tokens = parse_line(tok, lineno);

    if (objqueue_isempty(rest_tokens)) {
      objqueue_freeall(rest_tokens);
      pt->type = EOF_TOK;
      parse_error(*lineno, "unexpected EOF while reading label (expected instruction)");
      error = 1;
      return error;
    }
  }
  /* now parse the instruction */
  error = parse_instr(pt, opcode, rest_tokens, lineno);

  pt->type = SYMTBL_TOK;
  return error;
}

/* helper for parse_next */
static int parse_instr(ParsedTok *pt, char *name, ObjQueue *rest_tokens, long *lineno)
{
  int error = 0;
  char *next;
  int i;

  free(pt->ip->opcode);
  for (i=0; i<pt->ip->numargs; i++) {
    if (pt->ip->args[i] != NULL)
      tokname_free(pt->ip->args[i]);
    pt->ip->args[i] = NULL;
  }
  pt->ip->numargs = 0;

  pt->type = INSTR_TOK;
  pt->ip->opcode = name;
  while (!objqueue_isempty(rest_tokens)) {
    next = objqueue_remove(rest_tokens);
    pt->ip->args[pt->ip->numargs++] = tokname_new(next, *lineno);
    if (pt->ip->numargs > MAX_INSTR_ARGS) {
	error = 1;
	break;
    }
  }
  objqueue_freeall(rest_tokens);

  return error;
}

/* parse the next object out of the token stream -- either a header,
   label, or instruction */
static int parse_next(ParsedTok *pt, Tokenizer *tok, long *lineno) {
  char *next, *save;
  ObjQueue *tokens = parse_line(tok, lineno);

  if (objqueue_isempty(tokens)) {
    pt->type = EOF_TOK;
    return EOF_TOK;
  }

  next = objqueue_remove(tokens);
  if (streq(next, HEADER_TAG_START)) {
    if (!objqueue_isempty(tokens))
      parse_warning(*lineno, "ignoring extraneous text after '" HEADER_TAG_START "' header tag");
    free(next);
    objqueue_freeall(tokens);
    if (parse_header(pt, tok, lineno))
      return ERROR_TOK;
    return HEADER_TOK;
  }

  save = next;
  next = objqueue_remove(tokens);

  if (next != NULL && streq(next, LABEL_TERMINATOR)) {
    free(next);
    if (parse_symtblent(pt, tok, save, tokens, lineno))
      return ERROR_TOK;
    return SYMTBL_TOK;
  }
  
  if (next != NULL)
    objqueue_putback(tokens, next);

  if (parse_instr(pt, save, tokens, lineno))
    return ERROR_TOK;
  return INSTR_TOK;
}

static int islegal_labelname(char *name) {
  if (streq(name, HEADER_TAG_START) ||
      streq(name, HEADER_TAG_END)   ||
      streq(name, EMPTY_LABEL_STRING))
    return 0;
  return 1;
}

static int verify_headertok(IsHeaderTok *htok, long lineno) {
  int i, error = 0;

  if (current_instr != NULL)
    parse_warning(lineno, "header definition is between two connected instructions");
  
  if (hashtable_find(table_headers, htok->id) != NULL) {
    parse_error(lineno, "multiple definition of header with " 
		ISCRIPT_ID_TAG " %d", (int)htok->id);
    error = 1;
  }

  if (htok->type > MAX_HEADER_TYPE || 
      isheader_type_to_numanims(htok->type) == 0) {
    parse_error(lineno, "%d is not a valid header " ANIM_TYPE_TAG " value", htok->type);
    error = 1;
  } else {
    for (i=0; i<MAX_ANIM_ENTRIES; i++) {
      if (i>isheader_type_to_numanims(htok->type) &&
	  htok->labels[i] != NULL) {
	parse_warning(htok->labels[i]->lineno, 
		      "header " ANIM_TYPE_TAG " %d does not have '%s' animation; discarding",
		      htok->type, animno_to_name(i));
	tokname_free(htok->labels[i]);
	htok->labels[i] = NULL;
      } else if (htok->labels[i] == NULL) {
	if (i<isheader_type_to_numanims(htok->type))
	  parse_warning(lineno,
			"header is missing '%s' animation; assuming " 
			EMPTY_LABEL_STRING, animno_to_name(i));
      } else if (streq(htok->labels[i]->string, EMPTY_LABEL_STRING)) {
	tokname_free(htok->labels[i]);
	htok->labels[i] = NULL;
      } else if (!islegal_labelname(htok->labels[i]->string)) {
	parse_error(htok->labels[i]->lineno, "'%s' is not a legal label name", 
		    htok->labels[i]->string);
	error = 1;
      }
    }
  }

  return error;
}

static int verify_symtblenttok(IsSymTblEntTok *stok, IsInstrTok *itok, long lineno) {
  int error = 0;
  TokName *name, *prev;
  ObjQueue *tmp = objqueue_new();

  while (!objqueue_isempty(stok->names)) {
    name = objqueue_remove(stok->names);
    if ((prev = objhashtable_findkey(table_names, name)) != NULL) {
      parse_error(name->lineno, "multiple definition of label '%s' "
		  "(previous definition at line %d)", name->string, prev->lineno);
      error = 1;
    }
    if (!islegal_labelname(name->string)) {
      parse_error(name->lineno, "'%s' is not a legal label name", 
		  name->string);
      error = 1;
    }
    objqueue_insert(tmp, name);
  }
  objqueue_free(stok->names);
  stok->names = tmp;

  error = verify_instrtok(itok, lineno, 1) || error;  
  return error;
}

static int verify_instrtok(IsInstrTok *itok, long lineno, int label) {
  static int skipping = 0;
  int i, val, args, error = 0;
  char *e;
  byte *op;

  /* a parse error may have prevented a label from being read, so only
     display the "skipping" instructions error once per string that is
     skipped */
  if (current_instr == NULL && !label) {
    error = 1;
    if (!skipping)
      parse_error(lineno, "instruction '%s' not connected to a label; discarding "
		  "instructions until next label", itok->opcode);
    skipping = 1;
  } else
    skipping = 0;

  if ((op = instrname_to_num(itok->opcode)) == NULL) {
    parse_error(lineno, "unknown instruction opcode '%s'", itok->opcode);
    error = 1;
    return error;
  }

  if (instr_vararg(*op) != -1) {
    if (itok->args[instr_vararg(*op)] == NULL) {
      parse_error(lineno, "variable argument instruction '%s' missing argument %d",
		  itok->opcode, instr_vararg(*op));
      error = 1;
      return error;
    }
    args = strtol(itok->args[instr_vararg(*op)]->string, &e, 0) + 1;
    if (*e != '\0') {
      parse_error(lineno, "'%s' is not a valid argument in instruction '%s'",
		  itok->args[instr_vararg(*op)]->string, itok->opcode);
      error = 1;
      return error;
    }
  } else
    args = instr_numargs(*op);

  for (i=0; i<MAX_INSTR_ARGS; i++) {
    if (i>=args) {
      if (itok->args[i] != NULL) {
	parse_error(lineno, "too many arguments (%d) to instruction '%s', should be %d", i+1, itok->opcode, args);
	error = 1;
	return error;
      }
      break;
    } else if (itok->args[i] == NULL) {
      parse_error(lineno, "not enough arguments (%d) to instruction '%s', should be %d", i, itok->opcode, args);
      error = 1;
      return error;
    } else if (instr_jumparg(*op) == i) {
      if (!islegal_labelname(itok->args[i]->string)) {
	parse_error(lineno, "'%s' is not a legal label name", 
		    itok->args[i]->string);
	error = 1;
      }
    } else {
      val = strtol(itok->args[i]->string, &e, 0);
      if (*e != '\0') {
	parse_error(lineno, "'%s' is not a valid argument in instruction '%s'",
		    itok->args[i]->string, itok->opcode);
	error = 1;
      }
    }
  }
  itok->numargs = args;

  return error;
}

static void read_headertok(IsHeaderTok *htok) {
  IsHeader *hp = calloc(1, sizeof(IsHeader));
  int i;

  hp->id = htok->id;
  hp->type = htok->type;
  hp->st_entries = 
    calloc(isheader_type_to_numanims(htok->type), sizeof(IsSymTblEnt *));
  
  for (i=0; i<isheader_type_to_numanims(htok->type); i++) {
    if (htok->labels[i] != NULL) {
      /* will comeback and fill this in later */
      RelocPair *info = malloc(sizeof(RelocPair));
      info->hp   = hp;
      info->animno = i;
      info->jump = NULL;
      info->name = htok->labels[i];
      objqueue_insert(relocq, info);
      htok->labels[i] = NULL;
    }
  }

  hashtable_insert(table_headers, hp->id, hp);
}

static void read_symtblenttok(IsSymTblEntTok *stok, IsInstrTok *itok) {
  IsSymTblEnt *st_entry = calloc(1, sizeof(IsSymTblEnt));

  st_entry->id    = uniqid++;
  /* these will be filled in later when we have full
     relocation info */
  st_entry->anims = objlist_new();
  st_entry->jumps = objlist_new();
  st_entry->bc    = read_instrtok(itok);
  st_entry->bc->st_entry = st_entry;

  while (!objqueue_isempty(stok->names))
    objhashtable_insert(table_names, objqueue_remove(stok->names), st_entry);

  hashtable_insert(table_symtbl, st_entry->id, st_entry);
}

static IsInstr *read_instrtok(IsInstrTok *itok) {
  IsInstr *instr = calloc(1, sizeof(IsInstr));
  int i;

  instr->opcode  = *instrname_to_num(itok->opcode);
  instr->prev    = current_instr;
  if (current_instr != NULL)
    current_instr->next = instr;
  instr->numargs = itok->numargs;
  instr->args    = calloc(itok->numargs, sizeof(uint16));
  
  for (i=0; i<itok->numargs; i++) {
    if (instr_jumparg(instr->opcode) == i) {
      RelocPair *info = malloc(sizeof(RelocPair));
      info->hp   = NULL;
      info->animno = 0xFFFF;
      info->jump = instr;
      info->name = itok->args[i];
      objqueue_insert(relocq, info);
      itok->args[i] = NULL;
    } else
      instr->args[i] = strtol(itok->args[i]->string, NULL, 0);
  }

  if (isinstr_get_type(instr->opcode) == INSTR_TERM ||
      isinstr_get_type(instr->opcode) == INSTR_JMP)
    current_instr = NULL;
  else
    current_instr = instr;
  objqueue_insert(instr_ptrs, instr);

  return instr;
}

static int parser(FILE *stream) {
  Tokenizer *tok = tokenizer_new(stream, " \t", "\n" COMMENT_START LABEL_TERMINATOR);
  ParsedTok *pt = parsedtok_new();
  int error = 0, tokval;
  long lineno = 0;
  ObjHashEnum enumeration;
  TokName *name;
  IsSymTblEnt *st_entry;
  ObjQueue *bad_labels;

  do {
    tokval = parse_next(pt, tok, &lineno);

    switch(tokval) {
    case HEADER_TOK:
      if (verify_headertok(pt->hp, lineno) == 0)
	read_headertok(pt->hp);
      else
	error = 1;
      break;
    case SYMTBL_TOK:
      if (verify_symtblenttok(pt->sp, pt->ip, lineno) == 0)
	read_symtblenttok(pt->sp, pt->ip);
      else
	error = 1;
      break;
    case INSTR_TOK:
      if (verify_instrtok(pt->ip, lineno, 0) == 0)
	read_instrtok(pt->ip);
      else
	error = 1;
      break;
    case ERROR_TOK:
      error = 1;
      break;
    }
  } while (tokval != EOF_TOK);

  parsedtok_free(pt);
  tokenizer_free(tok);

  if (current_instr != NULL) {
    parse_error(lineno, "final instruction does not terminate");
    error = 1;
  }

  /* fill in relocation info for labels */
  while(!objqueue_isempty(relocq)) {
    RelocPair *info = objqueue_remove(relocq);
    IsSymTblEnt *st;

    if ((st = objhashtable_find(table_names, info->name)) == NULL) {
      parse_error(info->name->lineno, "can't find label name '%s'", info->name->string);
      error = 1;
    } else {
      /* connect label to header */
      if (info->hp != NULL) {
	IsAnim *anim = malloc(sizeof(IsAnim));

	anim->header = info->hp;
	anim->animno = info->animno;
	anim->type   = HEADER;
	objlist_insert(st->anims, anim);

	info->hp->st_entries[info->animno] = st;
      }
      /* connect label to instruction */
      if (info->jump != NULL) {
	objlist_insert(st->jumps, info->jump);
	info->jump->jump = st;
      }
    }
    tokname_free(info->name);
    free(info);
  }

  /* find labels that can't be reached */
  enumeration = objhashenum_create(table_names);
  bad_labels = objqueue_new();
  while (1) {
    objhashenum_next_pair(&enumeration, (void **)&name, (void **)&st_entry);
    if (name == NULL)
      break;
    if (objlist_length(st_entry->anims) < 1 &&
	objlist_length(st_entry->jumps) < 1) {
      parse_warning(name->lineno, "label '%s' is not referred to "
		    "by any headers or instructions", name->string);
      objqueue_insert(bad_labels, st_entry);
    }
    if (objlist_length(st_entry->anims) < 1) {
      List *l = isinstr_reached_by(st_entry->bc);
      if (list_length(l) < 1) {
	parse_warning(name->lineno, "label '%s' is unreachable", name->string);
	objqueue_insert(bad_labels, st_entry);
      }
      list_free(l);
    }
  }

  /* don't continue after this if there have been errors
     up until now, because we might have to free instructions
     when checking label validity, and an error would free it
     twice */
  if (error) {
    objqueue_free(bad_labels);
    return error;
  }

  /* found some bad labels -- get rid of them */
  if (!objqueue_isempty(bad_labels)) {
    HashTable *done = hashtable_new(16);
    ObjHashTable *freeinstrs = objhashtable_new(64, pointer_hash_fn, pointer_eq_fn);
    IsInstr *instr;

    while (!objqueue_isempty(bad_labels)) {
      st_entry = objqueue_remove(bad_labels);
      err_remove_label(st_entry->id, done, freeinstrs);
    }

    enumeration = objhashenum_create(freeinstrs);
    while ((instr = objhashenum_next(&enumeration)) != NULL) {
      if (instr->prev != NULL)
	instr->prev->next = NULL;
      if (instr->next != NULL)
	instr->next->prev = NULL;
      free(instr->args);
      free(instr);
    }
    hashtable_free(done);
    objhashtable_free(freeinstrs);
    objqueue_free(bad_labels);
  } 

  return error;
}

static void err_remove_label(long id, HashTable *done, ObjHashTable *freeinstrs) {
  IsSymTblEnt *st_entry = hashtable_remove(table_symtbl, id);
  IsInstr *instr;

  if (st_entry == NULL)
    return;

  if (hashtable_find(done, st_entry->id) != NULL)
    return;
  hashtable_insert(done, st_entry->id, (void *)1);

  instr = st_entry->bc;
  objlist_freeall(st_entry->anims);
  objlist_free(st_entry->jumps);
  free(st_entry);
  instr->st_entry = NULL;
  
  while (!objhashtable_find(freeinstrs, instr)) {
    List *l = isinstr_reached_by(instr);
    if (list_length(l) > 0) {
      list_free(l);
      break;
    }
    list_free(l);
    objhashtable_insert(freeinstrs, instr, instr);
    instr = instr->next;
    if (instr == NULL)
      break;
    /* don't need to follow jumps since we should
       have caught those labels as 'bad' too and will
       get to them later */
  }
}

static void err_free_headers(HashTable *headers) {
  HashEnum henum;
  IsHeader *hdrptr;
  
  if (headers == NULL)
    return;
  else
    henum = hashenum_create(headers);

  while ((hdrptr = hashenum_next(&henum)) != NULL) {
    if (hdrptr->offsets != NULL)
      free(hdrptr->offsets);
    if (hdrptr->st_entries != NULL)
      free(hdrptr->st_entries);
    free(hdrptr);
  }

  hashtable_free(headers);
}

static void err_free_symtbl(HashTable *st) {
  HashEnum henum;
  IsSymTblEnt *st_entry;

  if (st == NULL)
    return;
  else
    henum = hashenum_create(st);

  while ((st_entry = hashenum_next(&henum)) != NULL) {
    if (st_entry->anims != NULL)
      objlist_freeall(st_entry->anims);
    if (st_entry->jumps != NULL)
      objlist_free(st_entry->jumps);
    free(st_entry);
  }

  free(st);
}

static void err_free_instrs(ObjQueue *instrs) {
  IsInstr *ip;

  if (instrs == NULL)
    return;

  while (!objqueue_isempty(instrs)) {
    ip = objqueue_remove(instrs);
    free(ip->args);
    free(ip);
  }
  objqueue_free(instrs);
}

static void free_names(ObjHashTable *names) {
  ObjHashEnum e;
  TokName *t;

  if (names == NULL)
    return;
  
  e = objhashenum_create(names);
  while ((t = objhashenum_next_key(&e)) != NULL)
    tokname_free(t);

  objhashtable_free(names);
}

static int tokname_hash_fn(void *a) {
  return string_hash(((TokName *)a)->string);
}
static int tokname_eq_fn(void *a, void *b) {
  return streq(((TokName *)a)->string, ((TokName *)b)->string);
}

/* compile an iscript thingy to an Iscript object;
   fp is the filestream to read from (assumed to be open
   and set to the beginning) and filename is the name
   displayed for error/warning messages */
Iscript *iscript_compile(FILE *fp, char *filename) {
  Iscript *iscript;

  init_resource_tables();
  set_filename(filename);

  table_symtbl  = hashtable_new(SYMTBL_HASH_SIZE);
  table_headers = hashtable_new(HEADERS_HASH_SIZE);
  table_names   = objhashtable_new(SYMTBL_HASH_SIZE, tokname_hash_fn, tokname_eq_fn);
  current_instr = NULL;
  relocq        = objqueue_new();
  instr_ptrs    = objqueue_new();
  uniqid        = 0;

  if (!parser(fp)) {
    iscript = malloc(sizeof(Iscript));
    iscript->version = BROODWAR;
    iscript->headers = table_headers;
    iscript->symtbl  = table_symtbl;
    iscript->uniq_id = uniqid;
    objqueue_free(instr_ptrs);
  } else {
    iscript = NULL;
    err_free_headers(table_headers);
    err_free_symtbl(table_symtbl);
    err_free_instrs(instr_ptrs);
  }

  table_symtbl  = NULL;
  table_headers = NULL;
  free_names(table_names);
  objqueue_free(relocq);
  current_instr = NULL;
  uniqid = 0;

  free_resource_tables();

  return iscript;
}

int main(int argc, char **argv) {
  Iscript *iscript, *tmp;
  ObjQueue *infiles = objqueue_new();
  char outfile[256];
  int verbose = 0;

  memset(outfile, '\0', 256);
  
  sc_register_prog_name(argv[0]);
  get_args(argc, argv, infiles, outfile, &verbose);
  set_verboseness(verbose);
  
  iscript = iscript_new_empty();

  while (!objqueue_isempty(infiles)) {
    char *filename = objqueue_remove(infiles);
    
    if (is_compiled(filename)) {
      if ((tmp = iscript_new(filename, BROODWAR)) == NULL)
	sc_err_fatal("could not create iscript from %s: %s", filename, 
		     strip_function_name(sc_get_err()));
    } else {
      FILE *source;
      if (streq(filename, "-"))
	source = stdin;
      else
	source = fopen(filename, "r");
      if (source == NULL)
	sc_err_fatal("could not open %s", filename);
      if ((tmp = iscript_compile(source, filename)) == NULL)
	sc_err_fatal("aborted due to syntax errors in %s", filename);
    }
    free(filename);
    iscript_merge(iscript, tmp);
  }
  
  if (iscript_save(outfile, iscript) == -1) {
    remove(outfile);
    sc_err_fatal("could not create %s: %s", outfile, 
		 strip_function_name(sc_get_err()));
  }

  iscript_free(iscript);
  objqueue_free(infiles);

#ifdef __DEBUG_MEMORY
  print_memory_leaks();
#endif

  return 0;
}

/* returns true if the file has a .bin extention */
static int is_compiled(char *filename) {
  
  while (*filename != '\0') {
    if (*filename == '.' && streq(filename, ".bin"))
      return 1;
    ++filename;
  }
  return 0;
}

static void err_display_help() {
  fprintf(stderr, 
	  "\nUsage: %s [-vh] [options] inputfile(s)\n\n"
	  "Options:\n\n"
	  "  -m                 merge with the default iscript.bin file\n"
	  "  -w                 display all syntax warnings and errors (usually\n"
	  "                      only the first 10 lines are displayed)\n"
	  "  -o ouputfile       compile the inputfiles to outputfile\n"
	  "  -c configfile      use the configuration file configfile\n"
	  "  -r configdir       use the configuration directory configdir\n"
	  "\n",
	  sc_get_prog_name());
  exit(1);
}

static void get_args(int argc, char **argv, ObjQueue *infiles, char *outfile, int *verbose) {
  int i;
  int merge = 0;
  char configdir[256];
  char *str;
  int nomoreswitches = 0;

  memset(configdir,'\0', 255);

  for (i=1; i<argc; i++) {
    if (streq(argv[i],"--")) {
      nomoreswitches = 1;
      continue;
    }
    if (argv[i][0] == '-' && !nomoreswitches) {
      while (argv[i] != NULL && strlen(argv[i]) > 1) {
	++argv[i];
	switch(argv[i][0]) {
	case 'h':
	  err_display_help();
	  break;
	case 'v':
	  display_version();
	  break;
	case 'w':
	  *verbose = 1;
	  break;
	case 'm':
	  merge = 1;
	  break;
	case 'o':
	  if (argv[i][1] != '\0')
	    strncpy(outfile, argv[i]+1, 255);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-o option expects a filename after it; e.g., -o output.txt");
	    strncpy(outfile, argv[i], 255);
	  }
	  argv[i] = NULL;
	  break;
	case 'c':
	  if (argv[i][1] != '\0')
	    set_config_file(argv[i]+1);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-c option expects a file after it; e.g., -c %s",
			   config_file_path);
	    set_config_file(argv[i]);
	  }
	  argv[i] = NULL;
	  break;
	case 'r':
	  if (argv[i][1] != '\0')
	    strncpy(configdir, argv[i]+1, 255);
	  else {
	    if (++i >= argc)
	      sc_err_fatal("-r option expects a directory after it; e.g., -r %s"
			   DIR_SEPARATOR "data", install_path);
	    strncpy(configdir, argv[i], 255);
	  }
	  argv[i] = NULL;
	  break;
	default:
	  sc_err_warn("unknown option: %c", argv[i][0]);
	  break;
	}
      }
    } else {
      str = malloc((strlen(argv[i])+1)*sizeof(char));
      strcpy(str, argv[i]);
      objqueue_insert(infiles, str);
      argv[i] = NULL;
    }
  }

  load_config_file();
  if (configdir[0] != '\0')
    set_config_dir(configdir);

  if (merge) {
    str = malloc((strlen(path_iscript_bin)+1)*sizeof(char));
    strcpy(str, path_iscript_bin);
    objqueue_putback(infiles, str);
  }
  if (objqueue_isempty(infiles))
      err_display_help();

  if (outfile[0] == '\0')
    strncpy(outfile, DEFAULT_OUTFILE, 255);

}
