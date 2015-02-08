from hashlib import md5
import mmap
from socket import socket, AF_INET,
 SOCKSTREAM, gethostname, listen, accept, recv, send
import json
from threading import Thread, Timer, Lock
from argparse import ArgumentParser


ACCESS_ADDR = 'localhost'
PORT = 23456
FOUND = "FOUND\0\0\0\0"
NOTFOUND = "NOTFOUND\0"
lock = Lock()

def parseargs():
	verbose = "verbose output"
	pars = ArgumentParser(
		prog='CacheHTTP'
	    fromfile_prefix_chars='<'

	)
	pars.add_argument(
		'-v','--verbose', 
		help=verbose
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
		default=PORT
		const=PORT
	)

	return pars.parse_args()

def serversocket():
	server = socket(AF_INET, SOCKSTREAM)
	server.bind((ACCESS_ADDR,PORT))
	return server

def flush(cache):
	lock.aquire()
	with open("cache.json", '+w') as cache_f:
		json.dump(cache,cache_f)
	lock.release()


def handlerequest(sock, cache):
	request = ""
	oldlen = -1
	while(len(request) > oldlen):
		oldlen = len(request)
		request += sock.recv(80000)
	"\r\n".join(request.split('\n', 2)[0:1])
	key = md5.update(request).digest()
	responses = cache.get(key, [])
	if not responses == []:
		send(FOUND)
		for response in responses:
			send(response)
	else:
		send(NOTFOUND)
		oldlen = -1
		while(len(response) > oldlen):
			oldlen = len(response)
			responses.append(sock.recv(80000)) 
		lock.aquire()
		cache[key] = responses
		lock.release()

def main():

	args = parseargs()
	threads = []

	# Load existing cache
	with open("cache.json",'r') as cache_f:
		cache = json.load(cache_f)


	serv = serversocket()
	serv.listen(5)
	Timer(60, flush,(cache,)).start()
	while 1:
		worksock = serv.accept()
		t = Thread(target=worker, args=(worksock,cache,))
		threads.append(t)
		t.start()

		# buf = mmap.mmap(-1,mmap.PAGESIZE*20, 
		#					mmap.MAP_SHARED, 
		#					mmap.PROT_READ)


if __name__ == '__main__':  
	main()