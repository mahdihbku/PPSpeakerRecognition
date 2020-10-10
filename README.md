# PPSpeakerRecognition
P Practical Privacy-Preserving Speaker Recognition System
## Installation
Requires [Kaldi](https://github.com/kaldi-asr/kaldi)

In egs/voxceleb/pp :
```bash
swig -python ec_elgamal.i
gcc -c ec_elgamal.c ec_elgamal_wrap.c -lcrypto -I/usr/include/python2.7 -fPIC
gcc -shared ec_elgamal.o ec_elgamal_wrap.o -lcrypto -o _ec_elgamal.so
python setup.py build_ext --inplace
```

## Usage
Server:
```bash
./server.py --suspectsDir suspectsDir/ --serverIP=127.0.0.1 --verbose --CPUs=16 --serverPort 8003
```

End device:
```bash
./client.py --serverIP=127.0.0.1 --threshold=0.99 --serverPort=8003
```
