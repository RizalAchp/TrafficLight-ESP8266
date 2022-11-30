#SHELL := /bin/bash
#PATH := /usr/local/bin:$(PATH)
ENVIRONMENT := d1esp32

all: clean
	bear -- bash -c "pio -f -c vim run --environment ${ENVIRONMENT}"

build:
	pio -f -c vim run --environment ${ENVIRONMENT}

upload:
	pio -f -c vim run --target upload --environment ${ENVIRONMENT}

clean:
	@rm -rf ./compile_commands.json
	pio -f -c vim run --target clean --environment ${ENVIRONMENT}

cleanall:
	@rm -rf ./compile_commands.json
	pio -f -c vim run --target cleanall --environment ${ENVIRONMENT}

envdump:
	pio -f -c vim run --target envdump --environment ${ENVIRONMENT}

program:
	pio -f -c vim run --target program --environment ${ENVIRONMENT}

uploadfs:
	pio -f -c vim run --target uploadfs --environment ${ENVIRONMENT}

update:
	pio -f -c vim update --environment ${ENVIRONMENT}

monitor:
	pio device monitor --environment ${ENVIRONMENT}
