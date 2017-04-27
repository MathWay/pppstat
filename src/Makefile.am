# Process this file with automake to produce Makefile.in

bin_PROGRAMS = pppstat

pppstat_SOURCES = \
main.c main.h \
filework.c filework.h \
output.c output.h \
stat.c stat.h

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DPPPSTAT_CFG=\"$(sysconfdir)/pppstat.conf\" @DEFS@

# pppstat_LDADD = 