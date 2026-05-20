import time
import threading

import numpy as np
import torch


class YOLOWorker(threading.Thread):


    def __init__(self, model, name: str, **infer_kwargs):

        super().__init__(daemon=True, name=name)
        self.model        = model
        self.infer_kwargs = infer_kwargs

        self._in_frame  = None
        self._in_lock   = threading.Lock()
        self._in_event  = threading.Event()

        self._out_result = None
        self._out_lock   = threading.Lock()

        self._stop      = threading.Event()
        self.latency_ms = 0.0

    def push_frame(self, frame: np.ndarray):
        with self._in_lock:
            self._in_frame = frame.copy()
        self._in_event.set()

    def get_results(self):

        with self._out_lock:
            return self._out_result

    def stop(self):
        self._stop.set()
        self._in_event.set()

    def run(self):
        while not self._stop.is_set():
            self._in_event.wait()
            self._in_event.clear()

            if self._stop.is_set():
                break

            with self._in_lock:
                frame = self._in_frame
            if frame is None:
                continue

            t0 = time.perf_counter()

            with torch.no_grad():
                result = self.model(
                    frame,
                    verbose=False,
                    **self.infer_kwargs,
                )[0]

            self.latency_ms = (time.perf_counter() - t0) * 1000

            with self._out_lock:
                self._out_result = result