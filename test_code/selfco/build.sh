#!/bin/bash
g++ -c coctx_swap.S -g -o coctx_swap.o
g++ -c selfco.cpp -g -o selfco.o
g++ -o selfco selfco.o coctx_swap.o
