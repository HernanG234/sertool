BINDIR=$(DESTDIR)/usr/bin
CFLAGS = -Wall -Wextra -O99 -Werror -Wno-unused-parameter -Wno-missing-field-initializers

sertool: sertool.o

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: install
install:
	install sertool $(BINDIR)/sertool

.PHONY: clean
clean:
	rm -f sertool sertool.o
