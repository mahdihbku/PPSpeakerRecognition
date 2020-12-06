#!/bin/bash

swig -python ec_elgamal.i
gcc -c ec_elgamal.c ec_elgamal_wrap.c -lcrypto -I/usr/include/python2.7 -fPIC
gcc -shared ec_elgamal.o ec_elgamal_wrap.o -lcrypto -o _ec_elgamal.so
python setup.py build_ext --inplace

# python server.py --load --verbose --db_folder test_db --db_metadata test_db_metadata --server_port 12345
# python server.py --verbose --db_folder small_db --db_metadata small_db_metadata --server_port $1 --cpus 1
python server.py --verbose --db_folder db1000 --db_metadata db_metadata --server_port $1 --cpus 4
# python server.py --verbose --db_folder small_db --db_metadata small_db_metadata --server_port $1 --cpus 4
