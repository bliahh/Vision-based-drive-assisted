
import cv2
import numpy as np

from lane_detection.drawing import COLOR_DRIVABLE, COLOR_LANE
def draw_hud(frame: np.ndarray,
             fps: float,
             show_da: bool,
             show_lane: bool,
             show_cars: bool,
             show_signs: bool,
             n_cars: int,
             n_signs: int,
             debug: bool,
             dev_str: str,
             hn_ms: float,
             tcp_connected: bool) -> np.ndarray:
    h, w = frame.shape[:2]

    # Fundal panel
    cv2.rectangle(frame, (8, 8), (300, 190), (0, 0, 0), -1)
    cv2.rectangle(frame, (8, 8), (300, 190), (60, 60, 60), 1)

    title = f"Drive Assist  {fps:.1f} FPS  [{dev_str}]"
    cv2.putText(frame, title, (16, 28),cv2.FONT_HERSHEY_SIMPLEX, 0.48, (220, 220, 220), 1, cv2.LINE_AA)

    items = [
        (f"[1] Drivable: {'ON' if show_da   else 'OFF'}", COLOR_DRIVABLE if show_da   else (60, 60, 60)),
        (f"[2] Lanes: {'ON' if show_lane  else 'OFF'}", COLOR_LANE if show_lane  else (60, 60, 60)),
        (f"[3] Cars:  {n_cars} obiecte", (255, 200, 50) if show_cars  else (60, 60, 60)),
        (f"[4] Signs: {n_signs} semne", (0, 140, 255)  if show_signs else (60, 60, 60)),
    ]
    for i, (txt, col) in enumerate(items):
        cv2.putText(frame, txt, (16, 52 + i * 26),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.46, col, 1, cv2.LINE_AA)

    tcp_col = (0, 220, 80) if tcp_connected else (80, 80, 80)
    tcp_txt = "TCP: ON" if tcp_connected else "TCP: OFF"
    cv2.putText(frame, tcp_txt, (16, 166),
                cv2.FONT_HERSHEY_SIMPLEX, 0.46, tcp_col, 1, cv2.LINE_AA)

    if debug:
        cv2.putText(frame, f"HN: {hn_ms:.0f}ms", (w - 110, 22),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (80, 255, 200), 1)

    cv2.putText(frame, "Q=quit 1=DA 2=lane 3=cars 4=signs D=debug S=save",
                (w - 385, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.38, (110, 110, 110), 1)

    return frame
