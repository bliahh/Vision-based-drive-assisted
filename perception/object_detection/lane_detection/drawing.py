import cv2
import numpy as np

COLOR_DRIVABLE = ( 60, 180, 60)
COLOR_LANE = (255, 200, 0)
ALPHA_DRIVABLE = 0.50
ALPHA_LANE = 0.80

def apply_mask(frame: np.ndarray, mask: np.ndarray,color: tuple, alpha: float) -> np.ndarray:

    if mask is None or not mask.any():
        return frame
    overlay = frame.copy()
    overlay[mask > 0] = color
    cv2.addWeighted(overlay, alpha, frame, 1 - alpha, 0, frame)
    return frame

def draw_hybridnets(frame: np.ndarray,mask_da: np.ndarray,mask_lane: np.ndarray,show_da: bool,show_lane: bool) -> np.ndarray:

    if show_da:
        frame = apply_mask(frame, mask_da, COLOR_DRIVABLE, ALPHA_DRIVABLE)
    if show_lane:
        frame = apply_mask(frame, mask_lane, COLOR_LANE, ALPHA_LANE)
    return frame