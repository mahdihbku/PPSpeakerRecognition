
all:

EXTRA_CXXFLAGS = -Wno-sign-compare
include ../kaldi.mk

OBJFILES = plda_normalized.o ec_elgamal.o

BINFILES = lda_transform_xvectors compare_one_file_to_db sim generate_encrypted_db generate_client_files

TESTFILES =

LIBNAME = kaldi-privacy-code

ADDLIBS = ../ivector/kaldi-ivector.a ../hmm/kaldi-hmm.a ../gmm/kaldi-gmm.a \
          ../tree/kaldi-tree.a ../util/kaldi-util.a ../matrix/kaldi-matrix.a \
          ../base/kaldi-base.a

LDLIBS += -lcrypto

include ../makefiles/default_rules.mk
