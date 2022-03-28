/* tokenizer.h */

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

/* maximum allowable length for lines in the input file
   (in chars, including newline) -- the tokenizer will actually
   handle lines longer than this, but if tokens get longer than
   this, then it has a potential of breaking, or at least giving
   wierd results */
#define MAX_LINE_LENGTH 512

typedef struct Tokenizer {
  FILE *stream;
  char delimiters[256];
  char chartokens[256];
  char currentline[MAX_LINE_LENGTH+2]; /* extra byte for safety */
  char *ptr;
  int  eol;
  char tokenbuffer[(MAX_LINE_LENGTH+1)*2];
  char chartok;
} Tokenizer;

extern Tokenizer *tokenizer_new(FILE *stream, char *delimiters, char *chartokens);
extern void tokenizer_free(Tokenizer *tok);
extern char *tokenizer_get_line(Tokenizer *tok);
extern char *tokenizer_next(Tokenizer *tok);

#endif
