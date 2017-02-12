
# Name of source file
SOURCE=demo

# S: Number of students here
# M: Number of markers here
# K: Number of markers needed to demonstrate a project
# N: Number of demos per marker / Max demos a marker can do
# T: Length of a session in minutes
# D: Length of a demo in minutes
PARAMETERS=2 1 1 2 4 2

## USE CASE #1
# Demos take 2 minutes, D=2
# there are 2 students and 1 marker, S=2, M=1
# 1 marker required for 1 demo, K=1, a marker does 2 demos, N=2
# S M K N T D
CASE_1=2 1 1 2 4 2

## USE CASE #2
# Demos take 1 minutes, D=1
# there are 1 students and 1 marker, S=1, M=1
# 1 marker required for 1 demo, K=1, a marker does 1 demos, N=1
# S M K N T D
CASE_2=1 1 1 1 2 1

## USE CASE #3
# TIMEOUT, complete 1 demo
# Demos take 2 minutes, D=2
# there are 3 students and 1 marker, S=1, M=1
# 1 marker required for 1 demo, K=1, a marker does 3 demos, N=3
# S M K N T D
CASE_3=1 1 1 3 3 2

## USE CASE #4
# Multiple Markers for multiple demos
# Demos take 2 minutes, D=2
# there are 50 students and 10 markers, S=50, M=10
# 2 marker required for 1 demo, K=2, a marker does 200 demos, N=200, T=150
# S M K N T D
CASE_4=50 10 2 200 150 2


# Can add -O3 to add optimisations // -l pthread // std=c99
CC_OPTS=-pthread -Wall -pedantic -Wextra

# First instruction is default
all: build
	./$(SOURCE) $(PARAMETERS)

tests: permissions
	./$(SOURCE) $(CASE_1) && \
	./$(SOURCE) $(CASE_2) && \
	./$(SOURCE) $(CASE_3) && \
	./$(SOURCE) $(CASE_4)

permissions:
	chmod 777 $(SOURCE)

build:
	gcc $(SOURCE).c -o $(SOURCE) $(CC_OPTS)
