# t2p: build a PDF file out of one or more TIFF Class F Group 4 files
# Makefile
# $Id: Makefile,v 1.11 2003/01/21 10:53:48 eric Exp $
# Copyright 2001 Eric Smith <eric@brouhaha.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.  Note that permission is
# not granted to redistribute this program under the terms of any
# other version of the General Public License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111  USA


CFLAGS = -Wall -g -I/usr/local/include/panda
LDLIBS = -g -ltiff -lm -L/usr/local/lib/panda -lpanda -lpng
YACC = bison
YFLAGS = -d -v

# -----------------------------------------------------------------------------
# You shouldn't have to change anything below this point, but if you do please
# let me know why so I can improve this Makefile.
# -----------------------------------------------------------------------------

VERSION = 0.1

PACKAGE = t2p

TARGETS = t2p bitblt_test

CSRCS = t2p.c semantics.c bitblt.c bitblt_test.c
OSRCS = scanner.l parser.y
HDRS = t2p.h semantics.h bitblt.h 
MISC = COPYING Makefile

DISTFILES = $(MISC) $(HDRS) $(CSRCS) $(OSRCS)
DISTNAME = $(PACKAGE)-$(VERSION)


AUTO_CSRCS = scanner.c parser.tab.c
AUTO_HDRS = parser.tab.h
AUTO_MISC = parser.output


all: $(TARGETS)


dist: $(DISTFILES)
	-rm -rf $(DISTNAME)
	mkdir $(DISTNAME)
	for f in $(DISTFILES); do ln $$f $(DISTNAME)/$$f; done
	tar --gzip -chf $(DISTNAME).tar.gz $(DISTNAME)
	-rm -rf $(DISTNAME)


t2p: t2p.o scanner.o semantics.o parser.tab.o bitblt.o

bitblt_test: bitblt_test.o bitblt.o


clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_SRCS) $(AUTO_HDRS) $(AUTO_MISC)

very_clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_SRCS) $(AUTO_HDRS) $(AUTO_MISC) \
		*~ *.pdf

wc:
	wc -l $(HDRS) $(CSRCS) $(OSRCS) $(MISC)

ls-lt:
	ls -lt $(HDRS) $(CSRCS) $(OSRCS) $(MISC)


%.tab.c %.tab.h %.output: %.y
	$(YACC) $(YFLAGS) $<

# %.c: %.l
# 	$(LEX) $(LFLAGS) $<


ALL_CSRCS = $(CSRCS) $(AUTO_CSRCS)

DEPENDS = $(ALL_CSRCS:.c=.d)

%.d: %.c
	$(CC) -M -MG $(CFLAGS) $< | sed -e 's@ /[^ ]*@@g' -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

include $(DEPENDS)
