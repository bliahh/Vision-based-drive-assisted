
import torch
import numpy as np

CONF_SIGNS = 0.35


def run_signs(model, frame: np.ndarray):
    with torch.no_grad():
        return model(
            frame,
            verbose=False,
            conf=CONF_SIGNS,
            imgsz=640,
        )[0]


def collect_sign_data(results, names: dict) -> list:
    out = []
    if results is None or results.boxes is None:
        return out
    for box, cls, conf in zip(
        results.boxes.xyxy.cpu().numpy(),
        results.boxes.cls.cpu().numpy(),
        results.boxes.conf.cpu().numpy(),
    ):
        out.append({
            "label": names[int(cls)],
            "conf":  round(float(conf), 3),
            "box":   [int(x) for x in box],
        })
    return out
