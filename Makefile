#SHELL := /bin/bash
#PATH := /usr/local/bin:$(PATH)


all:
	@rm -rf ./compile_commands.json
	bear -- bash -c "pio -f -c vim run"

upload:
	pio -f -c vim run --target upload

clean:
	@rm -rf ./compile_commands.json
	pio -f -c vim run --target clean

cleanall:
	@rm -rf ./compile_commands.json
	pio -f -c vim run --target cleanall

envdump:
	pio -f -c vim run --target envdump

program:
	pio -f -c vim run --target program

uploadfs:
	pio -f -c vim run --target uploadfs

update:
	pio -f -c vim update

monitor:
	pio device monitor
