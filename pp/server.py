#!/usr/bin/env python

import time
import argparse
import pickle
import numpy as np
import socket
import ec_elgamal
import os
import datetime
from multiprocessing import Pool
from utils import *
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--db_folder',		type=str,	default="test_db",	help="Path to folder containing speakers utterances.")
parser.add_argument('--db_metadata',	type=str,	default="test_db_metadata",	help="Folder containig all db generated files.")
parser.add_argument('--nnet_dir',		type=str,	default="xvector_nnet_1a",	help="Path to the trained NN model."		)
parser.add_argument('--mfcc_config',	type=str,	default="conf/mfcc.conf",	help="Path to mfcc configuration."			)
parser.add_argument('--server_ip',		type=str,	default='127.0.0.1',	help="Server IP address."						)
parser.add_argument('--server_port',	type=int,	default=6546,	help="Server port number."								)
parser.add_argument('--cpus',			type=int,	default=16,		help="Number of parallel CPUs to be used."				)
parser.add_argument('--generate_keys',	action='store_true', 		help="Generate new server keys."						)
parser.add_argument('--verbose',		action='store_true', 		help="Output more details."								)
parser.add_argument('--load', 			action='store_true', 		help="Load from stored encrypted DB."					)
parser.add_argument('--off_results',	type=str,	default="off_results_s.csv",	help="Experiment offline time/storage."	)
parser.add_argument('--onl_results',	type=str,	default="onl_results_s.csv",	help="Experiment online time/storage."	)
args = parser.parse_args()

pub_key_file	= os.path.join(args.db_metadata, "ec_pub.txt")
priv_key_file	= os.path.join(args.db_metadata, "ec_priv.txt")
enc_db_file 	= os.path.join(args.db_metadata, "enc_db.bin")
enc_sum_x2_file	= os.path.join(args.db_metadata, "enc_sum_x2.bin")
names_list_file	= os.path.join(args.db_metadata, "names_list.txt")
n_list_file 	= os.path.join(args.db_metadata, "n_list.txt")

#just for testing. If True scores will be decrypted, otherwise the program will only check if score_is_positive
decrypt = True	# TODO set to False

def generate_DB_files():
	try:
		if args.verbose:	print("generate_DB_files: Generating encrypted DB files...")
		output = subprocess.check_output('sh generate_encrypted_db.sh '+args.db_folder+' '+args.db_metadata+' '+args.nnet_dir+' ' \
			+args.mfcc_config+' '+pub_key_file+' '+enc_db_file+' '+enc_sum_x2_file+' '+names_list_file+' '+n_list_file+' ' \
			+str(args.cpus)+' '+args.off_results+' &> /dev/null || (>&2 echo "server.py: ERROR calling generate_encrypted_db.sh!"; exit 1)', shell=True)
			# +str(args.cpus)+' '+args.off_results+' &> /dev/null || (>&2 echo "server.py: ERROR calling generate_encrypted_db.sh!"; exit 1)', stderr=subprocess.STDOUT, shell=True)
		# print(output)
		if args.verbose:	print("generate_DB_files: DB files have been generated ({}, {}, {} and {})".format(enc_db_file, enc_sum_x2_file, names_list_file, n_list_file))
		
	except IndexError as e:
		print "generate_DB_files: Error getting encrypted DB files!"
		if args.verbose:	print(e)

def send_enc_DB(connection):
	try:
		if args.verbose:	print("send_enc_DB: Sending encrypted DB files...")
		send_file(enc_db_file, connection)
		send_file(enc_sum_x2_file, connection)
		send_file(n_list_file, connection)
		if args.verbose:	print("send_enc_DB: Encrypted DB files sent")
	except IndexError as e:
		print "send_enc_DB: Error sending encrypted DB files!"
		if args.verbose:	print(e)

def score_is_positive_sub_list(enc_list):
	return [ec_elgamal.score_is_positive(d) for d in enc_list]

def get_scores(connection):
	try:
		data = recv_msg(connection)
		enc_scores = pickle.loads(data)
		start_dec = time.time()
		scores = []
		# if decrypt:
		# 	ct5M = ec_elgamal.encrypt_ec("5000000")
		# 	scores = [ec_elgamal.add2(enc_score, ct5M) for enc_score in enc_scores]
		# 	scores = [ec_elgamal.decrypt_ec(enc_score) for enc_score in scores]
		# 	scores = [int(s) - 5000000 for s in scores]
		# else:
		# 	if (len(enc_scores) > args.cpus*10):
		# 		pool = Pool(processes=args.cpus)
		# 		scores = pool.map(score_is_positive_sub_list, (enc_scores[int(i*len(enc_scores)/args.cpus):int((i+1)*len(enc_scores)/args.cpus)] for i in range(args.cpus)))
		# 		scores = [ent for sublist in scores for ent in sublist]
		# 	else:
		# 		scores = score_is_positive_sub_list(enc_scores)
		end_dec = time.time()
		# print(scores)
		# if decrypt and ("5000000" in scores) or not decrypt and (0 in scores):
		if True:
			# suspect_id = scores.index("5000000") if decrypt else scores.index(0)
			suspect_id = 0
			# print("get_scores: SUSPECT DETECTED! id={} name={}".format(suspect_id, suspects_names[suspect_id]))
			print("get_scores: SUSPECT DETECTED! id={}".format(suspect_id))
			message = "GET rec    "
			connection.sendall(message)
			now = datetime.datetime.now()
			rec_file_name = "suspect_"+str(now.strftime("%Y-%m-%d-%H-%M")+".wav")
			get_file(rec_file_name, connection)
			print("get_scores: Suspect's recording saved in {}".format(rec_file_name))
		else:
			message = "No match   "
			connection.sendall(message)
		if args.verbose:	print("N	{}	cpus 	{}	dec_time	{}".format(len(scores), args.cpus, (end_dec-start_dec)*1000))
		results_file = open(args.onl_results, "a+")
		results_file.write("N	{}	cpus 	{}	dec_time	{}\n".format(len(scores), args.cpus, (end_dec-start_dec)*1000))
		results_file.close()
	except IndexError as e:
		print "get_scores: Error getting scores!"
		if args.verbose:	print(e)

def wait_for_clients():
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server_address = (args.server_ip, args.server_port)
	sock.bind(server_address)
	sock.listen(10)
	while True:
		print("waitForClients: Waiting for a connection...")
		connection, client_address = sock.accept()
		try:
			while True:
				data = recv_all(connection, 11)
				print("waitForClients: Received: {} from: {}".format(data, client_address))
				if data:
					if (data == "GET pub_key"):	send_file(pub_key_file, connection)
					if (data == "GET DBfiles"):	send_enc_DB(connection)
					if (data == "new_scores "):	get_scores(connection)
				else:
					print("waitForClients: No more data from client {}".format(client_address))
					break
		finally:
			connection.close()
			print("waitForClients: Connection closed with client {}".format(client_address))


if __name__ == '__main__':
	if not os.path.isfile(pub_key_file):
		print("server.py: {} not found".format(pub_key_file))
		exit(1)
	if not os.path.isfile(priv_key_file):
		print("server.py: {} not found".format(priv_key_file))
		exit(1)

	ec_elgamal.prepare(pub_key_file, priv_key_file)
	if not args.load:	# --load has more priority than --generate_keys (for safety)
		# if args.generate_keys:	# TODO uncomment
		# 	ec_elgamal.generate_keys(pub_key_file, priv_key_file)
		# 	if not decrypt:
		# 		ec_elgamal.generate_decrypt_file()
		generate_DB_files()
	# TODO check if db files are in place. Otherwise generate them
	if not decrypt:
		ec_elgamal.load_encryption_file()
	wait_for_clients()
