# t2p: build a PDF file out of one or more TIFF Class F Group 4 files
# Makefile
# $Id: Makefile,v 1.10 2002/08/25 22:03:55 eric Exp $
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

SRCS = bitblt.c bitblt_test.c t2p.c semantics.c
HDRS = bitblt.h t2p.h semantics.h
MISC = Makefile scanner.l parser.y

TARGETS = t2p bitblt_test

AUTO_SRCS = scanner.c parser.tab.c
AUTO_HDRS = parser.tab.h
AUTO_MISC = parser.output

t2p: t2p.o scanner.o semantics.o parser.tab.o bitblt.o

bitblt_test: bitblt_test.o bitblt.o


clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_SRCS) $(AUTO_HDRS) $(AUTO_MISC)

very_clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_SRCS) $(AUTO_HDRS) $(AUTO_MISC) \
		*~ *.pdf

wc:
	wc -l $(SRCS) $(HDRS) $(MISC)

ls-lt:
	ls -lt $(SRCS) $(HDRS) $(MISC)


%.tab.c %.tab.h %.output: %.y
	$(YACC) $(YFLAGS) $<

# %.c: %.l
# 	$(LEX) $(LFLAGS) $<


ALL_SRCS = $(SRCS) $(AUTO_SRCS)

DEPENDS = $(ALL_SRCS:.c=.d)

%.d: %.c
	$(CC) -M -MG $(CFLAGS) $< | sed -e 's@ /[^ ]*@@g' -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

include $(DEPENDS)
