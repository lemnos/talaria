VERSION=0.1
LDFLAGS=-lcairo -lX11 -lutil -lXinerama -lpthread
CFLAGS=-g -Wno-error=switch -Wno-error=unused-parameter -pedantic -Wall -Wextra -Wno-unused-parameter `pkg-config --libs --cflags fontconfig`

all:
	-mkdir bin
	gcc -DVERSION=\"$(VERSION)\" ${LDFLAGS} ${CFLAGS} -DGIT_HASH=\"$$(git rev-parse HEAD)\" -o bin/talaria filters/*.c widgets/*.c *.c 
man:
	pandoc -s -t man man.md|gzip > talaria.1.gz
install:
	install -m755 bin/talaria /usr/bin
	install -m544 talaria.1.gz /usr/share/man/man1
