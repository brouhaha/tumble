# tiff2pdf: build a PDF file out of one or more TIFF Class F Group 4 files
# Makefile
# $Id: Makefile,v 1.5 2001/12/29 10:59:17 eric Exp $
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

SRCS = bitblt.c bitblt_test.c tiff2pdf.c
HDRS = type.h bitblt.h tiff2pdf.h
MISC = Makefile scanner.l parser.y

tiff2pdf: tiff2pdf.o scanner.o parser.tab.o

bitblt_test: bitblt_test.o bitblt.o


%.tab.c %.tab.h: %.y
	$(YACC) $(YFLAGS) $<

# %.c: %.l
# 	$(LEX) $(LFLAGS) $<

