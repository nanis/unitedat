OPTIMIZE = -O2 -favor:INTEL64 -Qpar

CC = cl

CFLAGS = -MD -utf-8

CPPFLAGS = -DWINDOWS

PROGRAM = unitedat

EXE = .exe

O = .obj

$(PROGRAM)$(EXE): $(PROGRAM)$(O)
	link $** \
		Shell32.lib \
		wsetargv.obj \
		-Fe:$@ \
		-subsystem:console

$(PROGRAM)$(O): $(PROGRAM).c
	$(CC) -c $(CFLAGS) $(OPTIMIZE) $(CPPFLAGS) $**

INSTALL = copy /y

RM_F = del /q

install: $(PROGRAM)$(EXE) test
	$(INSTALL) $(PROGRAM)$(EXE) $(SITE_DIR)\bin

test: $(PROGRAM)$(EXE)
	prove -v t\test.pl

clean:
	$(RM_F) *.exe
	$(RM_F) *.obj