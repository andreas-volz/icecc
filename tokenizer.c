/*
  IceCC. This is a mini stream tokenizer module.
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

/* $Id: tokenizer.c,v 1.2 2001/01/14 05:30:46 jp Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

/* new (hackish :P) tokenizer object. stream must be open, and the caller is
   responsible for closing it. delimiters delimits the tokens.
   chartokens is a collection of characters that you want to count
   as single tokens, but do not want to be delimiters; e.g., "\n"
   to return '\n' as a token in itself */
Tokenizer *tokenizer_new(FILE *stream, char *delimiters, char *chartokens) {
  Tokenizer *tok;
  
  if (stream == NULL)
    return NULL;

  tok = calloc(1, sizeof(Tokenizer));
  tok->stream = stream;
  tok->ptr = tok->tokenbuffer;
  tok->chartok = '\0';
  tok->eol = 1;
  strncpy(tok->delimiters, delimiters, 255);
  strncpy(tok->chartokens, chartokens, 255);
  return tok;
}

/* The caller is still responsible for closing the
   file stream handle */
void tokenizer_free(Tokenizer *tok) {
  free(tok);
}

/* Returns the current line that the tokenizer is
   processing. If the last token spans multiple lines,
   then, it returns the last line. Do not free this string  */
char *tokenizer_get_line(Tokenizer *tok) {
  return tok->currentline;
}

/* Returns the next token in the stream. do not free the string */
char *tokenizer_next(Tokenizer *tok) {
  static char singlecharbuf[] = { '\0', '\0' };
  char *token = "";
  char *fillstart = tok->tokenbuffer;
  int  filllen = MAX_LINE_LENGTH*2+1;
  int  tokstarted = 0;
  int  done = 0;

  /* found a single char last time we want to return */
  if (tok->chartok != '\0') {
    *singlecharbuf = tok->chartok;
    tok->chartok = '\0';
    return singlecharbuf;
  }

  while (!done) {
    if (tok->eol) {
      int len;

      if (fgets(tok->currentline, MAX_LINE_LENGTH+1, tok->stream) == NULL) {
	/* try to get out the last bit */
	if (strlen(token) > 0)
	  return token;
	/* else nothing left */
	return NULL;
      }
      len = strlen(tok->currentline);
      /* this will start losing stuff at the end of the line
	 if a token is too long (more than 2x the max line length)
         or a lot of long tokens were read in a row just error out */
      if ((fillstart + len) - tok->tokenbuffer > MAX_LINE_LENGTH*2+1)
	return NULL;
      strncpy(fillstart, tok->currentline, filllen);
      tok->ptr = tok->tokenbuffer;
      tok->eol = 0;
    }
    
    if (!tokstarted) {
      /* skip through initial delimiters */
      for (; strchr(tok->delimiters, *tok->ptr) != NULL; tok->ptr++) {
	if (*tok->ptr == '\0') {
	  tok->eol = 1;
	  break;
	}
      }
      if (tok->eol)
	continue;
      token = tok->ptr;
      tokstarted = 1;
    }
    /* read until next delimiter is found, or end of line */
    for (;*tok->ptr != '\0' && 
	   strchr(tok->chartokens, *tok->ptr) == NULL &&
	   strchr(tok->delimiters, *tok->ptr) == NULL; tok->ptr++)
      ;
    /* didn't find a delimiter, need to continue the token
       on the next read */
    if (*tok->ptr == '\0') {
      memmove(tok->tokenbuffer, token, strlen(token)+1);
      token = tok->tokenbuffer;
      tok->ptr = tok->tokenbuffer + strlen(token);
      fillstart = tok->ptr;
      filllen   = tok->tokenbuffer + (MAX_LINE_LENGTH*2+1) - tok->ptr;
      tok->eol = 1;
    } else {
      /* found a single char we want to return (possible next time) */
      if (strchr(tok->chartokens, *tok->ptr) != NULL) {
	/* is this token, need to return it */
	if (tok->ptr == token) {
	  *singlecharbuf = *tok->ptr;
	  token = singlecharbuf;
	} else
	  /* otherwise, just save it for next time */
	  tok->chartok = *tok->ptr;
      }

      *tok->ptr = '\0';
      tok->ptr++;

      done = 1;
    }
  }

  return token;
}
