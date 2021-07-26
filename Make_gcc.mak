OPTIMIZE = -O2 -march=native

CC = gcc

CFLAGS = -std=c11 -Wall -pedantic

CPPFLAGS =

PROGRAM = unitedat

EXE =

O = .o

$(PROGRAM)$(EXE): $(PROGRAM)$(O)
	gcc $< -o $@

$(PROGRAM)$(O): $(PROGRAM).c
	$(CC) -c $(CFLAGS) $(OPTIMIZE) $(CPPFLAGS) $<

INSTALL = cp

RM_F = rm -f

install: $(PROGRAM)$(EXE) test
	$(INSTALL) $(PROGRAM)$(EXE) $(SITE_DIR)\bin

test: $(PROGRAM)$(EXE)
	prove -v t\test.pl

clean:
	$(RM_F) $(PROGRAM)$(EXE)
	$(RM_F) $(PROGRAM)$(O)
