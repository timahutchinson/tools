from __future__ import division
import time

import numpy as np
import pyaudio
import pyautogui


def click(x, y, dummymode=False):
    print("Click!")
    if dummymode:
        return
    pyautogui.click(x, y)


DUMMYMODE = True
dispsize = pyautogui.size()
clickpos = (dispsize[0]//2, int(dispsize[1]*0.4))

chunk = 1024
format = pyaudio.paFloat32
channels = 2
rate = 44100
refresh_time = 10.0
ambient_mem = 2
threshold = chunk * 0.3
delta_t_click = 1.0

ambient = 10**-10
old_data = []
stopped = False
last_click = time.time() - delta_t_click

while not stopped:
    p = pyaudio.PyAudio()
    stream = p.open(format=format,
                    channels=channels,
                    rate=rate,
                    input=True,
                    frames_per_buffer=chunk)
    time.sleep(0.1)
    n_frames = int(rate / chunk * refresh_time)
    frame_size = chunk // 2
    data = np.zeros(n_frames*frame_size, dtype=np.float32) * np.NaN
    t0 = time.time()
    for i in range(n_frames):
        data[i*frame_size:(i+1)*frame_size] = np.fromstring(
            stream.read(chunk), count=frame_size)
        if ambient is None:
            ambient = np.nanmean(np.abs(data[i * frame_size:(i+1) *
                                             frame_size]))
        if np.sum(np.abs(data[i * frame_size:(i+1) * frame_size]) >
                  ambient).astype(int) > threshold:
            print("Mean noise level: %s" %
                  (np.mean(data[i * frame_size:(i+1) * frame_size])))
            if clickpos is None:
                x = np.random.randint(0, dispsize[0])
                y = np.random.randint(0, dispsize[1])
            else:
                x, y = clickpos
            if time.time() > last_click + delta_t_click:
                click(x, y, dummymode=DUMMYMODE)
                last_click = time.time()
    old_data.append(data)
    if len(old_data) > ambient_mem:
        old_data.pop(0)
    ambient = np.nanmean(np.abs(old_data))
    print("Ambient = %s" % (ambient))
    stream.stop_stream()
    stream.close()
