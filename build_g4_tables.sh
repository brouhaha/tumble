#!/bin/bash

./g4_table_gen -h > $1/g4_tables.h
./g4_table_gen -c > $1/g4_tables.c
