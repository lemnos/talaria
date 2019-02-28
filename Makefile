VERSION=0.1
LDFLAGS=-lcairo -lX11 -lutil -lXinerama -lpthread
CFLAGS=-g -Wno-error=switch -Wno-error=unused-parameter -pedantic -Wall -Wextra -Wno-unused-parameter

all:
	-mkdir bin
	gcc -DVERSION=\"$(VERSION)\" ${LDFLAGS} ${CFLAGS} -DGIT_HASH=\"$$(git show-ref -s HEAD)\" -o bin/talaria filters/*.c widgets/*.c *.c 
man:
	pandoc -s -t man man.md|gzip > talaria.1.gz
install:
	install -m755 bin/talaria /usr/bin
	install -m544 talaria.1.gz /usr/share/man/man1
