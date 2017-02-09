
# Name of source file
SOURCE=demo

# S: Number of students here
# M: Number of markers here
# K: Number of markers needed to demonstrate a project
# N: Number of demos per marker / Max demos a marker can do
# T: Length of a session in minutes
# D: Length of a demo in minutes

## USE CASE #1
# Demos take 2 minutes, D=2
# there are 2 students and 1 marker, S=2, M=1
# 1 marker required for 1 demo, K=1, a marker does 2 demos, N=2
# To have enough time to run all demos, the session must be D < T, T=4
# S M K N T D
# 2 1 1 2 4 2

## USE CASE #2
# Demos take 1 minutes, D=1
# there are 1 students and 1 marker, S=1, M=1
# 1 marker required for 1 demo, K=1, a marker does 1 demos, N=1
# To have enough time to run all demos, the session must be D < T, T=1
# S M K N T D
# 1 2 1 1 2 1

PARAMETERS=2 1 1 2 4 2

# Can add -O3 to add optimisations // -l pthread // std=c99
CC_OPTS=-pthread -Wall -pedantic -Wextra

# First instruction is default
all: build permissions
	./$(SOURCE) $(PARAMETERS)

permissions:
	chmod 777 $(SOURCE)

build:
	gcc $(SOURCE).c -o $(SOURCE) $(CC_OPTS)
