# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dwm.c util.c
OBJ = ${SRC:.c=.o}

all: options dwm dwm-msg

options:
	@echo dwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

dwm-msg: dwm-msg.o
	${CC} -o $@ $< ${LDFLAGS}

clean:
	rm -f config.h dwm dwm-msg ${OBJ} dwm-${VERSION}.tar.gz drw.o dwm.o util.o *.orig *.rej

dist: clean
	mkdir -p dwm-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
		dwm.1 drw.h util.h ${SRC} dwm.png transient.c dwm-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm dwm-msg ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm-msg
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1
	gcc vim-mask/main-alpha.c -lX11 -o vim-mask/main-alpha
	cp -f vim-mask/main-alpha ${DESTDIR}${PREFIX}/bin/vimmask
	chmod 755 ${DESTDIR}${PREFIX}/bin/vimmask
	cp -f software/vimmask.sh ${HOME}/software
	gcc simulate-key/simulate_key.c -o simulate-key/simulate_key
	cp -f simulate-key/simulate_key ${HOME}/software
	chmod +x ${HOME}/software/simulate_key
	cp -f runningtag/runningtag.py ${HOME}/software
	chmod +x ${HOME}/software/runningtag.py
	mkdir -p ${HOME}/software/bin/rofi-script
	cp -f rofi-script/librofiscript.py ${HOME}/software/bin/rofi-script
	cp -f rofi-script/funcs.py ${HOME}/software/bin/rofi-script
	cp -f rofi-script/rofi.py ${HOME}/software/bin/rofi-script
	chmod +x ${HOME}/software/bin/rofi-script/rofi.py
	cp -f rofi-script/rofi.sh ${HOME}/software
	chmod +x ${HOME}/software/rofi.sh

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm \
		${DESTDIR}${MANPREFIX}/man1/dwm.1 \
		${DESTDIR}${PREFIX}/bin/dwm-msg 

.PHONY: all options clean dist install uninstall
