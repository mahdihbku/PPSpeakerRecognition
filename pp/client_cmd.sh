#!/bin/bash

swig -python ec_elgamal.i
gcc -c ec_elgamal.c ec_elgamal_wrap.c -lcrypto -I/usr/include/python2.7 -fPIC
gcc -shared ec_elgamal.o ec_elgamal_wrap.o -lcrypto -o _ec_elgamal.so
python setup.py build_ext --inplace

# ./client.py --load --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav

# ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 1
# # ./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 1
# ./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 1
# ./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 1

# ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 2
# # ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 2
./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 2
./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 2
./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 2

# ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 3
# # ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 3
# ./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 3
# ./client.py --verbose --load --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 3

# ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav 
# ./client.py --verbose --wav_dir one_file_db --server_port $1 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav
# ./client.py --verbose --server_port $1  --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 3
# ./client.py --load --verbose --wav_dir one_file_db --server_port 12345
