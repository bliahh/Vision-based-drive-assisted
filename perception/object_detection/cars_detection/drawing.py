
import cv2
import numpy as np
from .detector import CAR_CLASSES

CAR_COLORS = {
    2: (255, 200,  50),
    3: (255, 150,  50),
    5: (255,  80,  80),
    7: (200,  80, 255),
    0: (100, 200, 255),
}


def draw_cars(frame: np.ndarray, results, debug: bool):
    """
        Deseneaza pe frame detectiile de masini rezultate din YOLO.

        Afiseaza bounding box-uri si etichete pentru clasele definite in CAR_CLASSES.
        Daca debug este activ, afiseaza si scorul de incredere (confidence).

        Args:
            frame (np.ndarray): cadrul video pe care se deseneaza detectiile
            results: output-ul YOLO (contine box-uri, clase si confidente)
            debug (bool): daca este True, afiseaza si confidence-ul

        Returns:
            tuple:
                - frame: imaginea modificata cu bounding box-uri
                - n: numarul de masini desenate
        """



    if results is None or results.boxes is None:
        return frame, 0

    n = 0
    for box, cls, conf in zip(
        results.boxes.xyxy.cpu().numpy(),
        results.boxes.cls.cpu().numpy(),
        results.boxes.conf.cpu().numpy(),
    ):
        cls_int = int(cls)
        if cls_int not in CAR_CLASSES:
            continue

        x1, y1, x2, y2 = map(int, box)
        color = CAR_COLORS.get(cls_int, (200, 200, 200))
        name  = CAR_CLASSES[cls_int]

        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2, cv2.LINE_AA)

        lbl = f"{name} {conf:.2f}" if debug else name
        (tw, th), _ = cv2.getTextSize(lbl, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
        cv2.rectangle(frame, (x1, y1 - th - 6), (x1 + tw + 6, y1), color, -1)
        cv2.putText(frame, lbl, (x1 + 3, y1 - 3),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1, cv2.LINE_AA)
        n += 1

    return frame, n
