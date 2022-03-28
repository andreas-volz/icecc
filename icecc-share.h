/* icecc-share.h */

#ifndef _ICECC_SHARE_H_
#define _ICECC_SHARE_H_

#if defined(WIN32)
#define INSTALL_PATH "."
#elif defined(MACOS)
#define INSTALL_PATH "."
#else
#define INSTALL_PATH "."
#endif

#define VERSION         "1.3"
#define CONFIG_FILE     "icecc.ini"
#define DATADIR         "data"

/* installation path */
extern char install_path[];
extern char config_file_path[];

/* paths to resource files */
extern char path_iscript_bin[];
extern char path_images_dat[];
extern char path_images_lst[];
extern char path_images_tbl[];
extern char path_sprites_dat[];
extern char path_flingy_dat[];
extern char path_units_dat[];
extern char path_sfxdata_dat[];
extern char path_sfxdata_tbl[];
extern char path_weapons_dat[];
extern char path_stat_txt_tbl[];
extern char path_iscript_lst[];

#define HEADER_TAG_START ".headerstart"
#define HEADER_TAG_END   ".headerend"
#define ISCRIPT_ID_TAG   "IsId"
#define ANIM_TYPE_TAG    "Type"

/* these two must be one chars long */
#define LABEL_TERMINATOR ":"
#define COMMENT_START    "#"

/* string for labels which point to nothing
   (e.g., are NULL) */
#define EMPTY_LABEL_STRING  "[NONE]"

/* animno -> cannonical name */
#define animno_to_name(no) (table_animno_to_name[(no)])

extern void display_version();
extern char *strip_function_name(const char *errmsg);
extern void commalist_to_queue(char *list, Queue *q);
extern void load_config_file();
extern void set_install_path(char *name);
extern void set_config_file(char *configdir);
extern void set_config_dir(char *configdir);
extern void set_file_path(char *ptr, char *prefix, char *suffix);

extern char *table_animno_to_name[];
extern int string_eq(void *a, void *b);
extern int string_hash(void *a);
extern int pointer_hash_fn(void *ptr);
extern int pointer_eq_fn(void *p1, void *p2);

#endif
