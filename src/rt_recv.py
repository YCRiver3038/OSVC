import sys
import multiprocessing
import datetime
import socket
import selectors
import argparse
import queue
import time

import numpy as np
import pyaudio

import sound_device
import windowfuncs

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
        print(f"\x1b[1;1H\x1b[0Krecv chunk size: {len(rcv_raw)//8}")
        try:
            to_loopback.put(rcv_raw)
        except queue.Full:
            pass

def thr_dsp(dataQueue: multiprocessing.Queue,
            outputQueue:multiprocessing.Queue,
            maLen:multiprocessing.Queue):
    movavrLen = 16
    dataLen = 0
    movavrWin = windowfuncs.vorbis_window(movavrLen)
    movavrWin = movavrWin / np.sum(movavrWin)
    dataL = np.zeros(2)
    dataR = np.zeros(2)
    while True:
        if not maLen.empty():
            movavrLen = maLen.get()
            movavrWin = windowfuncs.vorbis_window(movavrLen)
            movavrWin = movavrWin / np.sum(movavrWin)
        dataLR = dataQueue.get()
        dataLRf = np.frombuffer(dataLR, dtype=np.float32)
        if dataLen != len(dataLRf):
            dataLen = len(dataLRf)
            dataL = np.zeros(len(dataLRf[0::2])*2)
            dataR = np.zeros(len(dataLRf[1::2])*2)
        processedLR = np.zeros(len(dataLRf))
#        dataL = np.zeros(len(dataLRf[0::2])+(movavrLen*2))
#        dataR = np.zeros(len(dataLRf[1::2])+(movavrLen*2))
#        dataL[movavrLen-1:movavrLen-1+len(dataLRf[0::2])] = dataLRf[0::2]
#        dataR[movavrLen-1:movavrLen-1+len(dataLRf[1::2])] = dataLRf[1::2]
        dataL[:len(dataL)//2] = dataL[(len(dataL)//2):]
        dataR[:len(dataR)//2] = dataL[(len(dataR)//2):]
        dataL[(len(dataL)//2):] = dataLRf[0::2]
        dataR[(len(dataR)//2):] = dataLRf[1::2]
        dataL = np.convolve(dataL, movavrWin, mode='same')
        dataR = np.convolve(dataR, movavrWin, mode='same')
#        processedLR[0::2] = dataL[movavrLen-1:movavrLen-1+len(dataLRf[0::2])]
#        processedLR[1::2] = dataR[movavrLen-1:movavrLen-1+len(dataLRf[1::2])]
        processedLR[0::2] = dataL[(len(dataL)//4):len(dataL)-(len(dataL)//4)]
        processedLR[1::2] = dataR[(len(dataR)//4):len(dataR)-(len(dataR)//4)]
        outputQueue.put(processedLR.astype(dtype=np.float32).tobytes())

def lb_starter(fs, n_ch_o, res, dev_idx,
               lb_queue: multiprocessing.Queue):
    s_out = sound_device.sound_output(dev_idx, fs, res, n_ch_o, False, 768, 2)
    print("\x1b[2J")
    while True:
        s_out.playback(lb_queue.get())

if __name__ == '__main__':
    rcv_bind_ip = '127.0.0.1'
    rcv_bind_port = 60288
    aud_fsample = 48000
    aud_dev = 0
    aud_res = pyaudio.paInt16
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
    dsp_data_queue = multiprocessing.Queue(maxsize=2)
    maLenQueue = multiprocessing.Queue()

    e_recv_thr = multiprocessing.Process(target=thr_nw_get,
                                         args=(rcv_bind_ip, rcv_bind_port,
                                               lb_data_queue),
                                         daemon=True)
    dsp_thr = multiprocessing.Process(target=thr_dsp,
                                      args=(lb_data_queue, dsp_data_queue, maLenQueue),
                                      daemon=True)
    lb_thr = multiprocessing.Process(target=lb_starter, #target=thr_loopback,
                                     args=(aud_fsample, 2, res_str, aud_dev,
                                           dsp_data_queue),
                                     daemon=True)
    lb_thr.start()
    e_recv_thr.start()
    dsp_thr.start()
    
    running = True
    try:
        print(
            "Commands:\n"
            "vol : set volume\n"
            "ml: Moving average length\n"
            "q   : exit")
        while running:
            cmd = input("com > ")
            if cmd.lower() == 'q':
                running = False
            if cmd.lower() == 'ml':
                try:
                    maLenQueue.put(int(input("mov avr len > ")))
                except ValueError:
                    print("Illegal value.")
            if (cmd.lower() == 'v') | (cmd.lower() == 'vol'):
                try:
                    vg_db = float(input("vol > "))
                    s_vol.put(vg_db)
                except ValueError:
                    print("Illegal value.")
    except KeyboardInterrupt:
        print(f" main: at {datetime.datetime.now().isoformat()}")
        print(" main: KeyboardInterrupt received, exiting.")
print("\x1b[2J")
