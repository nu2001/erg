from __future__ import print_function
import serial
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
import pickle


def readlog(fname):
    '''reads pickle file and plots it'''
    data = pickle.load(open(fname))
    st = data['start_time_us']
    et = data['end_time_us']
    samples = data['samples']

    time_diff = float(et-st)
    tl = np.arange(0.0, time_diff, time_diff/len(samples))
    plt.plot(tl, samples)
    plt.ylim(-50, 5050)
    plt.grid('on')
    

def readdata(fname):
    '''reads pickle file and plots it'''
    data = pickle.load(open(fname))
    st = data['start_time_us']
    et = data['end_time_us']
    samples = data['samples']
    return (st, et, samples)
