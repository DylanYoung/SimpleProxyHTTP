from hashlib import md5
import mmap
from socket import (
			socket, AF_INET,
			SOCK_STREAM, SOL_SOCKET, 
			SO_REUSEADDR, gethostname
			)
import json
from threading import Thread, Timer, Lock
from argparse import ArgumentParser
import os
from datetime import datetime


ACCESS_ADDR = 'localhost'
PORT = 23456
FOUND = "FOUND\0\0\0\0"
NOTFOUND = "NOTFOUND\0"
lock = Lock()

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

def flush(cache):
	lock.acquire()
	with open("cache.json", 'w') as cache_f:
		json.dump(cache,cache_f)
	lock.release()


def handlerequest(serv, cache):
	while 1:
		sock = serv.accept()[0]
		request = ""
		while 1:
			print "..."
			part = sock.recv(80000)
			if len(part) < 0:
				break;
			else: request += part;
		print request
		"\r\n".join(request.split('\n', 2)[0:1])
		key = md5.update(request).digest()
		responses = cache.get(key, [])
		if not responses == []:
			sock.send(FOUND)
			for response in responses:
				sock.send(response)
		else:
			sock.send(NOTFOUND)

			responses = []
			while 1:
				response = recv(80000)
				print response
				responses.append(response)
				if len(response) < 1:
					break;
			lock.acquire()
			cache[key] = responses
			lock.release()
		sock.close()

def main():

	args = parseargs()
	threads = [-1]*5
	print len(threads)
	print NOTFOUND
	print FOUND
	# Load existing cache
	try: 
		with open("cache.json",'r') as cache_f:
			cache = json.load(cache_f)
	except IOError:
		os.rename("cache.json",
			datetime.now().time().isoformat()+"cache.json")
		cache = dict()
		with open("cache.json", 'w') as cache_f:
			json.dump(cache,cache_f)
		

	serv = serversocket()
	serv.listen(5)
	Timer(30, flush,(cache,)).start()
	for t in threads:
		t = Thread(target=handlerequest, args=(serv,cache,))
		threads.append(t)
		t.start()

		# buf = mmap.mmap(-1,mmap.PAGESIZE*20, 
		#					mmap.MAP_SHARED, 
		#					mmap.PROT_READ)
	for t in threads:
		t.join()
	serv.close()
if __name__ == '__main__':  
	main()