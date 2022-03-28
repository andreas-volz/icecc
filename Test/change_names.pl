#!/usr/bin/perl

# changes the old function/global names to new ones
# which conform better to a standard convention

%NAMES = (
	  dat_format => DatType,
	  dat_fmt_entry => DatFmtEnt,
	  dat => Dat,
	  
	  dat_fmt => DatFmt,

	  freeBucket => hashbucket_free,
	  nextBucketHashEnum => hashenum_next_hashbucket,
	  

	  free_dat => dat_free,
	  create_dat => dat_new,
	  save_dat => dat_save,
	  get_dat_value_by_num => dat_get_value,
	  get_dat_value_by_name => dat_get_value_by_varname,
	  get_dat_var_index => dat_indexof_varname,
	  get_dat_var_name => dat_nameof_varno,
	  get_dat_var_size => dat_sizeof_varno,
	  get_dat_var_num => dat_numberof_varno,
	  get_dat_var_offset => dat_offsetof_varno,
	  valid_dat_var => dat_isvalid_varno,
	  valid_dat_entry => dat_isvalid_entryno,
	  set_dat_value => dat_set_value,
	  
	  HashBucket => HashBucket,
	  HashTable => HashTable,
	  HashEnum => HashEnum,
	  
	  createHashTable => hashtable_new,
	  insertData => hashtable_insert,
	  findData => hashtable_find,
	  findAndRemoveData => hashtable_remove,
	  emptyTable => hashtable_freecontents,
	  freeEntireTable => hashtable_freeall,
	  freeOnlyTable => hashtable_free,
	  
	  createHashEnum => hashenum_create,
	  nextHashEnum => hashenum_next,
	  nextKeyHashEnum => hashenum_next_key,
	  nextAllHashEnum => hashenum_next_pair,
	  
	  make_iscript_id_hash => iscript_id_hash_new,
	  free_iscript_id_hash => iscript_id_hash_free,
	  
	  instr_fmt => IsInstrFmt,
	  get_next_iscarg => isinstr_get_next_arg,
	  get_isctype => isinstr_get_type,
	  write_next_iscarg => isinstr_write_next_arg,
	  
	  anim => IsAnim,
	  symtbl_entry => IsSymTblEnt,
	  header_info => IsHeader,
	  byte_code => IsInstr,
	  imagescript => Iscript,
	  bytecode_enum => IsInstrEnum,
	  iscid_enum => IsIdEnum,
	  
	  create_imagescript => iscript_new,
	  save_imagescript => iscript_save,
	  free_imagescript => iscript_free,
	  set_iscript_version => iscript_set_version,
	  valid_anim => iscript_isvalid_anim,
	  num_anims => iscript_numberof_anims,
	  make_bytecode_enum => isinstrenum_create,
	  rewind_bytecode_enum => isinstrenum_rewind,
	  next_bytecode_enum => isinstrenum_next,
	  get_jump => isinstr_get_jump,
	  get_st_entry => isinstr_get_label,
	  make_bytecode_enum_from_st_entry => isinstrenum_create_from_symtblent,
	  
	  get_symtbl_anim_info => symtblent_get_anim_list,
	  get_symtbl_id => symtblent_get_id,
	  make_iscid_enum => isidenum_create,
	  next_iscid_enum => isidenum_next,
	  
	  createObjList => objlist_new,
	  emptyObjList => objlist_isempty,
	  addToObjList => objlist_insert,
	  freeOnlyObjList => objlist_free,
	  freeEntireObjList => objlist_freeall,
	  createObjListEnum => objlistenum_create,
	  emptyObjListEnum => objlistenum_isempty,
	  nextObjListEnum => objlistenum_next,
	  
	  makeEmptyQueue => queue_new,
	  insertQ => queue_insert,
	  removeQ => queue_remove,
	  emptyQ => queue_isempty,
	  freeQ => queue_free,
	  makeEmptyObjQueue => objqueue_new,
	  insertObjQ => objqueue_insert,
	  removeObjQ => objqueue_remove,
	  emptyObjQ => objqueue_isempty,
	  freeEntireObjQ => objqueue_freeall,
	  freeOnlyObjQ => objqueue_free,
	  
	  new_mfile => mcreat,
	  save_mfile => msave,
	  resize_mfile => mresize,
	  
	  get_sc_error => sc_get_err,
	  get_sc_error_prev => sc_get_err_prev,
	  get_sc_old => sc_get_err_old,
	  
	  err_log => sc_err_log,
	  err_fatal => sc_err_fatal,
	  err_warn => sc_err_warn,
	  
	  tbl => Tbl,
	  free_tbl => tbl_free,
	  create_tbl => tbl_new,
	  save_tbl => tbl_save,
	  get_tbl_string => tbl_get_string,
	  get_tbl_size => tbl_get_size,
	  set_tbl_string => tbl_set_string,
	  insert_tbl_string => tbl_insert_string,
);

foreach $key (keys %NAMES) {
    #print STDOUT "$key -> $NAMES{$key}\n";
    next if /#include/;
    s/\b$key\b/$NAMES{$key}/g;
}
