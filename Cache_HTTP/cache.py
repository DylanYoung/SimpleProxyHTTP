from hashlib import md5
import mmap
from socket import (
			socket, AF_INET,
			SOCK_STREAM, SOL_SOCKET,
			SO_REUSEADDR, gethostname, MSG_WAITALL
			)
import json
from threading import Thread, Timer, Lock
from argparse import ArgumentParser
import os
path = os.path.dirname(os.path.abspath(__file__))+"/"
from datetime import datetime
from base64 import b64encode, b64decode
from email.utils import formatdate, parsedate
from time import mktime



ACCESS_ADDR = 'localhost'
PORT = 23456
FOUND = "FOUND\0\0\0\0"
NOTFOUND = "NOTFOUND\0"
VERSION = "0.0.1"
lock_d = Lock()
lock_f = Lock()

def parseargs():
	verbose = "verbose output"
	pars = ArgumentParser(
		prog='CacheHTTP',
	    fromfile_prefix_chars='<'
	)
	pars.add_argument(
		'-v','--verbose',
		help=verbose,
		action='store_true'
	)

	pars.add_argument(
		'--version',
		action='version',
		version='%(prog)s 2.0'
	)

	pars.add_argument(
		'--port', '-p',
		nargs='?',
		default=PORT,
		const=PORT
	)

	return pars.parse_args()

def serversocket():
	s = socket(AF_INET, SOCK_STREAM)
	s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
	s.bind((ACCESS_ADDR,PORT))
	return s

def appendEntry(entry):
	lock_f.acquire()
	with open(path+"cache.json", 'r+') as cache_f:
		cache_f.seek(-1, 2)
		cache_f.write(", ")
		cache_f.write(json.dumps(entry[0]))
		cache_f.write(": ")
		cache_f.write(json.dumps(entry[1]))
		cache_f.write("}")
	lock_f.release()

def flush(cache):
	print "Flushing to File \n"
	lock_f.acquire()
	lock_d.acquire()
	with open(path+"cache.json", 'w') as cache_f:
		json.dump(cache,cache_f)
	lock_f.release()
	lock_d.release()
	Timer(600, flush,(cache,)).start()

def validate(responses):
	if len(responses) > 0:
		response = b64decode(responses[0])
	else: return []

	for line in response.split("\n"):
		if "Expires: " in line:
			#print "Expires: " + line[9:] + "\n"
			try:
				exptime = mktime(parsedate(line[9:]))
				nowtime = mktime(parsedate(formatdate(timeval=None, localtime=False, usegmt=True)))
				#print "Now: " + formatdate(timeval=None, localtime=False, usegmt=True)
				valid = exptime > nowtime
			except TypeError:
				valid = True
		else: valid = True

	if valid:
		return responses
	else: return []


def handlerequest(serv, cache):
	while 1:
		sock = serv.accept()[0]
		request = ""
		#while 1:
		print "...\n"
		part = sock.recv(80000)
			#if len(part) < 1:
			#	break;
			#else:
		request += part
			#	sock.settimeout(0)
		#print request
		request="\r\n".join(request.split('\n', 2)[0:1])
		m = md5()
		m.update(request)
		key = b64encode(m.digest())
		print "Key: " + key
		responses = cache.get(key, [])
		validate(responses)
		if len(responses) > 0:
			n = sock.send(FOUND, MSG_WAITALL)

			for response in responses:
				n = sock.send(b64decode(response))
				print "Message \""+FOUND+"\" - "+ str(n) +" bytes sent."
		else:
			n = sock.send(NOTFOUND, MSG_WAITALL)
			print "Message \""+NOTFOUND+"\" - "+ str(n) +" bytes sent."

			responses = []
			while 1:
				response = sock.recv(80000)
				if len(response) < 1:
					break;
				else:
					responses.append(b64encode(response))
			lock_d.acquire()
			cache[key] = responses
			lock_d.release()
			Thread(target=appendEntry, args=((key,responses),)).start()
			print "Cache size: " + str(len(cache))
		sock.close()

def main():

	args = parseargs()
	threads = [-1]*10
	print len(threads)
	print NOTFOUND
	print FOUND
	# Load existing cache
	try:
		with open(path+"cache.json",'r') as cache_f:
			cache = json.load(cache_f)
	except (IOError, ValueError) as e:
		if isinstance(e, ValueError):
			os.rename(path+"cache.json",
				path+datetime.now().time().isoformat()+"cache.json")
			
		cache = {"CACHE":{"Version":VERSION}}
		with open(path+"cache.json", 'w') as cache_f:
			json.dump(cache,cache_f)


	serv = serversocket()
	serv.listen(5)
	# Flush the whole cache every 10 minutes
	Timer(600, flush,(cache,)).start()
	for t in threads:
		t = Thread(target=handlerequest, args=(serv,cache,))
		t.start()

# TODO: Memory map the database for faster writes
		# buf = mmap.mmap(-1,mmap.PAGESIZE*20,
		#					mmap.MAP_SHARED,
		#					mmap.PROT_READ)
	for t in threads:
		t.join()
	serv.close()
if __name__ == '__main__':
	main()
