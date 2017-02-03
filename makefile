
# Name of source file
SOURCE=demo

# Can add -O3 to add optimisations // -l pthread
CC_OPTS=-c -std=c99 -pipe -Wall -pedantic -Wextra -O3 -Wno-switch -ggdb -g3 -l pthread
CC=gcc


build:
	$(CC) $(SOURCE).c -o $(SOURCE) $(CC_OPTS)

clean:
	rm -f $(SOURCE)

#  This is the default action
all:
	clean
	build

