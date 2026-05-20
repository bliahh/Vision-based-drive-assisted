import torch

CAR_CLASSES = {2: "car", 3: "moto", 5: "bus", 7: "truck", 0: "person"}
CONF_CARS   = 0.40


def run_cars(model, frame):
    """
    Ruleaza inferenta cu tracking pentru detectia de vehicule.

    Utilizeaza YOLOv11m cu BoT-SORT tracker integrat pentru a detecta
    si urmari clasele definite ca vehicule si pietoni.

    Args:
        model: Model YOLO incarcat.
        frame: Imaginea (frame-ul video) pe care se face inferenta.

    Returns:
        Rezultatul primei predictii YOLO cu tracking IDs.
    """
    with torch.no_grad():
        return model.track(
            frame,
            verbose=False,
            conf=CONF_CARS,
            classes=list(CAR_CLASSES.keys()),
            imgsz=640,
            iou=0.45,
            agnostic_nms=True,
            persist=True,       # pastreaza track-urile intre frame-uri
            tracker="botsort.yaml",
        )[0]


def collect_car_data(results) -> list:
    """
    Extrage datele despre vehicule cu tracking ID persistent.

    Returns:
        list: Lista de obiecte detectate, fiecare avand:
            - label: numele clasei (ex: car, truck)
            - conf: scorul de incredere
            - box: coordonatele bounding box [x1, y1, x2, y2]
            - track_id: ID unic persistent intre frame-uri (-1 daca nu e disponibil)
    """
    out = []
    if results is None or results.boxes is None:
        return out

    boxes = results.boxes.xyxy.cpu().numpy()
    classes = results.boxes.cls.cpu().numpy()
    confs = results.boxes.conf.cpu().numpy()

    # Track IDs — poate fi None daca tracker-ul nu a assignat inca
    has_ids = results.boxes.id is not None
    track_ids = results.boxes.id.cpu().numpy().astype(int) if has_ids else None

    for i, (box, cls, conf) in enumerate(zip(boxes, classes, confs)):
        cls_int = int(cls)
        if cls_int not in CAR_CLASSES:
            continue

        bw = box[2] - box[0]
        bh = box[3] - box[1]
        if bh < 40 or bw < 30:
            continue

        tid = int(track_ids[i]) if track_ids is not None else -1

        out.append({
            "label": CAR_CLASSES[cls_int],
            "conf":  round(float(conf), 3),
            "box":   [int(x) for x in box],
            "track_id": tid,
        })
    return out