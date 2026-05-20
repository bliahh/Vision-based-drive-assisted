import cv2
import numpy as np

SIGN_COLOR  = (0, 100, 255)
CORNER_SIZE = 12


def draw_signs(frame: np.ndarray, results, names: dict, debug: bool):
    if results is None or results.boxes is None:
        return frame, 0

    n = 0
    for box, cls, conf in zip(
        results.boxes.xyxy.cpu().numpy(),
        results.boxes.cls.cpu().numpy(),
        results.boxes.conf.cpu().numpy(),
    ):
        x1, y1, x2, y2 = map(int, box)
        name = names[int(cls)]

        for px, py in [(x1, y1), (x2, y1), (x1, y2), (x2, y2)]:
            dx = CORNER_SIZE  if px == x1 else -CORNER_SIZE
            dy = CORNER_SIZE  if py == y1 else -CORNER_SIZE
            cv2.line(frame, (px, py), (px + dx, py), SIGN_COLOR, 3, cv2.LINE_AA)
            cv2.line(frame, (px, py), (px, py + dy), SIGN_COLOR, 3, cv2.LINE_AA)

        lbl = f"{name} {conf:.2f}" if debug else name
        (tw, th), _ = cv2.getTextSize(lbl, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
        cv2.rectangle(frame, (x1, y1 - th - 8), (x1 + tw + 6, y1), SIGN_COLOR, -1)
        cv2.putText(frame, lbl, (x1 + 3, y1 - 4),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)
        n += 1

    return frame, n
