# generic Makefile targets for icecc

#####
# Stuff for the cli binaries
OBJS1 = icedc.o iscript.c iscript-instr.o icecc-share.o tokenizer.o \
	icecc-utils.o scdef.o dat.o tbl.o queue.o hashtable.o list.o
OBJS2 = icecc.o tokenizer.o iscript.c iscript-instr.o icecc-share.o \
	icecc-utils.o scdef.o dat.o tbl.o queue.o hashtable.o list.o

#####
# IceCCUI - Stuff for the GUI
JAVAPACK = com$(DS)campaigncreations$(DS)IceCCUI
JAVASRC = $(JAVAPACK)$(DS)IceCCUI.java \
	  $(JAVAPACK)$(DS)CompilerPane.java \
	  $(JAVAPACK)$(DS)DecompilerPane.java \
	  $(JAVAPACK)$(DS)PreferencesDialog.java \
	  $(JAVAPACK)$(DS)IceCCUtil.java

#####
# Data files
HELP = $(README) doc$(DS)manual.pdf COPYING Examples
DATADIR = data
DATA = $(DATADIR)$(DS)arr data$(DS)rez $(DATADIR)$(DS)scripts $(DATADIR)$(DS)images.lst \
       $(DATADIR)$(DS)arr$(DS)flingy.dat $(DATADIR)$(DS)arr$(DS)flingy.tbl \
       $(DATADIR)$(DS)arr$(DS)images.dat $(DATADIR)$(DS)arr$(DS)sfxdata.dat \
       $(DATADIR)$(DS)arr$(DS)sfxdata.tbl $(DATADIR)$(DS)arr$(DS)sprites.dat \
       $(DATADIR)$(DS)arr$(DS)units.dat $(DATADIR)$(DS)arr$(DS)weapons.dat \
       $(DATADIR)$(DS)rez$(DS)stat_txt.tbl $(DATADIR)$(DS)scripts$(DS)iscript.bin \
       $(DATADIR)$(DS)sprites.lst $(DATADIR)$(DS)flingy.lst $(DATADIR)$(DS)units.lst \
       $(DATADIR)$(DS)iscript.lst

#####
# Other stuff
ifndef DISTDIR
DISTDIR = IceCC-$(VERSION)
endif
CONFIGFILE = icecc.ini
UICONFIGFILE = iceccui.ini

#####
# main target

all: $(EXEC1) $(EXEC2) iceccui $(UIEXEC)

#####
# package

dist: $(DIST)

$(DIST): $(EXEC1) $(EXEC2) $(JARFILE) $(UIEXEC) $(DATA) $(HELP)
	$(REMOVE) $(REMOVE_FLAGS)  $(DISTDIR)
	$(MKDIR) $(MKDIR_FLAGS)  $(DISTDIR)
	$(COPY) $(COPY_FLAGS) $(DIST_FILE) $(EXEC1) $(EXEC2) $(JARFILE) \
		$(UIEXEC) $(DATADIR) $(HELP)  $(DISTDIR)
	$(COPY) $(COPY_FLAGS) $(CONFIG)  $(DISTDIR)$(DS)$(CONFIGFILE)
	$(COPY) $(COPY_FLAGS) $(UICONFIG)  $(DISTDIR)$(DS)$(UICONFIGFILE)
	bin/remove-cvs $(DISTDIR)
	$(ARCHIVE) $(ARCHIVE_FLAGS) $(DIST)  $(DISTDIR)
	$(REMOVE) $(REMOVE_FLAGS)  $(DISTDIR)

#####
# c

$(EXEC1): $(OBJS1)
	$(CC) -o $(EXEC1) $(LDFLAGS) $(OBJS1)

$(EXEC2): $(OBJS2)
	$(CC) -o $(EXEC2) $(LDFLAGS) $(OBJS2)

*.o: *.c *.h

#####
# java

iceccui: $(JARFILE)

$(JARFILE): iceccui-classes IceCCUI.mf
	$(JAR) cfm $(JARFILE) IceCCUI.mf $(JAVAPACK)$(DS)*.class

iceccui-classes: $(JAVASRC)
	$(JAVAC) $(JFLAGS) -d . $(JAVASRC)

#####
# util

clean:
	$(REMOVE) $(REMOVE_FLAGS) *.o $(EXEC1) $(EXEC2) \
	$(JAVAPACK)$(DS)*.class $(JARFILE) $(DISTDIR)   \
	IceCC.app/Contents/Resources/Java/IceCCUI.jar
