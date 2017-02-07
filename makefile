
# Name of source file
SOURCE=demo
PARAMETERS=3 3 1 1 2 1

# Can add -O3 to add optimisations // -l pthread // std=c99
CC_OPTS=-pthread -Wall -pedantic -Wextra

# First instruction is default
all: build permissions
	./$(SOURCE) $(PARAMETERS)

permissions:
	chmod 777 $(SOURCE)

build:
	gcc $(SOURCE).c -o $(SOURCE) $(CC_OPTS)
