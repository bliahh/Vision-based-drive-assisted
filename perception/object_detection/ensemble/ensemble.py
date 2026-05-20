# """
# ensemble.py — Vision Drive Assist pipeline
#
# Architecture:
#     HybridNets (lanes + drivable) → async worker
#     YOLOv11m (cars)               → async worker
#     YOLO (signs)                  → async worker
#     IPM auto-calibration          → from lane lines
#     TCP sender                    → JSON to OpenGL HUD
#
# All object/lane coordinates are transformed to real-world meters
# via Inverse Perspective Mapping before being sent over TCP.
# """
#
# import sys
# import argparse
# import time
# from pathlib import Path
#
# ROOT = Path(__file__).resolve().parent.parent.parent
# OBJ_DET    = ROOT / "object_detection"
# TCP_DIR    = ROOT / "TCP"
# MODELS_DIR = ROOT / "models"
#
# for p in [str(OBJ_DET), str(TCP_DIR)]:
#     if p not in sys.path:
#         sys.path.insert(0, p)
#
# import cv2
# import torch
# from ultralytics import YOLO
# from lane_detection.collector import collect_lane_data
# from lane_detection import HybridNetsWorker, load_hybridnets, draw_hybridnets
# from cars_detection import collect_car_data, draw_cars
# from sign_detection import collect_sign_data, draw_signs
# from tcp_sender import TCPSender
# from hud import draw_hud
# from yolo_worker import YOLOWorker
# from ipm import DepthEstimator
#
# from depth_worker import DepthWorker
#
# from cars_detection.detector import CAR_CLASSES, CONF_CARS
# from sign_detection.detector import CONF_SIGNS
#
# DEFAULT_CARS_MODEL  = str(MODELS_DIR / "cars_yolov11m.engine")
# DEFAULT_SIGNS_MODEL = str(MODELS_DIR / "signs.engine")
#
# HN_PUSH_EVERY    = 5
# CARS_PUSH_EVERY  = 1
# SIGNS_PUSH_EVERY = 2
#
#
# def run(signs_path: str,
#         source,
#         no_signs: bool = False,
#         tcp_host: str  = "127.0.0.1",
#         tcp_port: int  = 5005,
#         no_tcp: bool   = False):
#
#     device   = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
#     use_half = device.type == 'cuda'
#     dev_str  = "CUDA" if device.type == 'cuda' else "CPU"
#
#     if device.type == 'cuda':
#         torch.cuda.set_device(0)
#
#     # ── Load models ──────────────────────────────────────────────
#     hn_model = load_hybridnets(device)
#
#     print(f"[INFO] Loading cars model: {DEFAULT_CARS_MODEL}...")
#     model_cars = YOLO(DEFAULT_CARS_MODEL)
#
#     model_signs = None
#     if not no_signs:
#         signs_path = signs_path or DEFAULT_SIGNS_MODEL
#         if Path(signs_path).exists():
#             print(f"[INFO] Loading signs model: {signs_path}...")
#             model_signs = YOLO(signs_path)
#         else:
#             print(f"[WARN] Signs model missing: {signs_path} -> disabled")
#
#     # ── Start workers ────────────────────────────────────────────
#     hn_worker = HybridNetsWorker(hn_model, device, use_half)
#     hn_worker.start()
#     print("[OK] HybridNets worker started")
#
#     cars_worker = YOLOWorker(
#         model_cars,
#         name="CarsWorker",
#         conf=CONF_CARS,
#         classes=list(CAR_CLASSES.keys()),
#         imgsz=640,
#         iou=0.45,
#         agnostic_nms=True,
#     )
#     cars_worker.start()
#     print("[OK] Cars worker started")
#
#     signs_worker = None
#     if model_signs is not None:
#         signs_worker = YOLOWorker(
#             model_signs,
#             name="SignsWorker",
#             conf=CONF_SIGNS,
#             imgsz=640,
#         )
#         signs_worker.start()
#         print("[OK] Signs worker started")
#
#     # ── IPM (auto-calibrates from lane lines) ────────────────────
#     ipm = DepthEstimator(device="cuda" if torch.cuda.is_available() else "cpu")
#
#     # ── TCP sender ───────────────────────────────────────────────
#     tcp = None
#     if not no_tcp:
#         tcp = TCPSender(host=tcp_host, port=tcp_port)
#         tcp.start()
#         print(f"[INFO] TCP -> {tcp_host}:{tcp_port}")
#
#     # ── Open video source ────────────────────────────────────────
#     cap = cv2.VideoCapture(source)
#     if not cap.isOpened():
#         print(f"[ERR] Cannot open source: {source}")
#         _shutdown(hn_worker, cars_worker, signs_worker, tcp)
#         sys.exit(1)
#
#     show_da    = True
#     show_lane  = True
#     show_cars  = True
#     show_signs = model_signs is not None
#     debug      = False
#     frame_cnt  = 0
#     fps        = 0.0
#     t_start    = time.time()
#     shot_idx   = 0
#
#     ret, frame0 = cap.read()
#     if not ret:
#         print(f"[ERR] Cannot read first frame")
#         _shutdown(hn_worker, cars_worker, signs_worker, tcp)
#         sys.exit(1)
#
#     # ── Warmup ───────────────────────────────────────────────────
#     print("[INFO] Warmup GPU...")
#     hn_worker.push_frame(frame0)
#     cars_worker.push_frame(frame0)
#     if signs_worker:
#         signs_worker.push_frame(frame0)
#     time.sleep(1.5)
#     print("[OK] Warmup done")
#
#     print("[INFO] Controls:")
#     print("  1: DA toggle   2: Lane toggle")
#     print("  3: Cars toggle  4: Signs toggle")
#     print("  D: Debug  S: Screenshot  Q: Quit")
#
#     # ── Main loop ────────────────────────────────────────────────
#     i = 0
#     while True:
#         ret, frame = cap.read()
#         if not ret:
#             print("[INFO] Stream ended")
#             break
#
#         # Push frames to workers
#         if i % HN_PUSH_EVERY == 0:
#             hn_worker.push_frame(frame)
#         if show_cars and i % CARS_PUSH_EVERY == 0:
#             cars_worker.push_frame(frame)
#         if show_signs and signs_worker and i % SIGNS_PUSH_EVERY == 0:
#             signs_worker.push_frame(frame)
#         i += 1
#
#         # ── Get results ──────────────────────────────────────────
#         mask_da, mask_lane = hn_worker.get_masks()
#         frame = draw_hybridnets(frame, mask_da, mask_lane, show_da, show_lane)
#
#         res_cars = cars_worker.get_results() if show_cars else None
#         n_cars = 0
#         if res_cars is not None and show_cars:
#             frame, n_cars = draw_cars(frame, res_cars, debug)
#
#         res_signs = signs_worker.get_results() if (show_signs and signs_worker) else None
#         n_signs = 0
#         if res_signs is not None and show_signs:
#             frame, n_signs = draw_signs(frame, res_signs, model_signs.names, debug)
#
#         # ── FPS ──────────────────────────────────────────────────
#         frame_cnt += 1
#         elapsed = time.time() - t_start
#         if elapsed >= 0.5:
#             fps       = frame_cnt / elapsed
#             frame_cnt = 0
#             t_start   = time.time()
#
#         # ── TCP send (with IPM transform) ────────────────────────
#         if tcp is not None:
#
#             ipm.process_frame(frame)
#             car_data = collect_car_data(res_cars)
#             sign_data = (collect_sign_data(res_signs, model_signs.names)
#                          if (res_signs is not None and model_signs) else [])
#             lane_data = collect_lane_data(mask_da, mask_lane, frame.shape)
#
#             # Auto-calibrate IPM from lane lines (first ~10 frames)
#
#
#             # Transform all coordinates to meters
#             if ipm.calibrated:
#                 # Cars: add x_m, z_m fields
#                 for car in car_data:
#                     bx = car["box"]
#                     w = ipm.transform_bbox(bx[0], bx[1], bx[2], bx[3])
#                     car["x_m"] = w["x_m"]
#                     car["z_m"] = w["z_m"]
#
#                 # Lane lines: [nx, ny] → [x_m, z_m]
#                 if "lane_lines" in lane_data:
#                     for idx, line in enumerate(lane_data["lane_lines"]):
#                         lane_data["lane_lines"][idx] = ipm.transform_lane_points(line)
#
#                 # Drivable polygons: [nx, ny] → [x_m, z_m]
#                 if "drivable" in lane_data:
#                     for idx, poly in enumerate(lane_data["drivable"]):
#                         lane_data["drivable"][idx] = ipm.transform_lane_points(poly)
#
#                 lane_data["ipm"] = True
#
#             tcp.send({
#                 "ts":    round(time.time(), 3),
#                 "fps":   round(fps, 1),
#                 "cars":  car_data,
#                 "signs": sign_data,
#                 "lane":  lane_data,
#             })
#
#         # ── HUD overlay ──────────────────────────────────────────
#         frame = draw_hud(
#             frame, fps,
#             show_da, show_lane, show_cars, show_signs,
#             n_cars, n_signs, debug, dev_str,
#             hn_worker.latency_ms,
#             tcp.connected if tcp is not None else False,
#         )
#
#         cv2.imshow("Vision Drive Assist", frame)
#
#         # ── Key handling ─────────────────────────────────────────
#         key = cv2.waitKey(1) & 0xFF
#         if key in (ord('q'), 27):
#             break
#         elif key == ord('1'): show_da    = not show_da
#         elif key == ord('2'): show_lane  = not show_lane
#         elif key == ord('3'): show_cars  = not show_cars
#         elif key == ord('4'): show_signs = not show_signs
#         elif key == ord('d'):
#             debug = not debug
#             print(f"Debug: {'ON' if debug else 'OFF'}")
#         elif key == ord('s'):
#             fname = f"drive_assist_{shot_idx:04d}.png"
#             cv2.imwrite(fname, frame)
#             print(f"Screenshot: {fname}")
#             shot_idx += 1
#
#     _shutdown(hn_worker, cars_worker, signs_worker, tcp)
#     cap.release()
#     cv2.destroyAllWindows()
#
#
# def _shutdown(hn_worker, cars_worker, signs_worker, tcp):
#     """Graceful shutdown of all workers."""
#     hn_worker.stop()
#     hn_worker.join(timeout=2.0)
#     cars_worker.stop()
#     cars_worker.join(timeout=2.0)
#     if signs_worker is not None:
#         signs_worker.stop()
#         signs_worker.join(timeout=2.0)
#     if tcp is not None:
#         tcp.stop()
#         tcp.join(timeout=2.0)
#
#
# if __name__ == "__main__":
#     p = argparse.ArgumentParser(description="Vision Drive Assist")
#     p.add_argument("--signs",    type=str, default=None,  help="Path to signs model")
#     p.add_argument("--video",    type=str, default=None,  help="Input video path")
#     p.add_argument("--camera",   type=int, default=0,     help="Camera index")
#     p.add_argument("--no-signs", action="store_true",     help="Disable sign detection")
#     p.add_argument("--tcp-host", type=str, default="127.0.0.1", help="TCP host")
#     p.add_argument("--tcp-port", type=int, default=5005,  help="TCP port")
#     p.add_argument("--no-tcp",   action="store_true",     help="Disable TCP sender")
#     args = p.parse_args()
#
#     source = args.video if args.video else args.camera
#     if args.video and not Path(args.video).exists():
#         print(f"[ERR] Video not found: {args.video}")
#         sys.exit(1)
#
#     run(
#         signs_path=args.signs,
#         source=source,
#         no_signs=args.no_signs,
#         tcp_host=args.tcp_host,
#         tcp_port=args.tcp_port,
#         no_tcp=args.no_tcp,
#     )



"""
ensemble.py — Vision Drive Assist pipeline

Architecture:
    HybridNets (lanes + drivable) → async worker
    YOLOv11m (cars)               → async worker
    YOLO (signs)                  → async worker
    Depth Anything V2             → async worker
    TCP sender                    → JSON to OpenGL HUD

All object/lane coordinates are transformed to real-world meters
via Depth Anything V2 before being sent over TCP.
"""

import sys
import argparse
import time
from pathlib import Path

ROOT       = Path(__file__).resolve().parent.parent.parent
OBJ_DET    = ROOT / "object_detection"
TCP_DIR    = ROOT / "TCP"
MODELS_DIR = ROOT / "models"

for p in [str(OBJ_DET), str(TCP_DIR)]:
    if p not in sys.path:
        sys.path.insert(0, p)

import cv2
import torch
from ultralytics import YOLO
from lane_detection.collector import collect_lane_data
from lane_detection import HybridNetsWorker, load_hybridnets, draw_hybridnets
from cars_detection import collect_car_data, draw_cars
from sign_detection import collect_sign_data, draw_signs
from tcp_sender import TCPSender
from hud import draw_hud
from yolo_worker import YOLOWorker
from depth_worker import DepthWorker

from cars_detection.detector import CAR_CLASSES, CONF_CARS
from sign_detection.detector import CONF_SIGNS

DEFAULT_CARS_MODEL  = str(MODELS_DIR / "cars_yolov11m.engine")
DEFAULT_SIGNS_MODEL = str(MODELS_DIR / "signs.engine")

HN_PUSH_EVERY    = 5
CARS_PUSH_EVERY  = 1
SIGNS_PUSH_EVERY = 2
DEPTH_PUSH_EVERY = 3


def run(signs_path: str,
        source,
        no_signs: bool = False,
        tcp_host: str  = "127.0.0.1",
        tcp_port: int  = 5005,
        no_tcp: bool   = False):

    device   = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    use_half = device.type == 'cuda'
    dev_str  = "CUDA" if device.type == 'cuda' else "CPU"

    if device.type == 'cuda':
        torch.cuda.set_device(0)

    # ── Load models ──────────────────────────────────────────────
    hn_model = load_hybridnets(device)

    print(f"[INFO] Loading cars model: {DEFAULT_CARS_MODEL}...")
    model_cars = YOLO(DEFAULT_CARS_MODEL)

    model_signs = None
    if not no_signs:
        signs_path = signs_path or DEFAULT_SIGNS_MODEL
        if Path(signs_path).exists():
            print(f"[INFO] Loading signs model: {signs_path}...")
            model_signs = YOLO(signs_path)
        else:
            print(f"[WARN] Signs model missing: {signs_path} -> disabled")

    # ── Start workers ────────────────────────────────────────────
    hn_worker = HybridNetsWorker(hn_model, device, use_half)
    hn_worker.start()
    print("[OK] HybridNets worker started")

    cars_worker = YOLOWorker(
        model_cars,
        name="CarsWorker",
        conf=CONF_CARS,
        classes=list(CAR_CLASSES.keys()),
        imgsz=640,
        iou=0.45,
        agnostic_nms=True,
    )
    cars_worker.start()
    print("[OK] Cars worker started")

    signs_worker = None
    if model_signs is not None:
        signs_worker = YOLOWorker(
            model_signs,
            name="SignsWorker",
            conf=CONF_SIGNS,
            imgsz=640,
        )
        signs_worker.start()
        print("[OK] Signs worker started")

    depth_worker = DepthWorker(
        device="cuda" if torch.cuda.is_available() else "cpu",
        input_size=(640, 360),
    )
    depth_worker.start()
    print("[OK] Depth worker started")

    # ── TCP sender ───────────────────────────────────────────────
    tcp = None
    if not no_tcp:
        tcp = TCPSender(host=tcp_host, port=tcp_port)
        tcp.start()
        print(f"[INFO] TCP -> {tcp_host}:{tcp_port}")

    # ── Open video source ────────────────────────────────────────
    cap = cv2.VideoCapture(source)
    if not cap.isOpened():
        print(f"[ERR] Cannot open source: {source}")
        _shutdown(hn_worker, cars_worker, signs_worker, depth_worker, tcp)
        sys.exit(1)

    show_da    = True
    show_lane  = True
    show_cars  = True
    show_signs = model_signs is not None
    debug      = False
    frame_cnt  = 0
    fps        = 0.0
    t_start    = time.time()
    shot_idx   = 0

    ret, frame0 = cap.read()
    if not ret:
        print("[ERR] Cannot read first frame")
        _shutdown(hn_worker, cars_worker, signs_worker, depth_worker, tcp)
        sys.exit(1)

    # ── Warmup ───────────────────────────────────────────────────
    print("[INFO] Warmup GPU...")
    hn_worker.push_frame(frame0)
    cars_worker.push_frame(frame0)
    depth_worker.push_frame(frame0)
    if signs_worker:
        signs_worker.push_frame(frame0)
    time.sleep(1.5)
    print("[OK] Warmup done")

    print("[INFO] Controls:")
    print("  1: DA toggle   2: Lane toggle")
    print("  3: Cars toggle  4: Signs toggle")
    print("  D: Debug  S: Screenshot  Q: Quit")

    # ── Main loop ────────────────────────────────────────────────
    i = 0
    while True:
        ret, frame = cap.read()
        if not ret:
            print("[INFO] Stream ended")
            break

        # Push frames to workers
        if i % HN_PUSH_EVERY == 0:
            hn_worker.push_frame(frame)
        if show_cars and i % CARS_PUSH_EVERY == 0:
            cars_worker.push_frame(frame)
        if show_signs and signs_worker and i % SIGNS_PUSH_EVERY == 0:
            signs_worker.push_frame(frame)
        if i % DEPTH_PUSH_EVERY == 0:
            depth_worker.push_frame(frame)
        i += 1

        # ── Get results ──────────────────────────────────────────
        mask_da, mask_lane = hn_worker.get_masks()
        frame = draw_hybridnets(frame, mask_da, mask_lane, show_da, show_lane)

        res_cars = cars_worker.get_results() if show_cars else None
        n_cars   = 0
        if res_cars is not None and show_cars:
            frame, n_cars = draw_cars(frame, res_cars, debug)

        res_signs = signs_worker.get_results() if (show_signs and signs_worker) else None
        n_signs   = 0
        if res_signs is not None and show_signs:
            frame, n_signs = draw_signs(frame, res_signs, model_signs.names, debug)

        # ── FPS ──────────────────────────────────────────────────
        frame_cnt += 1
        elapsed    = time.time() - t_start
        if elapsed >= 0.5:
            fps       = frame_cnt / elapsed
            frame_cnt = 0
            t_start   = time.time()

        # ── TCP send ─────────────────────────────────────────────
        if tcp is not None:
            car_data  = collect_car_data(res_cars)
            sign_data = (collect_sign_data(res_signs, model_signs.names)
                         if (res_signs is not None and model_signs) else [])
            lane_data = collect_lane_data(mask_da, mask_lane, frame.shape)

            if depth_worker.calibrated:
                for car in car_data:
                    bx        = car["box"]
                    w         = depth_worker.transform_bbox(bx[0], bx[1], bx[2], bx[3])
                    car["x_m"] = w["x_m"]
                    car["z_m"] = w["z_m"]

                if "lane_lines" in lane_data:
                    for idx, line in enumerate(lane_data["lane_lines"]):
                        lane_data["lane_lines"][idx] = depth_worker.transform_lane_points(line)

                if "drivable" in lane_data:
                    for idx, poly in enumerate(lane_data["drivable"]):
                        lane_data["drivable"][idx] = depth_worker.transform_lane_points(poly)

                lane_data["ipm"] = True

            tcp.send({
                "ts":    round(time.time(), 3),
                "fps":   round(fps, 1),
                "cars":  car_data,
                "signs": sign_data,
                "lane":  lane_data,
            })

        # ── HUD overlay ──────────────────────────────────────────
        frame = draw_hud(
            frame, fps,
            show_da, show_lane, show_cars, show_signs,
            n_cars, n_signs, debug, dev_str,
            hn_worker.latency_ms,
            tcp.connected if tcp is not None else False,
        )

        cv2.imshow("Vision Drive Assist", frame)

        # ── Key handling ─────────────────────────────────────────
        key = cv2.waitKey(1) & 0xFF
        if key in (ord('q'), 27):
            break
        elif key == ord('1'): show_da    = not show_da
        elif key == ord('2'): show_lane  = not show_lane
        elif key == ord('3'): show_cars  = not show_cars
        elif key == ord('4'): show_signs = not show_signs
        elif key == ord('d'):
            debug = not debug
            print(f"Debug: {'ON' if debug else 'OFF'}")
        elif key == ord('s'):
            fname = f"drive_assist_{shot_idx:04d}.png"
            cv2.imwrite(fname, frame)
            print(f"Screenshot: {fname}")
            shot_idx += 1

    _shutdown(hn_worker, cars_worker, signs_worker, depth_worker, tcp)
    cap.release()
    cv2.destroyAllWindows()


def _shutdown(hn_worker, cars_worker, signs_worker, depth_worker, tcp):
    """Graceful shutdown of all workers."""
    hn_worker.stop()
    hn_worker.join(timeout=2.0)
    cars_worker.stop()
    cars_worker.join(timeout=2.0)
    if signs_worker is not None:
        signs_worker.stop()
        signs_worker.join(timeout=2.0)
    depth_worker.stop()
    depth_worker.join(timeout=2.0)
    if tcp is not None:
        tcp.stop()
        tcp.join(timeout=2.0)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description="Vision Drive Assist")
    p.add_argument("--signs",    type=str, default=None,        help="Path to signs model")
    p.add_argument("--video",    type=str, default=None,        help="Input video path")
    p.add_argument("--camera",   type=int, default=0,           help="Camera index")
    p.add_argument("--no-signs", action="store_true",           help="Disable sign detection")
    p.add_argument("--tcp-host", type=str, default="127.0.0.1", help="TCP host")
    p.add_argument("--tcp-port", type=int, default=5005,        help="TCP port")
    p.add_argument("--no-tcp",   action="store_true",           help="Disable TCP sender")
    args = p.parse_args()

    source = args.video if args.video else args.camera
    if args.video and not Path(args.video).exists():
        print(f"[ERR] Video not found: {args.video}")
        sys.exit(1)

    run(
        signs_path=args.signs,
        source=source,
        no_signs=args.no_signs,
        tcp_host=args.tcp_host,
        tcp_port=args.tcp_port,
        no_tcp=args.no_tcp,
    )