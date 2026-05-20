import os
import torch
import cv2
import sys
import time
from collections import defaultdict

print(torch.cuda.is_available())
print(torch.cuda.get_device_name(0))

os.environ['KMP_DUPLICATE_LIB_OK'] = 'TRUE'



try:
    from ultralytics import YOLO
except ImportError:
    import subprocess

    subprocess.check_call([sys.executable, "-m", "pip", "install", "ultralytics", "-q"])
    from ultralytics import YOLO

BASE_DIR   = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

VIDEO_FOLDER = os.path.join(BASE_DIR, "video")
MODEL_PATH   = os.path.join(BASE_DIR, "models", "yolov8m.pt")
OUTPUT_FILE  = os.path.join(BASE_DIR, "vehicle_detection_output.avi")




VEHICLE_CLASSES = [0, 1, 2, 3, 5, 6, 7]
CONF_THRESHOLD = 0.25
HEAVY_TRAFFIC_THRESHOLD = 10
LANE_THRESHOLD = 609
LINE_Y = 400

FONT = cv2.FONT_HERSHEY_SIMPLEX
FONT_COLOR = (255, 255, 255)
BG_COLOR = (0, 0, 255)



def estimate_distance(bbox_width: int) -> float:
    focal_length = 1000
    known_width = 2.0
    return (known_width * focal_length) / bbox_width if bbox_width > 0 else 0.0

def draw_label(frame, text, pos, width=420, scale=0.7):
    x, y = pos
    cv2.rectangle(frame, (x - 10, y - 25), (x + width, y + 10), BG_COLOR, -1)
    cv2.putText(frame, text, pos, FONT, scale, FONT_COLOR, 2, cv2.LINE_AA)


def find_video_file(folder: str) -> str:
    supported_extensions = ('.mp4', '.avi', '.mov', '.mkv', '.wmv', '.flv', '.webm')

    if not os.path.isdir(folder):
        print(f"[ERROR] Folder:   '{folder}' not found")
        sys.exit(1)

    for filename in sorted(os.listdir(folder)):
        if filename.lower().endswith(supported_extensions):
            full_path = os.path.join(folder, filename)
            print(f"[OK] Video găsit: {full_path}")
            return full_path

    print(f"[ERROR] Folder: '{folder}' is empty.")
    print(f"[INFO] SUPPORTED EXTENSION: {supported_extensions}")
    sys.exit(1)


def open_capture(folder: str) -> cv2.VideoCapture:
    video_path = find_video_file(folder)
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"[ERROR] can't open: {video_path}")
        sys.exit(1)
    return cap


def main():
    cap = open_capture(VIDEO_FOLDER)

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = cap.get(cv2.CAP_PROP_FPS) or 30.0
    frame_delay = 1.0 / fps
    print(f"RESOLUTION : {width}x{height} @ {fps:.1f} FPS")

    print(f"LOAD MODEL: {MODEL_PATH}")
    if not os.path.isfile(MODEL_PATH):
        print(f"[ERROR] MODEL NOT FOUND: {MODEL_PATH}")
        sys.exit(1)
    model = YOLO(MODEL_PATH)
    names = model.names

    fourcc = cv2.VideoWriter_fourcc(*"XVID")
    out = cv2.VideoWriter(OUTPUT_FILE, fourcc, fps, (width, height))
    print(f"Output: {OUTPUT_FILE}\n")

    crossed_ids = set()
    class_counts = defaultdict(int)
    frame_id = 0

    while cap.isOpened():
        frame_start = time.time()

        ret, frame = cap.read()
        if not ret:
            print("VIDEO ENDED.")
            break

        frame_id += 1

        results = model.track(
            frame,
            persist=True,
            classes=VEHICLE_CLASSES,
            conf=CONF_THRESHOLD,
            verbose=False,
            imgsz=1280,
            device="cuda",
            half=True,
            tracker="botsort.yaml"
        )

        vehicles_left = 0
        vehicles_right = 0

        cv2.line(frame, (0, LINE_Y), (width, LINE_Y), (0, 0, 255), 2)
        cv2.putText(frame, "tresh. line", (10, LINE_Y - 8),
                    FONT, 0.5, (0, 0, 255), 1, cv2.LINE_AA)

        if results[0].boxes is not None and len(results[0].boxes) > 0:
            boxes = results[0].boxes.xyxy.cpu()
            class_indices = results[0].boxes.cls.int().cpu().tolist()
            confs = results[0].boxes.conf.cpu().tolist()
            track_ids = (results[0].boxes.id.int().cpu().tolist()
                         if results[0].boxes.id is not None
                         else [-1] * len(boxes))

            for box, track_id, class_idx, conf in zip(boxes, track_ids, class_indices, confs):
                x1, y1, x2, y2 = map(int, box)
                cx = (x1 + x2) // 2
                cy = (y1 + y2) // 2
                class_name = names[class_idx]

                if cx < LANE_THRESHOLD:
                    vehicles_right += 1
                else:
                    vehicles_left += 1

                if cy > LINE_Y and track_id not in crossed_ids and track_id != -1:
                    crossed_ids.add(track_id)
                    class_counts[class_name] += 1

                bbox_width = x2 - x1
                distance = estimate_distance(bbox_width)

                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 255), 2)
                cv2.circle(frame, (cx, cy), 4, (0, 0, 255), -1)

                label = f"ID:{track_id} {class_name} {conf:.2f}"
                cv2.putText(frame, label, (x1, y1 - 8),
                            FONT, 0.45, (0, 255, 255), 1, cv2.LINE_AA)

                dist_label = f"Dist: {distance:.1f}m"
                cv2.putText(frame, dist_label, (x1, y2 + 18),
                            FONT, 0.45, (255, 0, 0), 1, cv2.LINE_AA)

        y_off = 30
        for cls_name, count in class_counts.items():
            cv2.putText(frame, f"{cls_name}: {count}",
                        (10, y_off), FONT, 0.7, (0, 255, 255), 2, cv2.LINE_AA)
            y_off += 28

        intens_left = "Heavy" if vehicles_left > HEAVY_TRAFFIC_THRESHOLD else "Smooth"
        intens_right = "Heavy" if vehicles_right > HEAVY_TRAFFIC_THRESHOLD else "Smooth"

        draw_label(frame, f"Left Lane:  {vehicles_left}  [{intens_left}]",
                   (10, height - 70), width=380)
        draw_label(frame, f"Right Lane: {vehicles_right}  [{intens_right}]",
                   (10, height - 30), width=380)

        elapsed = time.time() - frame_start
        actual_fps = 1.0 / elapsed if elapsed > 0 else 0
        cv2.putText(frame, f"FPS: {actual_fps:.1f}",
                    (width - 120, height - 10), FONT, 0.6, (0, 255, 0), 2)

        out.write(frame)
        cv2.imshow("Vehicle Detection", frame)

        sleep_time = frame_delay - (time.time() - frame_start)
        if sleep_time > 0:
            time.sleep(sleep_time)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            print("STOP MANUALLY (q).")
            break

    cap.release()
    out.release()
    cv2.destroyAllWindows()

    print(f"\n[OK!] output saved at: {OUTPUT_FILE}")



if __name__ == "__main__":
    main()