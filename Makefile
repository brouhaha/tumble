# tumble: build a PDF file from image files
# Makefile
# $Id: Makefile,v 1.41 2003/04/10 01:02:12 eric Exp $
# Copyright 2001, 2002, 2003 Eric Smith <eric@brouhaha.com>
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


# Conditionals:  uncomment the following defines as nessary.  Note that a
# "0" value is considered true by make, so to disable conditionals comment
# them out or set them to a null string.

#DEBUG=1
#EFENCE=1
#STATIC=1


CFLAGS = -Wall
LDFLAGS =
LDLIBS = -ltiff -ljpeg -lnetpbm -lz -lm

ifdef DEBUG
CFLAGS := $(CFLAGS) -g
LDFLAGS := $(LDFLAGS) -g
else
CFLAGS := $(CFLAGS) -O3
endif

ifdef EFENCE
LDLIBS := $(LDLIBS) -lefence -lpthread
endif

ifdef STATIC
LDLIBS := -Wl,-static $(LDLIBS)
endif


YACC = bison
YFLAGS = -d -v


# -----------------------------------------------------------------------------
# You shouldn't have to change anything below this point, but if you do please
# let me know why so I can improve this Makefile.
# -----------------------------------------------------------------------------

VERSION = 0.33

PACKAGE = tumble

TARGETS = tumble

CSRCS = tumble.c semantics.c \
	tumble_input.c tumble_tiff.c tumble_jpeg.c tumble_pbm.c \
	bitblt.c bitblt_table_gen.c bitblt_g4.c g4_table_gen.c \
	pdf.c pdf_util.c pdf_prim.c pdf_name_tree.c \
	pdf_bookmark.c pdf_page_label.c \
	pdf_text.c pdf_g4.c pdf_jpeg.c
OSRCS = scanner.l parser.y
HDRS = tumble.h tumble_input.h semantics.h bitblt.h bitblt_tables.h \
	pdf.h pdf_private.h pdf_util.h pdf_prim.h pdf_name_tree.h
MISC = COPYING README INSTALL Makefile

DISTFILES = $(MISC) $(HDRS) $(CSRCS) $(OSRCS)
DISTNAME = $(PACKAGE)-$(VERSION)

BIN_DISTFILES = COPYING README $(TARGETS)


AUTO_CSRCS = scanner.c parser.tab.c bitblt_tables.c g4_tables.c
AUTO_HDRS = parser.tab.h  bitblt_tables.h g4_tables.h
AUTO_MISC = parser.output


CFLAGS := $(CFLAGS) -DTUMBLE_VERSION=$(VERSION)


-include Maketest


all: $(TARGETS) $(TEST_TARGETS)


tumble: tumble.o semantics.o \
		tumble_input.o tumble_tiff.o tumble_jpeg.o tumble_pbm.o \
		scanner.o parser.tab.o \
		bitblt.o bitblt_g4.o bitblt_tables.o g4_tables.o \
		pdf.o pdf_util.o pdf_prim.o pdf_name_tree.o \
		pdf_bookmark.o pdf_page_label.o \
		pdf_text.o pdf_g4.o pdf_jpeg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
ifndef DEBUG
	strip $@
endif


bitblt_tables.h: bitblt_table_gen
	./bitblt_table_gen -h >bitblt_tables.h

bitblt_tables.c: bitblt_table_gen
	./bitblt_table_gen -c >bitblt_tables.c

bitblt_table_gen: bitblt_table_gen.o

g4_tables.h: g4_table_gen
	./g4_table_gen -h >g4_tables.h

g4_tables.c: g4_table_gen
	./g4_table_gen -c >g4_tables.c

g4_table_gen: g4_table_gen.o


dist: $(DISTFILES)
	-rm -rf $(DISTNAME)
	mkdir $(DISTNAME)
	for f in $(DISTFILES); do ln $$f $(DISTNAME)/$$f; done
	tar --gzip -chf $(DISTNAME).tar.gz $(DISTNAME)
	-rm -rf $(DISTNAME)


rh-rel := $(shell sed 's/^Red Hat Linux release \([0-9][0-9]*\.[0-9][0-9]*\) (.*)/\1/' </etc/redhat-release)

bin-dist-rh: $(BIN_DISTFILES) /etc/redhat-release
	tar --gzip -chf $(DISTNAME)-rh${rh-rel}.tar.gz $(BIN_DISTFILES)


clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_CSRCS) $(AUTO_HDRS) $(AUTO_MISC)

very_clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_CSRCS) $(AUTO_HDRS) $(AUTO_MISC) \
		*~ *.pdf

wc:
	wc -l $(HDRS) $(CSRCS) $(OSRCS) $(MISC)

ls-lt:
	ls -lt $(HDRS) $(CSRCS) $(OSRCS) $(MISC)


%.tab.c %.tab.h %.output: %.y
	$(YACC) $(YFLAGS) $<


ALL_CSRCS = $(CSRCS) $(AUTO_CSRCS) $(TEST_CSRCS)

DEPENDS = $(ALL_CSRCS:.c=.d)

%.d: %.c
	$(CC) -M -MG $(CFLAGS) $< | sed -e 's@ /[^ ]*@@g' -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

include $(DEPENDS)
