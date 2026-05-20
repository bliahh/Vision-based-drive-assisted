import sys
import time
import threading
from pathlib import Path

import cv2
import numpy as np
import torch
from torchvision import transforms

HYBRIDNETS_DIR = Path(__file__).resolve().parent / "HybridNets"
if HYBRIDNETS_DIR.exists():
    sys.path.insert(0, str(HYBRIDNETS_DIR))
else:
    for p in Path(__file__).parent.parent.glob("**/hybridnets_test_videos.py"):
        sys.path.insert(0, str(p.parent))
        break

HN_INPUT_W = 640
HN_INPUT_H = 384
HN_MEAN = [0.485, 0.456, 0.406]
HN_STD = [0.229, 0.224, 0.225]
HN_WEIGHTS = str(HYBRIDNETS_DIR / "weights" / "hybridnets.pth")
HN_PROJECT = str(HYBRIDNETS_DIR / "projects" / "bdd100k.yml")

def load_hybridnets(device: torch.device):
    print("[HybridNets] Load weights...")
    try:
        from utils.utils import Params
        from backbone import HybridNetsBackbone
    except ImportError as e:
        print(f"[ERR.] {e}\nimport from HybridNets: {HYBRIDNETS_DIR}")
        sys.exit(1)

    params = Params(HN_PROJECT)
    model = HybridNetsBackbone(
        compound_coef=3,
        num_classes=len(params.obj_list),
        ratios=eval(params.anchors_ratios),
        scales=eval(params.anchors_scales),
        seg_classes=len(params.seg_list),
        backbone_name=None,
        seg_mode=getattr(params, 'seg_mode', 'multiclass'),
    )
    ckpt = torch.load(HN_WEIGHTS, map_location=device)
    model.load_state_dict(ckpt.get('model', ckpt))
    model = model.to(device)
    if device.type == 'cuda':
        model = model.half()
    model.eval()
    print(f"[HybridNets] OK — {device}")
    return model

def hn_preprocess(frame: np.ndarray, device: torch.device, use_half: bool):

    img = cv2.resize(frame, (HN_INPUT_W, HN_INPUT_H))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    t = transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize(HN_MEAN, HN_STD),
    ])(img).unsqueeze(0).to(device)
    return t.half() if use_half else t

def hn_postprocess(outputs, orig_h: int, orig_w: int):

    seg = outputs[-1].squeeze(0).float().cpu().numpy()
    exp_seg = np.exp(seg - seg.max(axis=0, keepdims=True))
    seg_idx = np.argmax(exp_seg / exp_seg.sum(axis=0, keepdims=True), axis=0)

    def make_mask(cls_id: int) -> np.ndarray:
        m = cv2.resize(
            (seg_idx == cls_id).astype(np.uint8),
            (orig_w, orig_h),
            interpolation=cv2.INTER_LINEAR,
        )
        _, m = cv2.threshold(m, 0, 1, cv2.THRESH_BINARY)
        return m

    return make_mask(1), make_mask(2)

class HybridNetsWorker(threading.Thread):

    def __init__(self, model, device: torch.device, use_half: bool):
        super().__init__(daemon=True, name="HybridNetsWorker")
        self.model = model
        self.device = device
        self.use_half = use_half

        self._in_frame = None
        self._in_lock = threading.Lock()
        self._in_event = threading.Event()

        self._out_da = None
        self._out_lane = None
        self._out_lock = threading.Lock()

        self._stop = threading.Event()
        self.latency_ms = 0.0

    def push_frame(self, frame: np.ndarray):
        with self._in_lock:
            self._in_frame = frame.copy()
        self._in_event.set()

    def get_masks(self):

        with self._out_lock:
            return self._out_da, self._out_lane

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

            orig_h, orig_w = frame.shape[:2]
            t0 = time.perf_counter()

            with torch.no_grad():
                tensor = hn_preprocess(frame, self.device, self.use_half)
                outputs = self.model(tensor)

            mask_da, mask_lane = hn_postprocess(outputs, orig_h, orig_w)
            self.latency_ms = (time.perf_counter() - t0) * 1000

            with self._out_lock:
                self._out_da = mask_da
                self._out_lane = mask_lane