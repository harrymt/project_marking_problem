
# Name of source file
SOURCE=demo

# Can add -O3 to add optimisations // -l pthread // std=c99
CC_OPTS=-c -std=c99 -pipe -Wall -pedantic -Wextra -O3 -Wno-switch -ggdb -g3 -g -lpthread

# First instruction is default
build:
	gcc $(SOURCE).c -o $(SOURCE) $(CC_OPTS)