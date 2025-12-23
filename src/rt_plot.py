import multiprocessing
import datetime
import socket
import argparse
import sys
import time
import queue

import pyaudio
import numpy as np

import plotter

def thr_recv(e_rcv_sock,
             to_plotter: multiprocessing.Queue, plot_length: int,
             to_loopback: multiprocessing.Queue):
    SOCK_BUF_SIZE = 8192
    o_array_L = np.zeros(plot_length)
    o_array_R = np.zeros(plot_length)
    while True:
        try:
            rcv_raw = e_rcv_sock.recv(SOCK_BUF_SIZE)
        except BlockingIOError:
            continue
        try:
            to_loopback.put(rcv_raw, block=False)
        except queue.Full:
            pass
        r_data = np.frombuffer(rcv_raw, dtype=np.float32)
        r_data_L = r_data[0::2]
        r_data_R = r_data[1::2]
        #FIFO buffer
        o_array_L[0:(len(o_array_L)-len(r_data_L))] = o_array_L[len(r_data_L):]
        o_array_L[(len(o_array_L)-len(r_data_L)):] = r_data_L
        o_array_R[0:(len(o_array_R)-len(r_data_R))] = o_array_R[len(r_data_R):]
        o_array_R[(len(o_array_R)-len(r_data_R)):] = r_data_R
        try:
            to_plotter.put((o_array_L, o_array_R), block=False)
        except queue.Full:
            pass

def thr_nw_get(bind_ip: str, bind_port: int,
               to_plotter: multiprocessing.Queue, plot_length: int,
               to_loopback: multiprocessing.Queue):
    SOCK_BUF_SIZE = 8192
    e_srv_sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    e_srv_sock.bind((bind_ip, bind_port))
    print(f"Binded address: {bind_ip}\nBinded port: {bind_port}\nFIFO data length: {plot_length}")
    #e_srv_sock.listen(1)
    try:
        #while True:
            #print("listening...")
            #(rsock, c_addr) = e_srv_sock.accept()
            #print(f"Accepted client: {c_addr}")
            #rcv_thr = multiprocessing.Process(target=thr_recv,
            #                                args=(rsock, to_plotter, plot_length, to_loopback),
            #                                daemon=True)
            #rcv_thr.start()
        thr_recv(e_srv_sock, to_plotter, plot_length, to_loopback)
    except KeyboardInterrupt:
        return

def thr_waveform_plot(print_queue: multiprocessing.Queue, f_rate: int):
    w_plotter = plotter.Plotter(0.0, -140.0, print_queue)
    w_plotter.thr_plot_execute(f_rate)

class sound_output:
    def __init__(self, fs, n_ch_o, res, dev_idx):
        self.device = dev_idx
        self.fsample = fs
        self.channels = n_ch_o
        self.resolution = res
        self.ag_val = 1
        if res == pyaudio.paInt16:
            self.bsize = 2
            self.np_dt = np.int16
        elif res == pyaudio.paInt24:
            self.bsize = 4
            self.np_dt = np.int32
        elif res == pyaudio.paInt32:
            self.bsize = 4
            self.np_dt = np.int32
        elif res == pyaudio.paFloat32:
            self.bsize = 4
            self.np_dt = np.float32
        else:
            self.bsize = 2
            self.np_dt = np.int16

    def output_callback(self,
                        lb_queue: multiprocessing.Queue,
                        lb_request: multiprocessing.Event,
                        vol_q:multiprocessing.Queue):
        lb_device = pyaudio.PyAudio()
        def lb_callback(in_data, frame_count, time_info, status):
            data_arr = np.frombuffer(lb_queue.get(block=True), dtype=self.np_dt)
            try:
                self.ag_val = 10**(vol_q.get(block=False)/20)
            except queue.Empty:
                pass
            data_arr = data_arr*self.ag_val
            data = data_arr.astype(self.np_dt).tobytes()
            return (data, pyaudio.paContinue)

        chunk_len = len(lb_queue.get(block=True)) // (self.bsize*self.channels)
        lb_stream = lb_device.open(
            self.fsample, self.channels, self.resolution,
            input=False, output=True, output_device_index=self.device,
            frames_per_buffer=chunk_len,
            stream_callback=lb_callback)
        try:
            while True:
                lb_request.wait()
                if not lb_stream.is_active():
                    lb_stream.start_stream()
                while lb_request.is_set():
                    time.sleep(0.1)
                if not lb_request.is_set():
                    lb_stream.stop_stream()
        finally:
            lb_stream.stop_stream()
            lb_stream.close()
            lb_device.terminate()

def lb_starter(fs, n_ch_o, res, dev_idx,
                 lb_queue: multiprocessing.Queue,
                 lb_request: multiprocessing.Event,
                 vol_q:multiprocessing.Queue):
    s_out = sound_output(fs, n_ch_o, res, dev_idx)
    s_out.output_callback(lb_queue, lb_request, vol_q)

if __name__ == '__main__':
    plot_data_length = 8192
    plot_framerate = 60
    rcv_bind_ip = '127.0.0.1'
    rcv_bind_port = 60288
    aud_fsample = 48000
    aud_dev = 0

    ca_parser = argparse.ArgumentParser(description='')
    ca_parser.add_argument("--src-address",  help="IPv4 Address to bind")
    ca_parser.add_argument("--src-port", type=int, help="Port to bind")
    ca_parser.add_argument("--framerate", type=int, help="Plot framerate")
    ca_parser.add_argument("--plot-length", type=int, help="Plot data length")
    ca_parser.add_argument("--aud-list", action='store_true', help="List audio device")
    ca_parser.add_argument("--aud-fs", type=int, help="List audio device")
    ca_parser.add_argument("--aud-dev", type=int, help="List audio device")

    parsed_args = ca_parser.parse_args()
    parsed_args_dict = vars(parsed_args)
    if parsed_args_dict['src_address'] is not None:
        rcv_bind_ip = parsed_args_dict['src_address']
    if parsed_args_dict['src_port'] is not None:
        rcv_bind_port = parsed_args_dict['src_port']
    if parsed_args_dict['framerate'] is not None:
        plot_framerate = parsed_args_dict['framerate']
    if parsed_args_dict['plot_length'] is not None:
        plot_data_length = parsed_args_dict['plot_length']
    if parsed_args_dict['aud_list'] is True:
        audio_port = pyaudio.PyAudio()
        device_count = audio_port.get_device_count()
        print("\n Output list\n -------------")
        for idx in range(0, device_count, 1):
            dev_info = audio_port.get_device_info_by_index(idx)
            if dev_info['maxOutputChannels'] > 0:
                print(f" {audio_port.get_host_api_info_by_index(dev_info['hostApi'])['name']} "
                      f"| device{idx}: {dev_info['name']}, "
                      f"max input channel: {dev_info['maxInputChannels']}, "
                      f"max output channel: {dev_info['maxOutputChannels']}, "
                      f"default sample rate:{dev_info['defaultSampleRate']}")
        print(" -------------\n")
        sys.exit()
    if parsed_args_dict['aud_fs'] is not None:
        aud_fsample = parsed_args_dict['aud_fs']
    if parsed_args_dict['aud_dev'] is not None:
        aud_dev = parsed_args_dict['aud_dev']

    plot_data_queue = multiprocessing.Queue(maxsize=1)
    lb_data_queue = multiprocessing.Queue(maxsize=2)
    lb_event = multiprocessing.Event()
    lb_vol = multiprocessing.Queue(maxsize=1)

    e_recv_thr = multiprocessing.Process(target=thr_nw_get,
                                         args=(rcv_bind_ip, rcv_bind_port,
                                               plot_data_queue, plot_data_length,
                                               lb_data_queue))
    plot_thr = multiprocessing.Process(target=thr_waveform_plot,
                                       args=(plot_data_queue, plot_framerate), daemon=True)
    lb_thr = multiprocessing.Process(target=lb_starter,
                                     args=(aud_fsample, 2, pyaudio.paFloat32, aud_dev,
                                           lb_data_queue, lb_event, lb_vol), daemon=True)
    e_recv_thr.start()
    plot_thr.start()
    lb_thr.start()
    lb_event.set()
    try:
        print("Waiting for termination...")
        while True:
            try:
                lb_vol.put(float(input("vol > ")))
            except ValueError:
                pass
    except KeyboardInterrupt:
        print(f" main: at {datetime.datetime.now().isoformat()}")
        print(" main: KeyboardInterrupt received, exiting.")
