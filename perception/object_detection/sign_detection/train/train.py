"""
train.py - Antrenare YOLOv11s pentru detectie semne de circulatie
Dataset: traffic-signs-detection-europe (55 clase)
"""

import os
import argparse
from pathlib import Path
from ultralytics import YOLO


def parse_args():
    parser = argparse.ArgumentParser(description="Antrenare YOLOv11s - Traffic Signs")
    parser.add_argument("--data",       type=str,   default="config/data.yaml",    help="Calea catre data.yaml")
    parser.add_argument("--epochs",     type=int,   default=100,            help="Numar epoci")
    parser.add_argument("--imgsz",      type=int,   default=640,            help="Dimensiune imagine")
    parser.add_argument("--batch",      type=int,   default=16,             help="Batch size (-1 = auto)")
    parser.add_argument("--device",     type=str,   default="0",            help="GPU index (0) sau 'cpu'")
    parser.add_argument("--workers",    type=int,   default=8,              help="Numar DataLoader workers")
    parser.add_argument("--project",    type=str,   default="runs/train",   help="Director rezultate")
    parser.add_argument("--name",       type=str,   default="yolo11s_traffic_signs", help="Nume experiment")
    parser.add_argument("--resume",     action="store_true",                help="Continua antrenarea din ultimul checkpoint")
    parser.add_argument("--pretrained", type=str,   default="yolo11s.pt",   help="Model pretrained")
    return parser.parse_args()


def main():
    args = parse_args()

    # Verifica existenta data.yaml
    if not Path(args.data).exists():
        raise FileNotFoundError(
            f"Nu am gasit '{args.data}'.\n"
            "Descarca dataset-ul de pe Roboflow si pune data.yaml in directorul curent.\n"
            "Comanda Roboflow CLI:\n"
            "  pip install roboflow\n"
            "  python -c \"\n"
            "  from roboflow import Roboflow\n"
            "  rf = Roboflow(api_key='YOUR_API_KEY')\n"
            "  project = rf.workspace('radu-oprea-r4xnm').project('traffic-signs-detection-europe')\n"
            "  version = project.version(14)\n"
            "  dataset = version.download('yolov11')\n"
            "  \""
        )

    print(f"\n{'='*60}")
    print(f"  YOLOv11s - Traffic Signs Detection (55 clase)")
    print(f"{'='*60}")
    print(f"  Model:    {args.pretrained}")
    print(f"  Data:     {args.data}")
    print(f"  Epoci:    {args.epochs}")
    print(f"  ImgSz:    {args.imgsz}")
    print(f"  Batch:    {args.batch}")
    print(f"  Device:   {args.device}")
    print(f"{'='*60}\n")

    # Incarca modelul
    if args.resume:
        last_ckpt = Path(args.project) / args.name / "weights" / "last.pt"
        if not last_ckpt.exists():
            raise FileNotFoundError(f"Checkpoint negasit: {last_ckpt}")
        print(f"[RESUME] Continuand din: {last_ckpt}")
        model = YOLO(str(last_ckpt))
    else:
        model = YOLO(args.pretrained)

    # Antrenare
    results = model.train(
        data=args.data,
        epochs=args.epochs,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device,
        workers=args.workers,
        project=args.project,
        name=args.name,
        resume=args.resume,

        # Optimizer
        optimizer="AdamW",
        lr0=0.001,
        lrf=0.01,
        momentum=0.937,
        weight_decay=0.0005,
        warmup_epochs=3,

        # Augmentari
        hsv_h=0.015,
        hsv_s=0.7,
        hsv_v=0.4,
        degrees=5.0,
        translate=0.1,
        scale=0.5,
        shear=2.0,
        flipud=0.0,
        fliplr=0.5,
        mosaic=1.0,
        mixup=0.1,

        # Early stopping
        patience=20,

        # Salvare
        save=True,
        save_period=10,  # salveaza checkpoint la fiecare 10 epoci

        # Logging
        plots=True,
        verbose=True,
        exist_ok=True,
    )

    print(f"\n[OK] Antrenare finalizata!")
    print(f"[OK] Rezultate salvate in: {args.project}/{args.name}/")
    print(f"[OK] Best model: {args.project}/{args.name}/weights/best.pt")

    return results


if __name__ == "__main__":
    main()