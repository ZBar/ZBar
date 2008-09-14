# NOTE: This Makefile requires GNU make
# Location to put the targets.
TARGETBINDIR = .
TARGETLIBDIR = .
# Name of the targets
LIBQRCODE_TARGET = libqrcode.a
QRREADER_TARGET = qrreader
# The compiler to use
CC = gcc
# The command to use to generate dependency information
MAKEDEPEND = gcc -MM
#MAKEDEPEND = makedepend -f- -Y --
# The location of include files.
CINCLUDE = `pkg-config --cflags libpng`
# These are gcc-only, but not actually critical.
CFLAGS = -Wall -Wno-parentheses -Wpointer-arith -Wsign-compare
CFLAGS += -std=c89 -pedantic
CFLAGS += -pipe -fvisibility=hidden

# Extra compilation flags.
# You may get speed increases by including flags such as -O2 or -O3 or
#  -ffast-math, or additional flags, depending on your system and compiler.
CFLAGS += -mfpu=vfp -mfloat-abi=softfp -mcpu=arm1136j-s -mtune=arm1136j-s
CFLAGS += -Os
#CFLAGS += -ftree-vectorize -ftree-vectorizer-verbose=0
# The -g flag will generally include debugging information.
#CFLAGS += -g
LIBS = `pkg-config --libs libpng`

# ANYTHING BELOW THIS LINE PROBABLY DOES NOT NEED EDITING
CINCLUDE := -I../include ${CINCLUDE}
LIBSRCDIR = ../src
BINSRCDIR = ../src
WORKDIR = objs

# C source file lists
LIBQRCODE_CSOURCES = \
bch15_5.c \
binarize.c \
qrdec.c \
image.c \
isaac.c \
rs.c \
util.c \

LIBQRCODE_CHEADERS = \
bch15_5.h \
binarize.h \
image.h \
isaac.h \
qrcode.h \
rs.h \
util.h \

QRREADER_CSOURCES = reader.c

# Create object file list.
LIBQRCODE_OBJ:= ${WORKDIR}/qrdec.o
LIBQRCODE_ASM:= ${LIBQRCODE_OBJ:%.o=%.s}
LIBQRCODE_DEP:= ${LIBQRCODE_OBJ:%.o=%.d}
QRREADER_OBJS:= ${QRREADER_CSOURCES:%.c=${WORKDIR}/%.o}
ALL_OBJS:= ${LIBQRCODE_OBJ} ${QRREADER_OBJS}
# Create the dependency file list
ALL_DEPS:= ${ALL_OBJS:%.o=%.d}
# Prepend source path to file names.
LIBQRCODE_CSOURCES:= ${LIBQRCODE_CSOURCES:%=${LIBSRCDIR}/%}
LIBQRCODE_CHEADERS:= ${LIBQRCODE_CHEADERS:%=${LIBSRCDIR}/%}
QRREADER_CSOURCES:= ${QRREADER_CSOURCES:%=${BINSRCDIR}/%}
ALL_CSOURCES:= ${LIBQRCODE_CSOURCES} ${QRREADER_CSOURCES}
# Prepand target path to file names.
LIBQRCODE_TARGET:= ${TARGETLIBDIR}/${LIBQRCODE_TARGET}
QRREADER_TARGET:= ${TARGETBINDIR}/${QRREADER_TARGET}
ALL_TARGETS:= ${LIBQRCODE_TARGET} ${QRREADER_TARGET}

# Targets:
# Everything (default)
all: ${ALL_TARGETS}

# libqrcode
${LIBQRCODE_DEP}: ${LIBQRCODE_CSOURCES}
${LIBQRCODE_ASM}: ${LIBQRCODE_CSOURCES}
${LIBQRCODE_OBJ}: ${LIBQRCODE_CSOURCES}
${LIBQRCODE_TARGET}: ${LIBQRCODE_OBJ}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ $<

# qrreader
${QRREADER_TARGET}: ${QRREADER_OBJS} ${LIBQRCODE_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${QRREADER_OBJS} ${LIBS} ${LIBQRCODE_TARGET}

# Assembly listing
ALL_ASM := ${ALL_OBJS:%.o=%.s}
asm: ${ALL_ASM}

# Remove all targets.
clean:
	-rm ${ALL_ASM} ${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS}
	-rmdir ${WORKDIR}

# Make everything depend on changes in the Makefile
${ALL_ASM} ${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS} : Makefile

# Specify which targets are phony for GNU make
.PHONY : all clean

# Rules
# These are set up to compile each library as a single unit, to allow gcc to do
#  Inter Module Analysis (IMA) when optimizing.
${WORKDIR}/%.d: ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} $(filter %.c,$^) \
         -MT ${@:%.d=%.o} > $@
	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} $(filter %.c,$^) \
         -MT ${@:%.d=%.s} >> $@
	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} $(filter %.c,$^) \
         -MT $@ >> $@
${WORKDIR}/%.s: ${LIBSRCDIR}/%.c
	${CC} ${CINCLUDE} ${CFLAGS} -S -o $@ -combine $(filter %.c,$^)
${WORKDIR}/%.o: ${LIBSRCDIR}/%.c
	${CC} ${CINCLUDE} ${CFLAGS} -c -o $@ -combine $(filter %.c,$^)

#${WORKDIR}/%.d : ${BINSRCDIR}/%.c
#	mkdir -p ${dir $@}
#	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} $< -MT ${@:%.d=%.o} > $@
#${WORKDIR}/%.s : ${BINSRCDIR}/%.c ${WORKDIR}/%.o
#	mkdir -p ${dir $@}
#	${CC} ${CINCLUDE} ${CFLAGS} -S -o $@ $<
#${WORKDIR}/%.o : ${BINSRCDIR}/%.c
#	mkdir -p ${dir $@}
#	${CC} ${CINCLUDE} ${CFLAGS} -c -o $@ $<

# Include header file dependencies
include ${ALL_DEPS}
