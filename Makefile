# Makefile for PPPDstat

VERSION=0.3.1

CC=gcc
CFLAGS=-O2
LIBS=-lm

## uncomment for debug
#CFLAGS+=-ggdb

TARGET=pppstat
INSTALL_DIR=/usr/local/bin
SOURCES=pppdstat.c
#DIST=${SOURCES} README Makefile ribbon.xpm VERSION COPYING AUTHORS
#DIST_DIR=${TARGET}-${VERSION}

$(TARGET): ${SOURCES}
	$(CC) $(LIBS) $(CFLAGS) -o $@ $<

install: ${TARGET}
	cp -f ${TARGET} ${INSTALL_DIR}

#clean:
#	rm -f $(TARGET) core

#dist: ${DIST}
#	@mkdir -p ${DIST_DIR} && \
#	cp ${DIST} ${DIST_DIR} && \
#	tar cvzf ${DIST_DIR}.tar.gz ${DIST_DIR} &>/dev/null && \
#	rm -rf ${DIST_DIR}

uninstall: ${TARGET}
	rm -f ${INSTALL_DIR}/${TARGET}
# EOF