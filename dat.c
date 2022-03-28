/*
  IceCC. These are handlers for *.Dat files.
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

/* $Id: dat.c,v 1.10 2001/11/17 22:21:12 jp Exp $ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "scdef.h"
#include "dat.h"

/* These are the format structures that are used for each
   type of Dat file. Alter the names as you see fit. The values
   for them were from the starcraft.exe file. There are some
   screwy ones in units.Dat (the ones that have 912 entries)
   but I dunno what to do with them yet */

static DatFmt dat_flingy = {
  7,
  {
    {2, 209, 0, "Sprite"},
    {4, 209, 0, "Speed"},
    {2, 209, 0, "TurningStyle"},
    {4, 209, 0, "Acceleration"},
    {1, 209, 0, "TurnRadius"},
    {1, 209, 0, "Unknown6"},
    {1, 209, 0, "MovementControl"}
  }
};
  
static DatFmt dat_images = {
  14,
  {
    { 4, 999, 0, "File"},
    { 1, 999, 0, "GFXTurns"},
    { 1, 999, 0, "ShadowTurns"},
    { 1, 999, 0, "Unknown4"},
    { 1, 999, 0, "Floats"},
    { 1, 999, 0, "PaletteType"},
    { 1, 999, 0, "PaletteSpecial"},
    { 4, 999, 0, "IscriptID"},
    { 4, 999, 0, "ShieldOverlay"},
    { 4, 999, 0, "Overlay2"},
    { 4, 999, 0, "Overlay3"},
    { 4, 999, 0, "Overlay4"},
    { 4, 999, 0, "Overlay5"},
    { 4, 999, 0, "Overlay6"},
  }
};

static DatFmt dat_mapdata = {
  1,
  {
    { 4, 65, 0, "File"},
  }
};

static DatFmt dat_orders = {
  19,
  {
    { 2, 189, 0, "VarName1"},
    { 1, 189, 0, "VarName2"},
    { 1, 189, 0, "VarName3"},
    { 1, 189, 0, "VarName4"},
    { 1, 189, 0, "VarName5"},
    { 1, 189, 0, "VarName6"},
    { 1, 189, 0, "VarName7"},
    { 1, 189, 0, "VarName8"},
    { 1, 189, 0, "VarName9"},
    { 1, 189, 0, "VarName10"},
    { 1, 189, 0, "VarName11"},
    { 1, 189, 0, "VarName12"},
    { 1, 189, 0, "VarName13"},
    { 1, 189, 0, "VarName14"},
    { 1, 189, 0, "VarName15"},
    { 1, 189, 0, "VarName16"},
    { 2, 189, 0, "VarName17"},
    { 2, 189, 0, "VarName18"},
    { 1, 189, 0, "VarName19"},
  }
};

static DatFmt dat_sfxdata = {
  5,
  {
    { 4, 1144, 0, "File"},
    { 1, 1144, 0, "Unknown2"},
    { 1, 1144, 0, "Unknown3"},
    { 2, 1144, 0, "Unknown4"},
    { 1, 1144, 0, "Unknown5"},
  }
};

static DatFmt dat_sprites = {
  6,
  {
    { 2, 517, 0, "Image"},
    { 1, 387, 130, "HPBarLength"},
    { 1, 517, 0, "Unknown3"},
    { 1, 517, 0, "Unknown4"},
    { 1, 387, 130, "SelectionCircleImage"},
    { 1, 387, 130, "SelectionCircleVerticalOffset"}
  }
};

static DatFmt dat_techdata = {
  11,
  {
    { 2, 44, 0, "MineralCost"},
    { 2, 44, 0, "VespeneCost"},
    { 2, 44, 0, "ResarchTime"},
    { 2, 44, 0, "ManaRequired"},
    { 2, 44, 0, "Unknown5"},
    { 2, 44, 0, "Unknown6"},
    { 2, 44, 0, "Icon"},
    { 2, 44, 0, "Label"},
    { 1, 44, 0, "Race"},
    { 1, 44, 0, "ResearchedFlag"},
    { 1, 44, 0, "BroodWarFlag"},
  }
};

static DatFmt dat_units = {
  54,
  {
    { 1, 228, 0, "Graphic"},
    { 2, 228, 0, "Subunit1"},
    { 2, 228, 0, "Subunit2"},
    { 2, 96, 0, "Subunit3"},
    { 4, 228, 0, "ConstructionAnim"},
    { 1, 228, 0, "Unk0x08C4"},
    { 1, 228, 0, "ShieldEnable"},
    { 2, 228, 0, "ShieldAmount"},
    { 4, 228, 0, "HitPoints"},
    { 1, 228, 0, "AnimLevel"},
    { 1, 228, 0, "MovementType"},
    { 1, 228, 0, "StareditOrder/Sublabel"},
    { 1, 228, 0, "ActionCompAIIdle"},
    { 1, 228, 0, "ActionHumanAIIdle"},
    { 1, 228, 0, "ActionUnknown"},
    { 1, 228, 0, "ActionGroundAttack"},
    { 1, 228, 0, "ActionAirAttack"},
    { 1, 228, 0, "WeaponGround"},
    { 1, 228, 0, "MaxHitGround"},
    { 1, 228, 0, "WeaponAir"},
    { 1, 228, 0, "MaxHitAir"},
    { 1, 228, 0, "Unk0x1A94"},
    { 4, 228, 0, "SpecialAbilityFlags"},
    { 1, 228, 0, "SubunitAttackRange"},
    { 1, 228, 0, "SightRange"},
    { 1, 228, 0, "ArmorUpgradeGroup"},
    { 1, 228, 0, "Size"},
    { 1, 228, 0, "ArmorAmount"},
    { 1, 228, 0, "Unk0x237C"},
    { 2, 106, 0, "ReadySound"},
    { 2, 228, 0, "WhatSoundStart"},
    { 2, 228, 0, "WhatSoundEnd"},
    { 2, 106, 0, "PissSoundStart"},
    { 2, 106, 0, "PissSoundEnd"},
    { 2, 106, 0, "YesSoundStart"},
    { 2, 106, 0, "YesSoundEnd"},
    { 2, 456, 0, "Unk37_456Entries_"},
    { 2, 192, 0, "Unk38_192Entries_"},
    { 2, 912, 0, "Unk39_912Entries_"},
    { 2, 228, 0, "Portrait"},
    { 2, 228, 0, "MineralCost"},
    { 2, 228, 0, "VespeneCost"},
    { 2, 228, 0, "BuildTime"},
    { 2, 228, 0, "ResrictionFlags"},
    { 1, 228, 0, "StareditGroupFlags"},
    { 1, 228, 0, "Unk0x4210"},
    { 1, 228, 0, "FoodProduced"},
    { 1, 228, 0, "FoodCost"},
    { 1, 228, 0, "TransportOrBunkerSpace"},
    { 2, 228, 0, "BuildScore"},
    { 2, 228, 0, "DestroyScore"},
    { 2, 228, 0, "Unk0x4930"},
    { 1, 228, 0, "BroodwarUnitFlag"},
    { 2, 228, 0, "StareditAvailabilityFlags"},
  }
};

static DatFmt dat_upgrades = {
  12,
  {
    { 2, 61, 0, "MineralCostBase"},
    { 2, 61, 0, "MineralCostFactor"},
    { 2, 61, 0, "VespeneCostBase"},
    { 2, 61, 0, "VespeneCostFactor"},
    { 2, 61, 0, "ResearchTimeBase"},
    { 2, 61, 0, "ResearchTimeFactor"},
    { 2, 61, 0, "Unknown7"},
    { 2, 61, 0, "Icon"},
    { 2, 61, 0, "Label"},
    { 1, 61, 0, "Race"},
    { 1, 61, 0, "Repeat"},
    { 1, 61, 0, "BroodwarFlag"}
  }
};

static DatFmt dat_weapons = {
  24,
  {
    { 2, 130, 0, "StatusBarLabel"},
    { 4, 130, 0, "MissileSprite"},
    { 1, 130, 0, "SpecialAttack"},
    { 2, 130, 0, "AttackType"},
    { 4, 130, 0, "MinimumRange"},
    { 4, 130, 0, "MaximumRange"},
    { 1, 130, 0, "UpgradeGroup"},
    { 1, 130, 0, "WeaponType"},
    { 1, 130, 0, "WeaponBehavior"},
    { 1, 130, 0, "MissileType"},
    { 1, 130, 0, "ExplosionType"},
    { 2, 130, 0, "SplashValue1"},
    { 2, 130, 0, "SplashValue2"},
    { 2, 130, 0, "SplashValue3"},
    { 2, 130, 0, "DamageAmount"},
    { 2, 130, 0, "DamageBonus"},
    { 1, 130, 0, "CooldownDelay"},
    { 1, 130, 0, "DamageFactor"},
    { 1, 130, 0, "CoordinateX1"},
    { 1, 130, 0, "CoordinateX2"},
    { 1, 130, 0, "CoordinateY1"},
    { 1, 130, 0, "CoordinateY2"},
    { 2, 130, 0, "TargetingErrorMessage"},
    { 2, 130, 0, "Icon"},
  }
};

/* Some error checking is done, but not too much.
   Try to pass this a properly allocated Dat */
void dat_free(Dat *dat_st) {
  int i;
  if (dat_st == NULL)
    return;
  for (i=0; i < dat_st->fmt->numvars; ++i)
    if (dat_st->entries[i] != NULL)
      free(dat_st->entries[i]);
  free(dat_st->entries);
  free(dat_st);
}

/* Create an internal *.Dat file object with the filename
   dat_file_name using the format type should be one of the
   values in the DatType enum (see Dat.h) */
Dat *dat_new(const char *dat_file_name, DatType type) {
  MFILE   *dat_file = mopen(dat_file_name);
  Dat     *dat_st;
  DatFmt *fmt;
  int     i;

  /* could not open file */
  if (dat_file == NULL) {
    sc_err_log("dat_new: could not open %s", dat_file_name);
    return NULL;
  }
  
  /* allocate the Dat struct */
  dat_st = malloc(sizeof(dat_st));

  /* determine its type */
  switch (type) {
  case DAT_FLINGY:
    dat_st->fmt = fmt = &dat_flingy; break;
  case DAT_IMAGES:
    dat_st->fmt = fmt = &dat_images; break;
  case DAT_MAPDATA:
    dat_st->fmt = fmt = &dat_mapdata; break;
  case DAT_ORDERS:
    dat_st->fmt = fmt = &dat_orders; break;
  case DAT_SFXDATA:
    dat_st->fmt = fmt = &dat_sfxdata; break;
  case DAT_SPRITES:
    dat_st->fmt = fmt = &dat_sprites; break;
  case DAT_TECHDATA:
    dat_st->fmt = fmt = &dat_techdata; break;
  case DAT_UNITS:
    dat_st->fmt = fmt = &dat_units; break;
  case DAT_UPGRADES:
    dat_st->fmt = fmt = &dat_upgrades; break;
  case DAT_WEAPONS:
    dat_st->fmt = fmt = &dat_weapons; break;
  default:
    sc_err_log("dat_new: unrecognized DatType type %d", type);
    free(dat_st);
    return NULL;
  }

  /* zero out these just in case we get a read error and need to
     clean up */
  dat_st->entries = calloc(sizeof(void *), fmt->numvars);
  for (i=0; i < fmt->numvars; ++i) {
    /* allocate the entry array */
    dat_st->entries[i] = malloc(fmt->entries[i].num*fmt->entries[i].size);

    /* read all the values at once */
    if (mread(dat_st->entries[i], fmt->entries[i].size, 
	      fmt->entries[i].num, dat_file) != fmt->entries[i].num)
      {
	sc_err_log("dat_new: read of %s DAT failed", dat_file_name);
	mclose(dat_file);
	dat_free(dat_st);
	return NULL;
      }
  }

  mclose(dat_file);
  return dat_st;
}

/* Write the Dat object to the file file_name. Returns 0 on
   success, -1 on error. Note: Still need fix to make this function
   work on Big Endian machines. */
int dat_save(char *file_name, Dat *dat_st) {
  FILE *dat_file = fopen(file_name, "wb");
  int  i;

  /* could not open file */
  if (dat_file == NULL) {
    sc_err_log("dat_save: could not open %s for writing", file_name);
    return -1;
  }

  for (i=0; i < dat_st->fmt->numvars; ++i) {
    
    /* write all the values at once */
    if (fwrite(dat_st->entries[i], dat_st->fmt->entries[i].size, 
	       dat_st->fmt->entries[i].num, dat_file) != dat_st->fmt->entries[i].num)
      {
	sc_err_log("dat_save: write to %s failed", file_name);
	fclose(dat_file);
	remove(file_name);
	return -1;
      }
  }

  fclose(dat_file);
  return 0;
}

/* Finds the varible number var in entry number entry in the
   dat_st object and returns it as a uint32. Note: this function 
   will not function properly if entry or var are not valid values */
uint32 dat_get_value(const Dat *dat_st, unsigned entry, unsigned var) 
{
  switch(dat_st->fmt->entries[var].size) {
  case sizeof(uint32): 
    return ((uint32 *)dat_st->entries[var])[entry - dat_st->fmt->entries[var].offset];
  case sizeof(uint16):
    return ((uint16 *)dat_st->entries[var])[entry - dat_st->fmt->entries[var].offset];
  case sizeof(byte):
    return ((byte *)dat_st->entries[var])[entry - dat_st->fmt->entries[var].offset];
  default:
    return -1;
  }
}

/* Same as above, except find the variable in the Dat file by
   name (a char * string). If it doesn't find it, it will return
   (uint32)-1. (Note: this is a valid value, so this function is kinda
   depreciated since you don't really know if you have an error or not) */
uint32 dat_get_value_by_varname(const Dat *dat_st, unsigned entry, const char *name) {
  int i;
  for (i=0; i < dat_st->fmt->numvars; ++i)
    if (streq(dat_st->fmt->entries[i].name, name))
      return dat_get_value(dat_st, entry, i);
  return -1;
}

/* Returns the variable number that matches a variable named name.
   Returns (unsigned)-1 if it could not find it */
unsigned dat_indexof_varname(const Dat *dat_st, char *name) {
  int i;
  for (i=0; i < dat_st->fmt->numvars; ++i)
    if (streq(dat_st->fmt->entries[i].name, name))
      return i;
  return -1;
}

/* Return the varible name of variable number var in dat_st.
   DO NOT FREE THIS STRING. It is allocated in static memory,
   can not be changed or released. If you need to use it, use
   strcpy. Leave its memory alone */
char *dat_nameof_varno(const Dat *dat_st, unsigned var) {
  return dat_st->fmt->entries[var].name;
}

/* Return the size of each of varible var in dat_st */
size_t dat_sizeof_varno(const Dat *dat_st, unsigned var) {
  return dat_st->fmt->entries[var].size;
}

/* Return the number of the varibles var (i.e., the number of
   entries it applies to) in dat_st */
unsigned dat_numberof_varno(const Dat *dat_st, unsigned var) {
  return dat_st->fmt->entries[var].num;
}

/* Returns the offset or 'number of elements before' the first
   one in this variable set. So to get the first one is located
   at dat_offsetof_varno and the last one is at 
   dat_offsetof_varno + dat_numberof_varno */
unsigned dat_offsetof_varno(const Dat *dat_st, unsigned var) {
  return dat_st->fmt->entries[var].offset;
}

/* Return the number of different variables in dat_st */
size_t dat_numberof_vars(const Dat *dat_st) {
  return dat_st->fmt->numvars;
}

/* Returns the number of entries (for the variable with
   the greatest number of entries -- some variables might have
   less) You can use this number as a max to iterate over every
   dat entry, but make sure to check that it isvalid_entryno
   before using it */
size_t dat_numberof_entries(const Dat *dat_st) {
  int max = 0, i;
  for (i=0; i<dat_st->fmt->numvars; ++i)
    if (dat_st->fmt->entries[i].num +
	dat_st->fmt->entries[i].offset > max)
      max = dat_st->fmt->entries[i].num + 
	dat_st->fmt->entries[i].offset;
  return max;
}

/* Returns 1 if the variable number var is valid in
   dat_st, 0 otherwise */
int dat_isvalid_varno(const Dat *dat_st, unsigned var) {
  return var < dat_st->fmt->numvars;
}
/* Returns 1 if the varible var is a valid for entry number entry
   in dat_st */
int dat_isvalid_entryno(const Dat *dat_st, unsigned entry, unsigned var) {
  return (var < dat_st->fmt->numvars) && 
    (entry < dat_st->fmt->entries[var].offset + dat_st->fmt->entries[var].num) &&
    ((int)entry - (int)dat_st->fmt->entries[var].offset >= 0);
}

/* Sets the value of variable var in entry entry of the dat_st to the new
   value newval. Newval must be able to fit in the variable, or it will be
   truncated to fit */
void dat_set_value(Dat *dat_st, unsigned entry, unsigned var, uint32 newval) {
  switch(dat_st->fmt->entries[var].size) {
  case sizeof(uint32): 
    ((uint32 *)dat_st->entries[var])[entry - dat_st->fmt->entries[var].offset] = newval;
    return;
  case sizeof(uint16):
    ((uint16 *)dat_st->entries[var])[entry - dat_st->fmt->entries[var].offset] = newval;
    return;
  case sizeof(byte):
    ((byte *)dat_st->entries[var])[entry - dat_st->fmt->entries[var].offset] = newval;
    return;
  default:
    /* should not get here */
    return;
  }
}

/* *** DatEntLst routines *** */

/* helper -- removes the first newline in
   the first max chars of string and replaces
   it with a null character */
static void str_rmnewline(char *string, int max) {
  while (max-- > 0) {
    if (*string == '\n') {
      *string = '\0';
      break;
    }
    string++;
  }
}

/* Create a new "entry listing" structure from
   a file. This file should be a flat newline seperated
   enumeration of the dat file entry names from 0 to the end.
   Very little checking is done to verify this format, may change
   later. */
DatEntLst *datentlst_new(char *file_name) {
  FILE *file = fopen(file_name, "r");
  DatEntLst *result;
  char *array_buffer[MAX_DATENTLST_LINES];
  char string_buffer[MAX_DATENTLST_NAME];
  int i;

  if (file == NULL)
    return NULL;

  for (i=0; fgets(string_buffer, MAX_DATENTLST_NAME, file) != NULL; i++) {
    str_rmnewline(string_buffer, MAX_DATENTLST_NAME);
    array_buffer[i] = malloc(sizeof(char)*(strlen(string_buffer)+1));
    strcpy(array_buffer[i], string_buffer);
  }

  result = malloc(sizeof(DatEntLst));
  result->num = i;
  result->strings = malloc(sizeof(char *)*i);
  memcpy(result->strings, array_buffer, sizeof(char *)*i);

  fclose(file);

  return result;
}

void datentlst_free(DatEntLst *datlist) {
  int i;
  for (i=0; i < datlist->num; i++)
    free(datlist->strings[i]);
  free(datlist->strings);
  free(datlist);
}

/* get string number index from this datentlist,
   index should be within the bounds of the next
   function */
char *datentlst_get_string(DatEntLst *datlist, int index) {
  return datlist->strings[index];
}

/* return the number of strings in the datlist */
size_t datentlst_numberof_strings(DatEntLst *datlist) {
  return datlist->num;
}
