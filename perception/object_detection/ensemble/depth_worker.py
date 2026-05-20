"""
depth_worker.py — Depth Anything V2 async worker
Estimeaza adancimea per-pixel si transforma coordonate
din spatiul imaginii in spatiul real (metri, ego-centric).
Depth map-ul ramane la rezolutie mica pentru precizie.
"""

import threading
import queue
import numpy as np
import torch
import cv2
from PIL import Image
from transformers import pipeline

DEPTH_SCALE = 12.0
FOV_H_DEG = 85.0
NEAR_Z_M = 3.0
FAR_Z_M = 22.0
DEPTH_GAMMA = 1.3

class DepthWorker(threading.Thread):

    def __init__(self, device="cuda", input_size=(640, 360)):
        super().__init__(daemon=True, name="DepthWorker")
        self._in_queue = queue.Queue(maxsize=1)
        self._depth_map = None
        self._lock = threading.Lock()
        self._stop_event = threading.Event()
        self._device = device
        self._input_size = input_size
        self.calibrated = False
        self.frame_w = 1920
        self.frame_h = 1080
        self._focal_px = None
        self.latency_ms = 0.0

        dev_idx = 0 if (device == "cuda" and torch.cuda.is_available()) else -1
        print(f"[DEPTH] CUDA available: {torch.cuda.is_available()}")

        self._pipe = pipeline(
            task="depth-estimation",
            model="depth-anything/Depth-Anything-V2-Small-hf",
            device=dev_idx,
            dtype=torch.float16,
        )
        print(f"[DEPTH] Running on: {self._pipe.device}")

    def push_frame(self, frame_bgr):
        self.frame_h, self.frame_w = frame_bgr.shape[:2]
        if self._focal_px is None:
            fov_rad = np.radians(FOV_H_DEG)
            self._focal_px = (self.frame_w / 2.0) / np.tan(fov_rad / 2.0)
            print(f"[DEPTH] FOV={FOV_H_DEG} focal_px={self._focal_px:.1f}")
        try:
            self._in_queue.put_nowait(frame_bgr.copy())
        except queue.Full:
            pass

    def run(self):
        import time
        while not self._stop_event.is_set():
            try:
                frame_bgr = self._in_queue.get(timeout=0.1)
            except queue.Empty:
                continue

            t0 = time.perf_counter()

            small = cv2.resize(frame_bgr, self._input_size)
            rgb = small[:, :, ::-1]
            img = Image.fromarray(rgb)

            out = self._pipe(img)
            raw = np.array(out["depth"], dtype=np.float32)

            raw = cv2.GaussianBlur(raw, (5, 5), 0)

            d_min = raw.min()
            d_max = raw.max()
            if d_max - d_min > 1e-6:
                norm = (raw - d_min) / (d_max - d_min)
            else:
                norm = np.zeros_like(raw)

            depth_map = 1.0 - norm

            with self._lock:
                self._depth_map = depth_map
                self.calibrated = True

            self.latency_ms = (time.perf_counter() - t0) * 1000.0

    def stop(self):
        self._stop_event.set()

    def _depth_at_pixel(self, px, py):
        if self._depth_map is None:
            return 10.0

        scale_x = self._input_size[0] / self.frame_w
        scale_y = self._input_size[1] / self.frame_h
        x = int(np.clip(px * scale_x, 0, self._input_size[0] - 1))
        y = int(np.clip(py * scale_y, 0, self._input_size[1] - 1))

        raw_d = float(self._depth_map[y, x])
        corrected = pow(raw_d, DEPTH_GAMMA)
        z_m = float(np.clip(corrected * DEPTH_SCALE, NEAR_Z_M, FAR_Z_M))
        return z_m

    def _pixel_to_world(self, px, py):
        z_m = self._depth_at_pixel(px, py)
        cx = self.frame_w / 2.0
        x_m = (px - cx) * z_m / (self._focal_px or 1000.0)
        return x_m, z_m

    def transform_bbox(self, x1, y1, x2, y2):
        with self._lock:
            center_x = (x1 + x2) / 2.0
            bottom_y = y2
            x_m, z_m = self._pixel_to_world(center_x, bottom_y)
        return {"x_m": round(x_m, 2), "z_m": round(z_m, 2)}

    def transform_lane_points(self, points_norm):
        if not self.calibrated or not points_norm:
            return points_norm
        result = []
        with self._lock:
            for p in points_norm:
                px = p[0] * self.frame_w
                py = p[1] * self.frame_h
                x_m, z_m = self._pixel_to_world(px, py)
                result.append([round(x_m, 3), round(z_m, 3)])
        return result