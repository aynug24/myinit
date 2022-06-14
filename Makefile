OPTIMIZATION := O3

all: myinit scripts

myinit:
	gcc -$(OPTIMIZATION) main.c \
		log/log.c log/log.h \
		parse/parse_myinit_args.c parse/parse_myinit_args.h \
		parse/parse_config.c parse/parse_config.h \
		task/task.c task/task.h lists/add.c lists/add.h run_daemon/run_daemon.c run_daemon/run_daemon.h \
		-o myinit

scripts:
	chmod u+x *.sh

clear:
	rm -f myinit