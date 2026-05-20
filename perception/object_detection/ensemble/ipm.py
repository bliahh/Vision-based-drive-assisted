

import numpy as np
import torch
from PIL import Image
from transformers import pipeline


LANE_WIDTH_M  = 3.5
NEAR_Z_M      = 2.0
FAR_Z_M       = 40.0
ROAD_CENTER_X = LANE_WIDTH_M / 2.0

DEPTH_SCALE   = 20.0


FOV_H_DEG     = 70.0


class DepthEstimator:

    def __init__(self, device: str = "cuda"):
        self.calibrated  = False
        self.frame_w     = 1920
        self.frame_h     = 1080
        self._depth_map  = None
        self._focal_px   = None
        self._device     = device

        dev_idx = 0 if (device == "cuda" and torch.cuda.is_available()) else -1

        print("[DEPTH] Loading Depth Anything V2 Small...")
        self._pipe = pipeline(
            task="depth-estimation",
            model="depth-anything/Depth-Anything-V2-Small-hf",
            device=dev_idx,
        )
        print("[DEPTH] Model loaded")

    def process_frame(self, frame_bgr: np.ndarray):

        self.frame_h, self.frame_w = frame_bgr.shape[:2]

        if self._focal_px is None:
            fov_rad        = np.radians(FOV_H_DEG)
            self._focal_px = (self.frame_w / 2.0) / np.tan(fov_rad / 2.0)

        rgb   = frame_bgr[:, :, ::-1]
        img   = Image.fromarray(rgb)
        out   = self._pipe(img)
        raw   = np.array(out["depth"], dtype=np.float32)

        d_min = raw.min()
        d_max = raw.max()
        if d_max - d_min > 1e-6:
            norm = (raw - d_min) / (d_max - d_min)
        else:
            norm = np.zeros_like(raw)

        self._depth_map = 1.0 - norm
        self.calibrated = True

    def _depth_at_pixel(self, px: float, py: float) -> float:
        if self._depth_map is None:
            return 10.0

        x = int(np.clip(px, 0, self.frame_w - 1))
        y = int(np.clip(py, 0, self.frame_h - 1))

        raw_d = float(self._depth_map[y, x])


        z_m = DEPTH_SCALE * raw_d
        z_m = float(np.clip(z_m, NEAR_Z_M, FAR_Z_M))
        return z_m

    def _pixel_to_world(self, px: float, py: float):
        z_m = self._depth_at_pixel(px, py)
        cx  = self.frame_w / 2.0
        x_m = (px - cx) * z_m / self._focal_px
        return x_m, z_m

    def transform_bbox(self, x1, y1, x2, y2) -> dict:
        bcx      = (x1 + x2) / 2.0
        bcy      = y2
        x_m, z_m = self._pixel_to_world(bcx, bcy)
        return {"x_m": round(x_m, 2), "z_m": round(z_m, 2)}

    def transform_lane_points(self, points_norm: list) -> list:
        if not self.calibrated or not points_norm:
            return points_norm

        result = []
        for p in points_norm:
            px       = p[0] * self.frame_w
            py       = p[1] * self.frame_h
            x_m, z_m = self._pixel_to_world(px, py)
            result.append([round(x_m, 3), round(z_m, 3)])
        return result