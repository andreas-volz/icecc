#####
# generic Makefile for Unix

include Makefile.ver

# system options
DS = /

# cc options
INCLUDEDIR = /usr/include
CC = cc
CFLAGS = -g -I$(INCLUDEDIR) -Wall -ansi -pedantic
LDFLAGS = -g -I$(INCLUDEDIR) -Wall

# java options
JAVAC = javac
JAR = jar
JFLAGS = 

# utilities
REMOVE = rm
REMOVE_FLAGS = -rf
COPY = cp
COPY_FLAGS = -r 
MKDIR = mkdir
MKDIR_FLAGS =
ARCHIVE = tar
ARCHIVE_FLAGS = -czf # assumes gnu tar, which can gzip also

# distribution
DIST = icecc-$(VERSION)-linux.tar.gz

# executables
EXEC1 = icedc
EXEC2 = icecc
JARFILE = IceCCUI.jar
UIEXEC = IceCCUI

# confuration files
CONFIG = icecc.ini
UICONFIG = iceccui.ini

include Makefile.inc
