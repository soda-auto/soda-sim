#!/usr/bin/env python3

import zmq
import signal
import sys

is_running = True

def signal_handler(signum, frame):
    import sys
    global is_running
    if (is_running):
        is_running = False
    else:
        sys.exit(0)


if __name__ == '__main__':

    if len(sys.argv) >= 2: 
        zmq_addr = sys.argv[1];
        
    print('Started')
    signal.signal(signal.SIGINT, signal_handler)

    ctx = zmq.Context()
    zmq_sock = ctx.socket(zmq.SUB)
    zmq_sock.setsockopt(zmq.SUBSCRIBE, b'')
    zmq_sock.bind(zmq_addr)
    #zmq_sock.connect(zmq_addr)
    print('Connected to {}'.format(zmq_addr))

    while is_running:
        try:
            rawbuf = zmq_sock.recv()
            print("Got %s bytes: %s" %  (len(rawbuf), rawbuf))
            while zmq_sock.getsockopt(zmq.RCVMORE):
                rawbuf = zmq_sock.recv()
                print("Got more %s bytes" %  len(rawbuf))
        except zmq.ZMQError as e:
            print('ZMQ error')
            if e.errno != zmq.EAGAIN: raise
       
    print('Finished')

