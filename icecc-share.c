/*
  IceCC. Shared routines between the de/compiler.
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

/* $Id: icecc-share.c,v 1.9 2001/03/28 09:18:50 jp Exp $ */

#include <string.h>
#include <ctype.h>
#include "scdef.h"
#include "icecc-share.h"
#include "tokenizer.h"

char install_path[200]     = INSTALL_PATH;
char config_file_path[250] = INSTALL_PATH DIR_SEPARATOR CONFIG_FILE;

/* paths to resource files */
char path_iscript_bin[300]  = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR ISCRIPT_BIN_FILE_PATH;
char path_images_dat[300]   = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR IMAGES_DAT_FILE_PATH;
char path_images_lst[300]   = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR IMAGES_LST_FILE_PATH;
char path_images_tbl[300]   = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR IMAGES_TBL_FILE_PATH;
char path_sprites_dat[300]  = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR SPRITES_DAT_FILE_PATH;
char path_flingy_dat[300]   = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR FLINGY_DAT_FILE_PATH;
char path_units_dat[300]    =
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR UNITS_DAT_FILE_PATH;
char path_sfxdata_dat[300]  = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR SFXDATA_TBL_FILE_PATH;
char path_sfxdata_tbl[300]  = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR SFXDATA_TBL_FILE_PATH;
char path_weapons_dat[300]  = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR WEAPONS_DAT_FILE_PATH;
char path_stat_txt_tbl[300] = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR STAT_TXT_TBL_FILE_PATH;
char path_iscript_lst[300]  = 
INSTALL_PATH DIR_SEPARATOR DATADIR DIR_SEPARATOR ISCRIPT_LST_FILE_PATH;

static int pow_int(int x, int y);

#ifndef CLASSIC_ICECC
char *table_animno_to_name[] = {
  "Init",
  "Death",
  "GndAttkInit",
  "AirAttkInit",
  "Unused1",
  "GndAttkRpt",
  "AirAttkRpt",
  "CastSpell",
  "GndAttkToIdle",
  "AirAttkToIdle",
  "Unused2",
  "Walking",
  "WalkingToIdle",
  "SpecialState1", /* Some sort of category of special animations, in some cases an in-transit animation, sometimes used for special orders, sometimes having to do with the animation when something finishes morphing, or the first stage of a construction animation */
  "SpecialState2", /* Some sort of category of special animations, in some cases a burrowed animation, sometimes used for special orders, sometimes having to do with the animation when canceling a morph, or the second stage of a construction animation */
  "AlmostBuilt",
  "Built",
  "Landing",
  "LiftOff",
  "IsWorking",
  "WorkingToIdle",
  "WarpIn",
  "Unused3",
  "StarEditInit",
  "Disable",
  "Burrow",
  "UnBurrow",
  "Enable",
  NULL,
};
#else
char *table_animno_to_name[] = {
  "Init",
  "Death",
  "GndAttkInit",
  "AirAttkInit",
  "SpAbility1",
  "GndAttkRpt",
  "AirAttkRpt",
  "SpAbility2",
  "GndAttkToIdle",
  "AirAttkToIdle",
  "SpAbility3",
  "Walking",
  "Other",
  "BurrowInit",
  "ConstrctHarvst",
  "IsWorking",
  "Landing",
  "LiftOff",
  "Unknown18",
  "Unknown19",
  "Unknown20",
  "Unknown21",
  "Unknown22",
  "Unknown23",
  "Unknown24",
  "Burrow",
  "UnBurrow",
  "Unknown27",
  NULL,
};
#endif

/* ----- utility functions ----- */

void display_version() {
  printf("\nIceCC version %s, Copyright (C) 2000-2001 Jeffrey Pang\n"
	 "Various modifications, Copyright (C) 2006-2007 ShadowFlare\n"
	 "IceCC comes with ABSOLUTELY NO WARRANTY.\n"
	 "This is free software, and you are welcome to redistribute it\n"
	 "under certain conditions; see the file COPYING for details.\n\n",
	 VERSION);
  exit(0);
}

void set_config_file(char *name) {
  config_file_path[249] = '\0';
  strncpy(config_file_path, name, 249);
}

void set_install_path(char *name) {
  install_path[199] = '\0';
  strncpy(install_path, name, 199);
}

void load_config_file() {
  FILE *configfile = fopen(config_file_path, "r");
  Tokenizer *tok;
  char *next;
  char buf[256];
  int loadconfigdir = 1, installdir = 1;

  buf[255] = '\0';

  if (configfile == NULL) {
    /* try local directory */
    if ((configfile = fopen(strip_path(config_file_path), "r")) == NULL) {
      sc_err_warn("could not open configuration file %s; using defaults",
		  config_file_path);
      return;
    }
  }

  tok = tokenizer_new(configfile, "\n", "");
  
  while ((next = tokenizer_next(tok)) != NULL) {
    char *var, *val;

    strncpy(buf, next, 255);
    var = strtok(next, "=");
    val = strtok(NULL, "=");
    if (var != NULL && val != NULL) {
      if (streq(var, "INSTALLDIR")) {
	set_install_path(val);
	if (loadconfigdir) {
	  sprintf(buf, "%.200s" DIR_SEPARATOR "%.54s", install_path, DATADIR);
	  set_config_dir(buf);
	  installdir = 0;
	}
      } else if (streq(var, "CONFIGDIR")) {
	set_config_dir(val);
	loadconfigdir = 0;
      }
    }
  }

  if (loadconfigdir && installdir)
    sc_err_warn("did not find INSTALLDIR or CONFIGDIR in config file %s; using defaults", 
		config_file_path);

  tokenizer_free(tok);
  fclose(configfile);
}

/* strips the function name out of a logged error message */
char *strip_function_name(const char *errmsg) {
  char *tmp = (char *)errmsg;

  while (*tmp != ':' && *tmp != '\0') {
    if (!isalnum(*tmp) && *tmp != '_')
      return (char *)errmsg;
    tmp++;
  }

  if (*tmp == ':') {
    if (strchr(" \t", *++tmp))
      ++tmp;
    return tmp;
  } else
    return (char *)errmsg;
}

/* puts a comma, semicolon, space, or tab seperated list of 
   numbers into a queue, displays a warning if not a number */
void commalist_to_queue(char *list, Queue *q) {
  char *next = strtok(list, ",; \t");
  char *e;
  long val;

  while (next != NULL) {
    val = strtol(next, &e, 0);
    if (*e != '\0')
      sc_err_warn("'%s' is not a valid number", next);
    else
      queue_insert(q, val);
    next = strtok(NULL, ",");
  }
}

void set_config_dir(char *configdir) {
  set_file_path(path_iscript_bin,  configdir, ISCRIPT_BIN_FILE_PATH);
  set_file_path(path_images_dat,   configdir, IMAGES_DAT_FILE_PATH);
  set_file_path(path_images_lst,   configdir, IMAGES_LST_FILE_PATH);
  set_file_path(path_images_tbl,   configdir, IMAGES_TBL_FILE_PATH);
  set_file_path(path_sprites_dat,  configdir, SPRITES_DAT_FILE_PATH);
  set_file_path(path_flingy_dat,   configdir, FLINGY_DAT_FILE_PATH);
  set_file_path(path_units_dat,    configdir, UNITS_DAT_FILE_PATH);
  set_file_path(path_sfxdata_dat,  configdir, SFXDATA_DAT_FILE_PATH);
  set_file_path(path_sfxdata_tbl,  configdir, SFXDATA_TBL_FILE_PATH);
  set_file_path(path_weapons_dat,  configdir, WEAPONS_DAT_FILE_PATH);
  set_file_path(path_stat_txt_tbl, configdir, STAT_TXT_TBL_FILE_PATH);
  set_file_path(path_iscript_lst,  configdir, ISCRIPT_LST_FILE_PATH);
}

void set_file_path(char *ptr, char *prefix, char *suffix) {
  char *buffer = malloc((strlen(prefix)+strlen(suffix)+2)*sizeof(char));
  sprintf(buffer, "%s" DIR_SEPARATOR "%s", prefix, suffix);
  strncpy(ptr, buffer, 299);
  free(buffer);
}

/* ----- hashing functions ----- */

int string_eq(void *a, void *b) {
  return !strcmp(a,b);
}

static int pow_int(int x, int y) {
  int result = 1;
  for (; y>0; y--)
    result *= x;
  return result;
}

int string_hash(void *a) {
  int len = strlen(a);
  int i, result = 0;

  for (i=0; i<len; i++)
    result += ((char *)a)[i]*pow_int(37,i);

  return abs(result);
}

int pointer_hash_fn(void *ptr) {
  return abs((int)ptr);
}

int pointer_eq_fn(void *p1, void *p2) {
  return p1 == p2;
}
