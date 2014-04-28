all: bin/nis-notify

install: all
	install bin/nis-notify /usr/local/sbin/
	install init.d/nis-notify /etc/init.d/
	update-rc.d nis-notify defaults

uninstall:
	update-rc.d nis-notify remove
	rm -f /etc/init.d/nis-notify
	rm -f /usr/local/sbin/nis-notify

clean:
	rm -rf bin

bin/nis-notify: src/nis-notify.c bin
	$(CC) $(CFLAGS) $< -o $@

bin:
	mkdir bin
