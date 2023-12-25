#!/bin/bash

./bitblt_table_gen -h > $1/bitblt_tables.h
./bitblt_table_gen -c > $1/bitblt_tables.c
