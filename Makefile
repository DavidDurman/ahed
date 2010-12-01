all: main
main:
		gcc -O2 ahed.c main.c -o ahed
debug:
		gcc -ggdb3 ahed.c main.c -o ahed
test:
	./ahed -c -i text -o text.ahed -l text.log
	cat text.log
	./ahed -x -i text.ahed -o text.dec -l text.ahed.log
	cat text.ahed.log
	md5sum text
	md5sum text.dec

