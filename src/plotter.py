from multiprocessing import Queue
import pyqtgraph
import numpy as np

from scipy import fft
import windowfuncs

class Plotter:
    def __init__(self, h_upper, h_lower, d_queue: Queue):
        self.plot_data_queue = d_queue
        self.scope_min = h_lower
        self.scope_max = h_upper
        self.wf_L_graph = None
        self.wf_R_graph = None
        self.L_plot = None
        self.R_plot = None
        self.L_curve = None
        self.R_curve = None
        self.fsample = 48000
        self.window_len = 4096
        self.freq_list = fft.rfftfreq(self.window_len, 1/self.fsample)
        self.window_function = windowfuncs.DChebWindow(self.window_len)
        self.window_function = self.window_function / np.mean(self.window_function)

    def set_fsample(self, fs):
        self.fsample = fs

    def set_window_function(self, idx :int):
        self.window_function = windowfuncs.DChebWindow(self.window_len)
        self.window_function = self.window_function / np.mean(self.window_function)

    def thr_plot(self):
        #label_dc.setText("DC Component: %5.1f[dB]" % (data_to_plot[0]))
        p_data = self.plot_data_queue.get()
        if self.window_len != len(p_data[0]) :
            self.window_len = len(p_data[0])
            self.window_function = windowfuncs.DChebWindow(self.window_len)
            self.window_function = self.window_function / np.mean(self.window_function)
            self.freq_list = fft.rfftfreq(self.window_len, 1/self.fsample)
            self.L_plot.setXRange(np.log10(self.freq_list[1]), np.log10(self.fsample/2))
            self.R_plot.setXRange(np.log10(self.freq_list[1]), np.log10(self.fsample/2))

        fft_res_L = np.fft.rfft(p_data[0]*self.window_function/(len(p_data[0])*2))
        fft_res_R = np.fft.rfft(p_data[1]*self.window_function/(len(p_data[0])*2))
        #data_index_L = np.arange(0, len(fft_res_L), 1)
        #data_index_R = np.arange(0, len(fft_res_R), 1)
        #print(f"L len: {len(fft_res_L)}, R len: {len(fft_res_R)}\r")
        self.L_curve.setData(self.freq_list[1:], 20*(np.log10(np.absolute(fft_res_L)+1e-100)[1:]))
        self.R_curve.setData(self.freq_list[1:], 20*(np.log10(np.absolute(fft_res_R)+1e-100)[1:]))

    def thr_plot_execute(self, fps=30):
        g_app = pyqtgraph.mkQApp()
        self.wf_L_graph = pyqtgraph.GraphicsLayoutWidget(title="L_ch")
        self.wf_R_graph  = pyqtgraph.GraphicsLayoutWidget(title="R_ch")
        self.L_plot = self.wf_L_graph.addPlot(row=1, col=0)
        self.R_plot = self.wf_R_graph.addPlot(row=1, col=0)
        self.L_curve = self.L_plot.plot(
            symbol=None, symbolBrush=None, symbolPen=None, pen=(0, 255, 0))
        self.R_curve = self.R_plot.plot(
            symbol=None, symbolBrush=None, symbolPen=None, pen=(0, 255, 0))
        self.L_plot.setLogMode(x=True, y=None)
        self.R_plot.setLogMode(x=True, y=None)
        self.L_plot.setYRange(self.scope_min, self.scope_max)
        self.R_plot.setYRange(self.scope_min, self.scope_max)
        self.L_plot.setXRange(0, np.log10(self.fsample/2))
        self.R_plot.setXRange(0, np.log10(self.fsample/2))
        self.L_plot.showGrid(x=True, y=True, alpha=0.75)
        self.R_plot.showGrid(x=True, y=True, alpha=0.75)
        plot_timer = pyqtgraph.Qt.QtCore.QTimer()
        plot_timer.timeout.connect(self.thr_plot)
        plot_timer.start(int(1000/fps))
        self.wf_L_graph.show()
        self.wf_R_graph.show()
        g_app.exec()
