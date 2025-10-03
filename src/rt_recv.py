import sys
import multiprocessing
import datetime
import socket
import selectors
import argparse
import queue
#import time

#import numpy as np
import pyaudio

import sound_device

def thr_nw_get(bind_ip: str, bind_port: int, to_loopback: multiprocessing.Queue):
    sock_buf_size = 8192
    e_rcv_sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    e_rcv_sock.bind((bind_ip, bind_port))
    print(f"Binded address: {bind_ip}\nBinded port: {bind_port}")

    e_rcv_sock.setblocking(False)
    e_rcv_selector = selectors.DefaultSelector()
    e_rcv_selector.register(e_rcv_sock, selectors.EVENT_READ)
    while True:
        e_rcv_selector.select()
        rcv_raw = e_rcv_sock.recv(sock_buf_size)[0:]
        try:
            to_loopback.put(rcv_raw)
        except queue.Full:
            pass

def lb_starter(fs, n_ch_o, res, dev_idx,
                 lb_queue: multiprocessing.Queue,
                 lb_request: multiprocessing.Event,
                 vol_q:multiprocessing.Queue):
    s_out = sound_device.sound_output(dev_idx, fs, res, n_ch_o, False, 1024, 4)
    while True:
        s_out.playback(lb_queue.get())

if __name__ == '__main__':
    rcv_bind_ip = '127.0.0.1'
    rcv_bind_port = 60288
    aud_fsample = 48000
    aud_dev = 0
    res_str = "16"

    ca_parser = argparse.ArgumentParser(description='')
    ca_parser.add_argument("--src-address",  help="IPv4 Address to bind")
    ca_parser.add_argument("--src-port", type=int, help="Port to bind")
    ca_parser.add_argument("--aud-list", action='store_true', help="List audio device")
    ca_parser.add_argument("--aud-fs", type=int, help="List audio device")
    ca_parser.add_argument("--aud-dev", type=int, help="List audio device")
    ca_parser.add_argument("--aud-fmt", type=str, help="List audio device")

    parsed_args = ca_parser.parse_args()
    parsed_args_dict = vars(parsed_args)
    if parsed_args_dict['src_address'] is not None:
        rcv_bind_ip = parsed_args_dict['src_address']
    if parsed_args_dict['src_port'] is not None:
        rcv_bind_port = parsed_args_dict['src_port']
    if parsed_args_dict['aud_list'] is True:
        audio_port = pyaudio.PyAudio()
        device_count = audio_port.get_device_count()
        print("\n Output list\n -------------")
        for idx in range(0, device_count, 1):
            dev_info = audio_port.get_device_info_by_index(idx)
            if dev_info['maxOutputChannels'] > 0:
                print(f" {audio_port.get_host_api_info_by_index(dev_info['hostApi'])['name']}"
                      f" | device{idx}: {dev_info['name']},"
                      f" max input channel: {dev_info['maxInputChannels']},"
                      f" max output channel: {dev_info['maxOutputChannels']},"
                      f" default sample rate:{dev_info['defaultSampleRate']}")
        print(" -------------\n")
        audio_port.terminate()
        sys.exit()
    if parsed_args_dict['aud_fs'] is not None:
        aud_fsample = parsed_args_dict['aud_fs']
    if parsed_args_dict['aud_dev'] is not None:
        aud_dev = parsed_args_dict['aud_dev']
    if parsed_args_dict['aud_fmt'] is not None:
        res_str = parsed_args_dict['aud_fmt']

    s_vol = multiprocessing.Queue(maxsize=1)
    lb_data_queue = multiprocessing.Queue(maxsize=2)
    lb_event = multiprocessing.Event()

    e_recv_thr = multiprocessing.Process(target=thr_nw_get,
                                         args=(rcv_bind_ip, rcv_bind_port,
                                               lb_data_queue),
                                         daemon=True)
    lb_thr = multiprocessing.Process(target=lb_starter, #target=thr_loopback,
                                     args=(aud_fsample, 2, res_str,
                                           aud_dev, lb_data_queue, lb_event,
                                           s_vol),
                                     daemon=True)
    e_recv_thr.start()
    lb_thr.start()
    lb_event.set()

    running = True
    try:
        print(
            "Commands:\n"
            "vol : set volume\n"
            "lbs : stop loopback\n"
            "lb  : start loopback\n"
            "q   : exit")
        while running:
            cmd = input("com > ")
            if cmd.lower() == 'q':
                running = False
            if cmd.lower() == 'lbs':
                lb_event.clear()
            if cmd.lower() == 'lb':
                lb_event.set()
            if (cmd.lower() == 'v') | (cmd.lower() == 'vol'):
                try:
                    vg_db = float(input("vol > "))
                    s_vol.put(vg_db)
                except ValueError:
                    print("Illegal value.")
    except KeyboardInterrupt:
        print(f" main: at {datetime.datetime.now().isoformat()}")
        print(" main: KeyboardInterrupt received, exiting.")
