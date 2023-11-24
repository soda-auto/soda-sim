#!/usr/bin/env python3

import numpy as np
import zmq
import signal
import struct
import time


addr = "tcp://172.16.11.159:8008"

# struct MsgHeader
# {
# 	union Tenser_t {
# 		std::int32_t Data[4];
# 		struct
# 		{
# 			std::int32_t Depth;
# 			std::int32_t Height;
# 			std::int32_t Width;
# 			std::int32_t Channels;
# 		} Shape;
# 	} Tenser;
# 	std::int32_t DataType;
# 	std::int64_t Timestamp;
# 	std::int64_t Index;
# };

TENSOR_MSG_HEADER_FMT = 'iiiiiqq'
HEADER_LENGTH = struct.calcsize(TENSOR_MSG_HEADER_FMT)

is_running = True

def signal_handler(signum, frame):
    import sys
    global is_running
    if (is_running):
        is_running = False
    else:
        sys.exit(0)

if __name__ == '__main__':

    print('Started')
    signal.signal(signal.SIGINT, signal_handler)

    ctx = zmq.Context()

    cam_sock = ctx.socket(zmq.SUB)
    cam_sock.setsockopt(zmq.SUBSCRIBE, b'')
    cam_sock.setsockopt(zmq.CONFLATE, 1)
    cam_sock.connect(addr)
    print('Connected to {}'.format(addr))

    while is_running:
        try:
            rawbuf = cam_sock.recv(0)
        except zmq.ZMQError as e:
            if e.errno != zmq.EAGAIN: raise
            break
        img_header = struct.unpack(TENSOR_MSG_HEADER_FMT, rawbuf[:HEADER_LENGTH])

        cam_timestamp = img_header[5]
        system_timestamp = int(round(time.time() * 1000))
        diff = system_timestamp - cam_timestamp

        print("Addr: {}, System: {}, Device: {}, Diff: {}".format(addr, system_timestamp, cam_timestamp, diff))

    print('Finished')

