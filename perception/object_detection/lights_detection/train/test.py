import cv2
import argparse
from ultralytics import YOLO

COLORS = {
    "green":  (0, 255, 0),
    "red":    (0, 0, 255),
    "yellow": (0, 255, 255),
}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model",  required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--conf",   type=float, default=0.25)
    args = parser.parse_args()

    print(f"[INFO] Loading model: {args.model}")
    model = YOLO(args.model, task="detect")
    print(f"[INFO] Classes: {model.names}")

    source = int(args.source) if args.source.isdigit() else args.source
    cap = cv2.VideoCapture(source)

    if not cap.isOpened():
        print(f"[EROARE] can't open source: {args.source}")
        return

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        results = model(frame, conf=args.conf)[0]

        for box in results.boxes:
            x1, y1, x2, y2 = map(int, box.xyxy[0])
            conf    = float(box.conf[0])
            cls_id  = int(box.cls[0])
            label   = model.names[cls_id]
            color   = COLORS.get(label, (255, 255, 255))

            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
            cv2.putText(
                frame,
                f"{label} {conf:.2f}",
                (x1, y1 - 5),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6, color, 2
            )

        cv2.imshow("Traffic Lights Detection", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
