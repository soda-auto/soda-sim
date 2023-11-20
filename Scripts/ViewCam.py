#!/usr/bin/env python3

import numpy as np
import zmq
from threading import Thread
import signal
import struct
import cv2
import sys

camera_addr = "tcp://localhost:8008"
scale_percent = 1
depth_32bit_scale = 1 / 200.0

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

CV_8U = 0
CV_8S = 1
CV_16U = 2
CV_16S = 3
CV_32S = 4
CV_32F = 5
CV_64F = 6

def ocv2np_type(ocv_type):
    if ocv_type == CV_8U: return np.uint8
    if ocv_type == CV_8S: return np.int8
    if ocv_type == CV_16U: return np.uint16
    if ocv_type == CV_16S: return np.int16
    if ocv_type == CV_32S: return np.int32
    if ocv_type == CV_32F: return np.float32
    if ocv_type == CV_64F: return np.float64


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
        camera_addr = sys.argv[1];
        
    print('Started')
    signal.signal(signal.SIGINT, signal_handler)

    ctx = zmq.Context()
    cam_sock = ctx.socket(zmq.SUB)
    cam_sock.setsockopt(zmq.SUBSCRIBE, b'')
    cam_sock.setsockopt(zmq.CONFLATE, 1)
    cam_sock.connect(camera_addr)

    print('Connected to {}'.format(camera_addr))

    

    img_header, img_data = None, None
    while is_running:
        try:
            rawbuf = cam_sock.recv(0)
        except zmq.ZMQError as e:
            print('ZMQ error')
            if e.errno != zmq.EAGAIN: raise

        img_header = struct.unpack(TENSOR_MSG_HEADER_FMT, rawbuf[:HEADER_LENGTH])
        img_data = rawbuf[HEADER_LENGTH:]

        print("Got image; Depth:", img_header[0], "Height:", img_header[1], "Width:", img_header[2], "Channels:", img_header[3], "DataType:", img_header[4], "Timestamp:", img_header[5], "Index:", img_header[6])

        if img_header[3] > 1:
            np_image = np.frombuffer(img_data, ocv2np_type(img_header[4])).reshape(img_header[1:4])
        else:
            np_image = np.frombuffer(img_data, ocv2np_type(img_header[4])).reshape(img_header[1:3])

        if img_header[4] == CV_8U or img_header[4] == CV_16U:
            pass
        elif img_header[4] == CV_32F and img_header[3] == 1:
            np_image = np.trunc(np_image * depth_32bit_scale * 0xFFFF).astype(np.uint16)
        else:
            print("Data type ", mg_header[4], " not supported")
            break;

        width = int(img_header[2] * scale_percent)
        height = int(img_header[1] * scale_percent)
        cv2.imshow('image', cv2.resize(np_image, (width, height)) )
        cv2.waitKey(1)
       
    print('Finished')

