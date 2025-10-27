'''
module windowfuncs:
collections of window function
'''
import math

import numpy as np
from scipy import signal

def SMAWindow(wsize):
    return np.ones(wsize)

def kbd_window(wsize, k_beta=12):
    '''returns Kaiser-Bessel derived window function'''
    return signal.windows.kaiser_bessel_derived(wsize, np.pi*k_beta)

def kaiser_window(wsize, beta=14):#Kaiser(beta=14)
    '''returns Kaiser window function'''
    warray = np.zeros(wsize)
    warray = signal.get_window(('kaiser',beta), wsize)
    return warray

def hamming_window(wsize):
    '''returns Hamming window function'''
    return signal.windows.hamming(wsize, sym=True)

def vorbis_window(wsize):
    '''returns Vorbis window function'''
    #ref: https://ja.wikipedia.org/wiki/%E7%AA%93%E9%96%A2%E6%95%B0#Vorbis%E7%AA%93
    warray = np.zeros(wsize)
    for i in np.arange(0,wsize,1):
        warray[i] = math.sin((math.pi / 2.0) * math.pow(math.sin(math.pi*(i/(wsize-1))), 2))
    return warray

def vorbis_window_half(wsize):
    '''returns first half of Vorbis window function'''
    warray = vorbis_window(wsize*2)
    return warray[:wsize]

def kbd_window_half(wsize):
    '''returns first half of KBD window function'''
    #ref: https://ja.wikipedia.org/wiki/%E7%AA%93%E9%96%A2%E6%95%B0#Vorbis%E7%AA%93
    warray = kbd_window(wsize*2)
    return warray[:wsize]


def GaussianWindow(wsize):
    """
    ref: http://www.eng.kagawa-u.ac.jp/~tishii/Lab/Etc/gauss.html
    """
    warray = np.zeros(wsize)
    iterator1 = np.arange(0,wsize,1)
    sigma = np.std(iterator1)
    for i in iterator1:
        warray[i] = (1/(math.sqrt(2*math.pi)*sigma))*math.exp(-1*(math.pow(i-((wsize-1)/2),2)/(2*math.pow(sigma,2))))
    return warray

def SineWindow(wsize):
    """
    ref: http://www.spcom.ecei.tohoku.ac.jp/~aito/kisosemi/slides2.pdf
    """
    warray = np.zeros(wsize)
    iterator1 = np.arange(0,wsize,1)
    for i in iterator1:
        warray[i] = math.sin((math.pi*(i/(wsize-1))))
    return warray

def PowSineWindow(wsize, npow=2):
    """
    ref: http://www.spcom.ecei.tohoku.ac.jp/~aito/kisosemi/slides2.pdf
       : https://en.wikipedia.org/wiki/Window_function
    """
    warray = np.zeros(wsize)
    iterator1 = np.arange(0,wsize,1)
    for i in iterator1:
        warray[i] = math.sin((math.pi*(i/(wsize-1))))**npow
    return warray

def BlackmanWindow(wsize):
    warray = signal.get_window('blackman', wsize)
    return warray

def NuttallWindow(wsize):
    warray = signal.get_window('nuttall', wsize)
    return warray

def BHWindow(wsize):#Blackman-Harris
    warray = signal.get_window('blackmanharris', wsize)
    return warray

def DChebWindow(wsize):#Dolph-Chebyshev(100dB)
    warray = signal.get_window(('chebwin',100.0), wsize)
    return warray
