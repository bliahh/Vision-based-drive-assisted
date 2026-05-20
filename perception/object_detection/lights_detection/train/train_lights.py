
import argparse
from pathlib import Path
from ultralytics import YOLO


def parse_args():
    parser = argparse.ArgumentParser(description="Antrenare YOLOv8n - Traffic Lights")
    parser.add_argument("--data",       type=str,   default="config/data.yaml", help="Calea catre data.yaml")
    parser.add_argument("--epochs",     type=int,   default=100,                        help="Numar epoci")
    parser.add_argument("--imgsz",      type=int,   default=640,                        help="Dimensiune imagine")
    parser.add_argument("--batch",      type=int,   default=16,                         help="Batch size (-1 = auto)")
    parser.add_argument("--device",     type=str,   default="0",                        help="GPU index (0) sau 'cpu'")
    parser.add_argument("--workers",    type=int,   default=8,                          help="Numar DataLoader workers")
    parser.add_argument("--project",    type=str,   default="runs/train",               help="Director rezultate")
    parser.add_argument("--name",       type=str,   default="yolov8n_traffic_lights",   help="Nume experiment")
    parser.add_argument("--resume",     action="store_true",                            help="Continua antrenarea din ultimul checkpoint")
    parser.add_argument("--pretrained", type=str,   default="yolov8n.pt",              help="Model pretrained")
    return parser.parse_args()


def main():
    args = parse_args()

    if not Path(args.data).exists():
        raise FileNotFoundError(f"Nu am gasit '{args.data}'.")

    print(f"\n{'='*60}")
    print(f"  YOLOv8n - Traffic Lights Detection (3 clase)")
    print(f"{'='*60}")
    print(f"  Model:    {args.pretrained}")
    print(f"  Data:     {args.data}")
    print(f"  Epoci:    {args.epochs}")
    print(f"  ImgSz:    {args.imgsz}")
    print(f"  Batch:    {args.batch}")
    print(f"  Device:   {args.device}")
    print(f"{'='*60}\n")

    if args.resume:
        last_ckpt = Path(args.project) / args.name / "weights" / "last.pt"
        if not last_ckpt.exists():
            raise FileNotFoundError(f"Checkpoint negasit: {last_ckpt}")
        print(f"[RESUME] Continuand din: {last_ckpt}")
        model = YOLO(str(last_ckpt))
    else:
        model = YOLO(args.pretrained)

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

        hsv_h=0.015,
        hsv_s=0.7,
        hsv_v=0.4,
        degrees=0.0,
        translate=0.1,
        scale=0.5,
        shear=0.0,
        flipud=0.0,
        fliplr=0.5,
        mosaic=1.0,
        mixup=0.0,

        patience=20,

        save=True,
        save_period=10,

        # Logging
        plots=True,
        verbose=True,
        exist_ok=True,
    )

    print(f"\n[OK] ENDED!")
    print(f"[OK] Rezults saved in: {args.project}/{args.name}/")
    print(f"[OK] Best model: {args.project}/{args.name}/weights/best.pt")

    return results


if __name__ == "__main__":
    main()