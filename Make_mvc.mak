OPTIMIZE = -O2 -favor:INTEL64 -Qpar

CC = cl

CFLAGS = -MD -utf-8 -std:c11

CPPFLAGS = -DWINDOWS

PROGRAM = unitedat

EXE = .exe

O = .obj

# https://docs.microsoft.com/en-us/cpp/c-runtime-library/link-options?view=msvc-160
$(PROGRAM)$(EXE): $(PROGRAM)$(O)
	link $** \
		Shell32.lib \
		binmode.obj \
		noenv.obj \
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
