#!/usr/bin/env python

import time
import argparse
import os
import pickle
import sys
import numpy as np
import socket
import ec_elgamal
import os
import struct
import random
from multiprocessing import Pool
from random import randint
from utils import *
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--wav',			type=str,	default="client_dir/test.wav",	help="Captured recording in .wav file."	)
parser.add_argument('--wav_dir',		type=str,	default="",		help="Captured recording directory with .wav file."		)
parser.add_argument('--working_dir',	type=str,	default="client_dir",	help="Folder containing generated/received files.")
parser.add_argument('--nnet_dir',		type=str,	default="xvector_nnet_1a",	help="Path to the trained NN model."		)
parser.add_argument('--mfcc_config',	type=str,	default="conf/mfcc.conf",	help="Path to mfcc configuration."			)
parser.add_argument('--server_ip',		type=str,	default='127.0.0.1',	help="Server IP address."						)
parser.add_argument('--server_port',	type=int,	default=6546,	help="Server port number."								)
parser.add_argument('--cpus',			type=int,	default=4,		help="Number of parallel CPUs to be used."				)
parser.add_argument('--verbose',		action='store_true', 		help="Output more details."								)
parser.add_argument('--load', 			action='store_true', 		help="Load from stored encrypted DB."					)
parser.add_argument('--dby_portion',	type=int,	default=1,		help="Portion of DBY to be stored locally (0: no DBY, 1:full DBY, 2:half DBY, 3:third DBY, 4:quarter DBY.")
parser.add_argument('--off_results',	type=str,	default="off_results_c.csv",	help="Experiment offline time/storage."		)
parser.add_argument('--onl_results',	type=str,	default="onl_results_c.csv",	help="Experiment online time/storage."		)
args = parser.parse_args()

pub_key_file	= os.path.join(args.working_dir, "rec_ec_pub.txt")
enc_db_file 	= os.path.join(args.working_dir, "rec_enc_db.bin")
enc_sum_x2_file	= os.path.join(args.working_dir, "rec_enc_sum_x2.bin")
n_list_file 	= os.path.join(args.working_dir, "n_list.txt")
enc_db_y_file	= os.path.join(args.working_dir, "enc_db_y.bin")
# enc_y2_var_file	= os.path.join(args.working_dir, "enc_y2_var.bin")
# enc_y2_file		= os.path.join(args.working_dir, "enc_y2.bin")
enc_r3_file		= os.path.join(args.working_dir, "enc_r3.bin")
psi_file		= os.path.join(args.nnet_dir, "psi.data")
rand_nbrs_file	= os.path.join(args.working_dir, "rand_num.data")
generated_y_file= os.path.join(args.working_dir, "normalized_y.txt")
rand_nbrs_min_bitlen	= 8
rand_nbrs_max_bitlen	= 8
ec_elgamal_ct_size		= 130
# similarity_threshold	= -40.44
similarity_threshold	= -100

def connect_to_server():
	try:
		if args.verbose:	print("connectToServer: Connecting to {}:{}...".format(args.server_ip, args.server_port))
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		server_address = (args.server_ip, args.server_port)
		sock.connect(server_address)
		if args.verbose:	print("connectToServer: Connected")
		return sock
	except IndexError as e:
		print "connect_to_server: Error connecting to the server!"
		if args.verbose:	print(e)

def get_pub_key(sock):
	try:
		if args.verbose:	print("get_pub_key: Getting the server's key...")
		message = 'GET pub_key'
		sock.sendall(message)
		get_file(pub_key_file, sock)
		if args.verbose:	print("get_pub_key: pub_key received and saved in {}".format(pub_key_file))
	except IndexError as e:
		print "get_pub_key: Error getting the server's key!"
		if args.verbose:	print(e)

def get_DB_files(sock):
	try:
		if args.verbose:	print("get_DB_files: Getting Encrypted DB files...")
		message = "GET DBfiles"
		sock.sendall(message)
		get_file(enc_db_file, sock)
		get_file(enc_sum_x2_file, sock)
		get_file(n_list_file, sock)
		if args.verbose:	print("get_DB_files: Encrypted DB received and saved in {}, {} and {}".format(enc_db_file, enc_sum_x2_file, n_list_file))
	except IndexError as e:
		print "get_DB_files: Error getting the server's database files!"
		if args.verbose:	print(e)

def generate_local_files():
	try:
		if args.verbose:	print("generate_local_files: Generating client files...")
		start = time.time()
		expected_stored_indices = 256 if args.dby_portion == 1 else 128 if args.dby_portion == 2 else 86 if args.dby_portion == 3 else 64 if args.dby_portion == 4 else 0;
		expected_DBY_size = os.path.getsize(enc_db_file) * expected_stored_indices
		# TODO compare expected_DBY_size with available RAM
		subprocess.check_output('sh generate_client_files.sh '+args.working_dir+' '+pub_key_file+' '+enc_db_file+' '+enc_db_y_file+' '+enc_r3_file+' '+str(args.cpus)+' '+str(args.dby_portion)+' &> /dev/null || exit 1', shell=True)
		end = time.time()
		if args.verbose:	print("generate_local_files: Client files have been generated")
		if args.verbose:	print("N	{}	cpus 	{}	dby_portion	{}	enc_time	{}	storage(MB)	{}".format(os.path.getsize(enc_db_file)/130/200, \
			args.cpus, args.dby_portion, (end-start)*1000, (os.path.getsize(enc_db_file)+os.path.getsize(enc_db_y_file)+os.path.getsize(enc_r3_file))/1024/1024))
		results_file = open(args.off_results, "a+")
		results_file.write("N	{}	cpus 	{}	dby_portion	{}	enc_time	{}	storage(MB)	{}\n".format(os.path.getsize(enc_db_file)/130/200, \
			args.cpus, args.dby_portion, (end-start)*1000, (os.path.getsize(enc_db_file)+os.path.getsize(enc_db_y_file)+os.path.getsize(enc_r3_file))/1024/1024))
		results_file.close()
	except IndexError as e:
		print "generate_local_files: Error generating client files!"
		if args.verbose:	print(e)

def fill_rand_numbers_file():
	try:
		if args.verbose:	print("fill_rand_numbers_file: Generating random numbers file...")
		N = os.path.getsize(enc_db_file) / 200 / ec_elgamal_ct_size
		ec_elgamal.prepare_for_enc(pub_key_file)
		f = open(rand_nbrs_file, "wb")
		for i in range(N):
			rand1 = random.getrandbits(randint(rand_nbrs_min_bitlen, rand_nbrs_max_bitlen))
			rand2 = random.getrandbits(randint(rand_nbrs_min_bitlen, rand_nbrs_max_bitlen))
			if (rand1 < rand2):
				rand1, rand2 = rand2, rand1		# always rand1 > rand2
			f.write(str(rand1)+"\x00")
			f.write(ec_elgamal.encrypt_ec(str(rand2)))
		f.close()
		if args.verbose:	print("fill_rand_numbers_file: Random numbers file generated and saved in {}".format(rand_nbrs_file))
	except IndexError as e:
		print "fill_rand_numbers_file: Error filling the file of random numbers!"
		if args.verbose:	print(e)

def send_scores(sock, scores):
	try:
		message = "new_scores "
		sock.sendall(message)
		send_msg(sock, scores)
	except IndexError as e:
		print "send_scores: Error sending scores!"
		if args.verbose:	print(e)

def compute_sub_scores(list):
	# return [ec_elgamal.compute_encrypted_score_y_set(i, 1) for i in list]		# TODO uncomment
	return [ec_elgamal.compute_encrypted_score_y_set(i, 1) for i in list]

if __name__ == '__main__':
	sock = connect_to_server()
	if not args.load:
		# Offline phase_______________________________________
		get_pub_key(sock)
		get_DB_files(sock)
		generate_local_files()
		fill_rand_numbers_file()

	# Online phase____________________________________________
	# TODO check if required files are in place
	if args.verbose:	print("main: Loading required files to memory...")
	# ec_elgamal.prepare_for_enc(pub_key_file)	# TODO uncomment
	ec_elgamal.prepare(pub_key_file, "/home/mahdi/workspace/speaker_recognition/tools/kaldi-master/egs/voxceleb/pp/ec_priv.txt")	# TODO delete!
	ec_elgamal.load_scoring_files(enc_db_file, enc_sum_x2_file, enc_db_y_file, enc_r3_file, psi_file, rand_nbrs_file, n_list_file, similarity_threshold)
	N = os.path.getsize(enc_db_file) / 200 / ec_elgamal_ct_size	# number of suspects in the database
	if args.verbose:	print("main: Required files have been loaded")
	if args.verbose:	print("main: Generating the y vector...")
	start_rtt_time = time.time()
	speaker = "speaker1"

	# print('sh extract_y.sh '+speaker+' '+args.wav)

	proc = subprocess.Popen(['sh extract_y.sh '+speaker+' '+args.wav], stdout=subprocess.PIPE, shell=True)
	(xvector, err) = proc.communicate()

	# print(xvector)

	ec_elgamal.set_y_from_buf(xvector)

	end1 = time.time()
	if args.verbose:	print("main: Feature vector y generated")
	start2 = time.time()

	# enc_scores = [ec_elgamal.compute_encrypted_score_y_set(i) for i in range(N)]
	pool = Pool(processes=args.cpus)
	indices = range(N)
	enc_scores = pool.map(compute_sub_scores, (indices[int(i*N/args.cpus):int((i+1)*N/args.cpus)] for i in range(args.cpus)))
	enc_scores = [ent for sublist in enc_scores for ent in sublist]
	pool.close()
	end2 = time.time()

	# quit()

	send_scores(sock, pickle.dumps(enc_scores))
	data = sock.recv(11)

	stored_indices = os.path.getsize(enc_db_y_file)/130/200/N;
	dby_portion = 1 if stored_indices == 256 else 2 if stored_indices == 128 else 3 if stored_indices == 86 else 4 if stored_indices == 64 else 0
	if args.verbose:	print("N	{}	cpus 	{}	dby_portion	{}	reco_time	{}	enc_time 	{}	comm(B)	{}".format(N, args.cpus, dby_portion, \
		(end1-start_rtt_time)*1000, (end2-start2)*1000, N*130))

	if (data == "GET rec    "):
		send_file(args.wav, sock)	#TODO change to the real wav file (--wav or --wav_dir)
		print("main: Suspect detected! Recording has been sent")
		end_rtt_time = time.time()
		if args.verbose:	print("rtt_time 	{}".format((end_rtt_time-start_rtt_time)*1000))

	results_file = open(args.onl_results, "a+")
	results_file.write("N	{}	cpus 	{}	dby_portion	{}	reco_time	{}	enc_time 	{}	comm(KB)	{}".format(N, args.cpus, dby_portion, \
		(end1-start_rtt_time)*1000, (end2-start2)*1000, N*130/1024))
	if (data == "GET rec    "):
		results_file.write("	rtt_time 	{}".format((end_rtt_time-start_rtt_time)*1000))
	results_file.write("\n")
	results_file.close()

	fill_rand_numbers_file()
