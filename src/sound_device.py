import multiprocessing
import queue
import pyaudio

def list_devices():
    audio_port = pyaudio.PyAudio()
    device_count = audio_port.get_device_count()
    print("\n Input list\n -------------")
    for idx in range(0, device_count, 1):
        dev_info = audio_port.get_device_info_by_index(idx)
        if dev_info['maxInputChannels'] > 0:
            print(f" {audio_port.get_host_api_info_by_index(dev_info['hostApi'])['name']}"
                    f" | device{idx}: {dev_info['name']},"
                    f" max input channel: {dev_info['maxInputChannels']},"
                    f" max output channel: {dev_info['maxOutputChannels']},"
                    f" default sample rate:{dev_info['defaultSampleRate']}")
    print(" -------------\n")
    print("\n Output list\n -------------")
    for idx in range(0, device_count, 1):
        dev_info = audio_port.get_device_info_by_index(idx)
        if dev_info['maxOutputChannels'] > 0:
            print(f" {audio_port.get_host_api_info_by_index(dev_info['hostApi'])['name']} |"
                    f" device{idx}: {dev_info['name']},"
                    f" max input channel: {dev_info['maxInputChannels']},"
                    f" max output channel: {dev_info['maxOutputChannels']},"
                    f" default sample rate:{dev_info['defaultSampleRate']}")
    print(" -------------\n")
    audio_port.terminate()

class sound_output: 
    def __init__(self, index, fs, fmt, channels,
                 with_callback=False, frame_length=4096, q_len=2):
        self.fs = fs
        self.cb_data_q = multiprocessing.Queue(maxsize=q_len)
        self.with_callback = with_callback
        self.is_closed = False
        self.bytesize = 0
        self.frlen = frame_length
        if fmt == '32':
            self.format = pyaudio.paInt32
            self.bytesize = 4
        elif fmt == 'f32':
            self.format = pyaudio.paFloat32
            self.bytesize = 4
        elif fmt == '16':
            self.format = pyaudio.paInt16
            self.bytesize = 2
        elif fmt == '24':
            self.format = pyaudio.paInt24
            self.bytesize = 3
        elif fmt == '8':
            self.format = pyaudio.paInt8
            self.bytesize = 1
        self.channels = channels
        self.output_device = pyaudio.PyAudio()
        self.output_buf = bytes(self.frlen*self.channels*self.bytesize)
        #api_idx = self.output_device.get_host_api_info_by_type(pyaudio.paALSA)
        #self.dev_idx = self.output_device.get_device_info_by_host_api_device_index(api_idx, index)
        self.dev_idx = index
        try:
            if not with_callback:
                self.lb_stream = self.output_device.open(
                    self.fs, self.channels, self.format,
                    input=False, output=True,
                    output_device_index=self.dev_idx)
            else:
                self.lb_stream = self.output_device.open(
                    self.fs, self.channels, self.format,
                    input=False, output=True,
                    output_device_index=self.dev_idx,
                    frames_per_buffer=frame_length,
                    stream_callback=self.pb_callback)
        except Exception:
            self.lb_stream.close()
            self.output_device.terminate()
            raise
    def pb_callback(self, in_data, frame_count, time_info, status):
        try:
            self.output_buf=self.cb_data_q.get(block=False)
        except queue.Empty:
            #print("Underrun")
            pass
        return((self.output_buf, pyaudio.paContinue))
    def stream_reopen(self, stream: pyaudio.Stream):
        retry_ctr = 0
        while retry_ctr < 10:
            try:
                #print(f"Reopening stream (iteration {retry_ctr})")
                stream = self.output_device.open(
                    self.fs, self.channels, self.format,
                    input=False, output=True,
                    output_device_index=self.dev_idx)
                retry_ctr += 1
                #self.start()
                if stream.is_active():
                    #print(f"Stream reopened.(retried: {retry_ctr})")
                    return
                if retry_ctr > 10:
                    #print("reopen failed.")
                    break
            except Exception:
                if retry_ctr >= 10:
                    #print("Reopen failed.")
                    raise
    def stop(self):
        self.lb_stream.stop_stream()
    def start(self):
        self.lb_stream.start_stream()
    def playback(self, pb_data):
        if not self.with_callback:
            try:
                self.lb_stream.write(pb_data)
                return 0
            except IOError:
                print("Playback device error")
                self.stream_reopen(self.lb_stream)
            except Exception:
                print(" playback: error")
                self.output_device.close(self.lb_stream)
                raise
        else:
            try:
                self.cb_data_q.put(pb_data, timeout=10)
                return 0
            except queue.Full:
                return -1

    def close_device(self):
        if not self.is_closed:
            if not self.cb_data_q.empty():
                print("Output: Queue not empty")
                return -256
            print("Output: Closing device...")
            self.lb_stream.stop_stream()
            self.lb_stream.close()
            print("Output: stream closed")
            self.output_device.terminate()
            print("Output: device terminated")
            self.cb_data_q.close()
            print("Output: queue closed")
            self.is_closed = True
            print("Output: Device closed.")
            return 0
        print("WARN: Device already closed.")
        return 0

class sound_input: 
    def __init__(self, index, fs, fmt, channels,
                 frame_length=4096, with_callback=False, q_len=2):
        self.fs = fs
        self.cb_data_q = multiprocessing.Queue(maxsize=q_len)
        self.with_callback = with_callback
        self.is_closed = False
        if fmt == '32':
            self.format = pyaudio.paInt32
        elif fmt == 'f32':
            self.format = pyaudio.paFloat32
        elif fmt == '16':
            self.format = pyaudio.paInt16
        elif fmt == '24':
            self.format = pyaudio.paInt24
        elif fmt == '8':
            self.format = pyaudio.paInt8
        self.channels = channels
        self.input_device = pyaudio.PyAudio()
        self.frame_length = frame_length
        #api_idx = self.output_device.get_host_api_info_by_type(pyaudio.paALSA)
        #self.dev_idx = self.output_device.get_device_info_by_host_api_device_index(api_idx, index)
        self.dev_idx = index
        try:
            if not with_callback:
                self.in_stream = self.input_device.open(
                    self.fs, self.channels, self.format,
                    input=True, output=False,
                    input_device_index=self.dev_idx)
            else:
                self.in_stream = self.input_device.open(
                    self.fs, self.channels, self.format,
                    input=True, output=False,
                    input_device_index=self.dev_idx,
                    frames_per_buffer=frame_length,
                    stream_callback=self.in_callback)
        except Exception:
            self.in_stream.close()
            self.input_device.terminate()
            raise
    def in_callback(self, in_data, frame_count, time_info, status):
        try:
            self.cb_data_q.put(in_data, block=False)
        except queue.Full:
            self.cb_data_q.get()
            self.cb_data_q.put(in_data, block=False)
        return((in_data, pyaudio.paContinue))
    def stream_reopen(self, stream: pyaudio.Stream):
        retry_ctr = 0
        while retry_ctr < 10:
            try:
                #print(f"Reopening stream (iteration {retry_ctr})")
                stream = self.input_device.open(
                    self.fs, self.channels, self.format,
                    input=False, output=True,
                    output_device_index=self.dev_idx)
                retry_ctr += 1
                if stream.is_active():
                    #print(f"Stream reopened.(retried: {retry_ctr})")
                    return
                if retry_ctr > 10:
                    #print("reopen failed.")
                    break
            except Exception:
                if retry_ctr >= 10:
                    #print("Reopen failed.")
                    raise
    def stop(self):
        self.in_stream.stop_stream()
    def start(self):
        self.in_stream.start_stream()
    def get_data(self):
        if not self.with_callback:
            try:
                return self.in_stream.read(self.frame_length)
            except IOError:
                print("Input device error")
                self.stream_reopen(self.in_stream)
            except Exception:
                self.input_device.close(self.in_stream)
                raise
        else:
            try:
                return self.cb_data_q.get(timeout=10)
            except queue.Empty:
                return None

    def close_device(self):
        if not self.is_closed:
            while not self.cb_data_q.empty():
                self.cb_data_q.get()
            self.cb_data_q.close()
            self.in_stream.close()
            self.input_device.terminate()
            self.is_closed = True
            print("Input: Device closed.")
        else:
            print("WARN: Device already closed.")
        return 0
