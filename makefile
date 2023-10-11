all:
	echo "all"

run_read:
	gcc read_noncanonical.c -Wall -o read_noncanonical
	./read_noncanonical /dev/ttyS11
run_write:
	gcc write_noncanonical.c alarm.c -Wall -o write_noncanonical
	./write_noncanonical /dev/ttyS10

popo:
	echo popo
