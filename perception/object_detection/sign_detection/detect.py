
import argparse
import cv2
import numpy as np
from pathlib import Path
from ultralytics import YOLO


CATEGORY_COLORS = {
    "forb": (0, 0, 220),
    "warn": (0, 165, 255),
    "mand": (200, 50, 0),
    "prio": (0, 200, 0),
    "info": (180, 130, 0),
}

CLASS_NAMES = [
    'forb_ahead', 'forb_left', 'forb_overtake', 'forb_right',
    'forb_speed_over_10', 'forb_speed_over_100', 'forb_speed_over_130',
    'forb_speed_over_20', 'forb_speed_over_30', 'forb_speed_over_40',
    'forb_speed_over_5', 'forb_speed_over_50', 'forb_speed_over_60',
    'forb_speed_over_70', 'forb_speed_over_80', 'forb_speed_over_90',
    'forb_stopping', 'forb_trucks', 'forb_u_turn', 'forb_weight_over_3.5t',
    'forb_weight_over_7.5t', 'info_bus_station', 'info_crosswalk',
    'info_highway', 'info_one_way_traffic', 'info_parking', 'info_taxi_parking',
    'mand_bike_lane', 'mand_left', 'mand_left_right', 'mand_pass_left',
    'mand_pass_left_right', 'mand_pass_right', 'mand_right', 'mand_roundabout',
    'mand_straigh_left', 'mand_straight', 'mand_straight_right',
    'prio_give_way', 'prio_priority_road', 'prio_stop', 'warn_children',
    'warn_construction', 'warn_crosswalk', 'warn_cyclists',
    'warn_domestic_animals', 'warn_other_dangers', 'warn_poor_road_surface',
    'warn_roundabout', 'warn_slippery_road', 'warn_speed_bumper',
    'warn_traffic_light', 'warn_tram', 'warn_two_way_traffic',
    'warn_wild_animals'
]


def get_color(class_name: str):
    for prefix, color in CATEGORY_COLORS.items():
        if class_name.startswith(prefix):
            return color
    return (128, 128, 128)


def draw_detections(frame: np.ndarray, results, conf_thresh: float) -> np.ndarray:
    annotated = frame.copy()

    for result in results:
        boxes = result.boxes
        if boxes is None:
            continue

        for box in boxes:
            conf  = float(box.conf[0])
            if conf < conf_thresh:
                continue

            cls_id    = int(box.cls[0])
            cls_name  = CLASS_NAMES[cls_id] if cls_id < len(CLASS_NAMES) else f"cls_{cls_id}"
            color     = get_color(cls_name)

            x1, y1, x2, y2 = map(int, box.xyxy[0])

            cv2.rectangle(annotated, (x1, y1), (x2, y2), color, 2)

            label = f"{cls_name} {conf:.2f}"
            (tw, th), baseline = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
            cv2.rectangle(annotated, (x1, y1 - th - baseline - 4), (x1 + tw + 2, y1), color, -1)

            cv2.putText(
                annotated, label,
                (x1 + 1, y1 - baseline - 2),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                (255, 255, 255), 1, cv2.LINE_AA
            )

    return annotated


def run_on_image(model, source: Path, args):

    results_all = model.predict(
        source=str(source),
        imgsz=args.imgsz,
        conf=args.conf,
        iou=args.iou,
        device=args.device,
        save=args.save,
        save_txt=args.save_txt,
        project=args.project,
        name=args.name,
        exist_ok=True,
        verbose=True,
    )

    if args.show:
        for result in results_all:
            frame     = result.orig_img
            annotated = draw_detections(frame, [result], args.conf)
            cv2.imshow("Traffic Signs Detection", annotated)
            key = cv2.waitKey(0)
            if key == ord('q'):
                break
        cv2.destroyAllWindows()

    print(f"[OK] Rezultate salvate in: {args.project}/{args.name}/")
    return results_all


def run_on_video(model, source, args):
    is_webcam = isinstance(source, int)
    cap = cv2.VideoCapture(source)

    if not cap.isOpened():
        raise RuntimeError(f"Nu pot deschide sursa video: {source}")

    # Setup writer daca se salveaza
    writer = None
    if args.save and not is_webcam:
        src_path = Path(source)
        out_dir  = Path(args.project) / args.name
        out_dir.mkdir(parents=True, exist_ok=True)
        out_path = out_dir / src_path.name

        fps    = cap.get(cv2.CAP_PROP_FPS) or 30
        width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fourcc = cv2.VideoWriter_fourcc(*"mp4v")
        writer = cv2.VideoWriter(str(out_path), fourcc, fps, (width, height))
        print(f"[INFO] Salvez video in: {out_path}")

    frame_idx  = 0
    total_dets = 0

    print("[INFO] Apasa 'q' pentru a opri. Apasa 's' pentru screenshot.")
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            results   = model.predict(
                frame,
                imgsz=args.imgsz,
                conf=args.conf,
                iou=args.iou,
                device=args.device,
                verbose=False,
            )
            annotated = draw_detections(frame, results, args.conf)

            # Numar detectii curente
            n_dets = sum(len(r.boxes) for r in results if r.boxes is not None)
            total_dets += n_dets

            # HUD
            src_label = f"Webcam" if is_webcam else Path(source).name
            cv2.putText(annotated, f"Frame: {frame_idx} | Detectii: {n_dets}",
                        (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (0, 255, 0), 2)
            cv2.putText(annotated, f"Sursa: {src_label} | Conf: {args.conf}",
                        (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (200, 200, 200), 1)

            if writer:
                writer.write(annotated)

            if args.show or is_webcam:
                cv2.imshow("Traffic Signs Detection", annotated)
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q'):
                    print("[INFO] Oprire manuala.")
                    break
                elif key == ord('s'):
                    ss_path = f"screenshot_frame{frame_idx}.jpg"
                    cv2.imwrite(ss_path, annotated)
                    print(f"[OK] Screenshot salvat: {ss_path}")

            frame_idx += 1
    finally:
        cap.release()
        if writer:
            writer.release()
        cv2.destroyAllWindows()

    print(f"\n[OK] Procesate {frame_idx} frame-uri | Total detectii: {total_dets}")


def parse_args():
    parser = argparse.ArgumentParser(description="Inferenta YOLOv11s - Traffic Signs")
    parser.add_argument("--weights",  type=str,   required=True,              help="Calea catre best.pt")
    parser.add_argument("--source",   type=str,   required=True,              help="Sursa: imagine, video, director, '0' pentru webcam")
    parser.add_argument("--imgsz",    type=int,   default=1216,                help="Dimensiune imagine")
    parser.add_argument("--conf",     type=float, default=0.4,               help="Confidence threshold")
    parser.add_argument("--iou",      type=float, default=0.45,               help="IoU NMS threshold")
    parser.add_argument("--device",   type=str,   default="0",                help="GPU sau 'cpu'")
    parser.add_argument("--show",     action="store_true",                    help="Afiseaza in fereastra OpenCV")
    parser.add_argument("--save",     action="store_true", default=True,      help="Salveaza rezultatele")
    parser.add_argument("--save-txt", action="store_true",                    help="Salveaza labels in .txt")
    parser.add_argument("--project",  type=str,   default="runs/detect",      help="Director rezultate")
    parser.add_argument("--name",     type=str,   default="traffic_signs",    help="Nume experiment")
    return parser.parse_args()


def main():
    args = parse_args()

    weights_path = Path(args.weights)
    if not weights_path.exists():
        raise FileNotFoundError(f"Weights negasite: {weights_path}")

    print(f"\n{'='*60}")
    print(f"  YOLOv11s - Traffic Signs Inference")
    print(f"{'='*60}")
    print(f"  Weights: {args.weights}")
    print(f"  Source:  {args.source}")
    print(f"  Conf:    {args.conf}   IoU: {args.iou}")
    print(f"  Device:  {args.device}")
    print(f"{'='*60}\n")

    model = YOLO(args.weights)

    # Determina tipul sursei
    source_str = args.source.strip()

    # Webcam
    if source_str.isdigit():
        run_on_video(model, int(source_str), args)
        return

    source_path = Path(source_str)
    if not source_path.exists():
        raise FileNotFoundError(f"Sursa negasita: {source_path}")

    # Video
    video_exts = {".mp4", ".avi", ".mov", ".mkv", ".webm", ".m4v"}
    if source_path.suffix.lower() in video_exts:
        run_on_video(model, str(source_path), args)
    else:
        run_on_image(model, source_path, args)


if __name__ == "__main__":
    main()