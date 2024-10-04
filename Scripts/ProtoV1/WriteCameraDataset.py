#!/usr/bin/env python3

import numpy as np
import zmq
from threading import Thread
import signal
import struct
from PIL import Image

###########################################################################################
###################################### BEGIN INPUTS #######################################
###########################################################################################

cameras = [
    ["tcp://localhost:8008", "C:\\UnrealProjects\\dataset"],
#    ["ipc:///tmp/ue4_stere_right", "/home/ivan/dataset/stereo_right"],
#    ["ipc:///tmp/ue4_stere_left", "/home/ivan/dataset/stereo_left"],
#    ["ipc:///tmp/ue4_camera", "/home/ivan/dataset/stereo_left_depth"]
]
skip_mod = 1 # 1 - no skip 
depth_32bit_scale = 1 / 200.0

###########################################################################################
####################################### END INPUTS ########################################
###########################################################################################

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

class WriterThread(Thread):

    def __init__(self, ctx, camera_addr, file_prefix):
        Thread.__init__(self)
        self.cam_sock = ctx.socket(zmq.SUB)
        self.cam_sock.setsockopt(zmq.SUBSCRIBE, b'')
        self.cam_sock.connect(camera_addr)
        self.camera_addr = camera_addr
        self.file_prefix = file_prefix
        print('Connected to {}'.format(camera_addr))
        self.file_timestamps = open(file_prefix + ".timestamps", "w")

    
    def run(self):
        img_header, img_data, image = None, None, None
        while is_running:
            try:
                rawbuf = self.cam_sock.recv(0)
            except zmq.ZMQError as e:
                if e.errno != zmq.EAGAIN: raise
                break

            img_header = struct.unpack(TENSOR_MSG_HEADER_FMT, rawbuf[:HEADER_LENGTH])
            img_data = rawbuf[HEADER_LENGTH:]

            if img_header[6] % skip_mod != 0: continue

            if img_header[3] > 1:
                np_image = np.frombuffer(img_data, ocv2np_type(img_header[4])).reshape(img_header[1:4])
            else:
                np_image = np.frombuffer(img_data, ocv2np_type(img_header[4])).reshape(img_header[1:3])


            if img_header[4] == CV_8U or img_header[4] == CV_16U:
                img = Image.fromarray(np_image)

                if img_header[3] == 3:
                    b, g, r = img.split()
                    img = Image.merge("RGB", (r, g, b))
                elif img_header[3] == 4:
                    b, g, r, a = img.split()
                    img = Image.merge("RGB", (r, g, b))


            elif img_header[4] == CV_32F and img_header[3] == 1:
                np_image_16bit = np.trunc(np_image * depth_32bit_scale * 0xFFFF).astype(np.uint16)
                img = Image.new("I", np_image_16bit.T.shape)
                img.frombytes(np_image_16bit.tobytes(), 'raw', "I;16")
            else:
                print("Data type ", mg_header[4], " not supported")
                continue;
                
            path = self.file_prefix + ("_%06d.png" % img_header[6])
            img.save(path, "PNG", compress_level=0)


            print(self.camera_addr, "  : Save image ", path, "Res:", img_header[1], "x", img_header[2], "Timestamp:", img_header[5], "Index:", img_header[6])

            self.file_timestamps.write(str(img_header[6]) + " " + str(img_header[5]) + "\n")
            self.file_timestamps.flush()

        self.file_timestamps.close()
        print(self.camera_addr, 'is finished')
    
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
    
    writers = []

    for i in range(0, len(cameras)):
        writers.append(WriterThread(ctx, cameras[i][0], cameras[i][1]))
        writers[i].start()


    writers[0].join()

    print('Finished')

