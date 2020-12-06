# PPSpeakerRecognition
Practical Privacy-Preserving Speaker Recognition System
## Installation
Requires [Kaldi](https://github.com/kaldi-asr/kaldi)

Required changes:
- Copy the folder privacy_code/ into kaldi_dir/src/
- Erase the Makefile in kaldi_dir/src/ with the new Makefile
- Make kaldi_dir/src/
- Copy the folder pp/ into kaldi_dir/egs/voxceleb/
- cd kaldi_dir/egs/voxceleb/pp/

In kaldi_dir/egs/voxceleb/pp/ :
```bash
swig -python ec_elgamal.i
gcc -c ec_elgamal.c ec_elgamal_wrap.c -lcrypto -I/usr/include/python2.7 -fPIC
gcc -shared ec_elgamal.o ec_elgamal_wrap.o -lcrypto -o _ec_elgamal.so
python setup.py build_ext --inplace
```

## Usage
Server:
```bash
server.py --verbose --db_folder db1000 --db_metadata db_metadata --server_port 1234 --cpus 16
# OR
./server_cmd.sh
```

End device:
```bash
./client.py --verbose --wav_dir one_file_db --server_port 1234 --cpus 4 --wav one_file_db/wav/id10270/5r0dWxy17C8/00015.wav --dby_portion 1
# OR
./client_cmd.sh
```
