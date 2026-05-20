"""
export.py - Export YOLOv11s catre ONNX sau TensorRT
Foloseste modelul antrenat (best.pt) si il optimizeaza pentru inferenta rapida.
"""

import argparse
from pathlib import Path
from ultralytics import YOLO


def parse_args():
    parser = argparse.ArgumentParser(description="Export YOLOv11s - ONNX / TensorRT")
    parser.add_argument("--weights",   type=str,   required=True,      help="Calea catre best.pt")
    parser.add_argument("--format",    type=str,   default="onnx",
                        choices=["onnx", "engine", "torchscript", "openvino", "tflite"],
                        help="Format export: onnx, engine (TensorRT), torchscript, openvino, tflite")
    parser.add_argument("--imgsz",     type=int,   default=640,        help="Dimensiune imagine")
    parser.add_argument("--device",    type=str,   default="0",        help="GPU sau 'cpu'")
    parser.add_argument("--half",      action="store_true",            help="FP16 (recomandat GPU)")
    parser.add_argument("--int8",      action="store_true",            help="INT8 (TensorRT, nevoie calibrare)")
    parser.add_argument("--batch",     type=int,   default=1,          help="Batch size static")
    parser.add_argument("--dynamic",   action="store_true",            help="Batch dinamic (ONNX)")
    parser.add_argument("--simplify",  action="store_true", default=True, help="Simplifica ONNX graph (onnx-simplifier)")
    parser.add_argument("--opset",     type=int,   default=17,         help="ONNX opset version")
    parser.add_argument("--workspace", type=int,   default=4,          help="TensorRT workspace (GB)")
    return parser.parse_args()


def print_format_info(fmt: str):
    info = {
        "onnx": (
            "ONNX Export\n"
            "  - Compatibil cu: ONNXRuntime, OpenVINO, TensorRT, TFLite\n"
            "  - Bun pentru: deployment cross-platform\n"
            "  - Rulare: python detect_onnx.py  sau  onnxruntime"
        ),
        "engine": (
            "TensorRT Engine Export\n"
            "  - Optimizat pentru NVIDIA GPU (cel mai rapid)\n"
            "  - ATENTIE: engine-ul este specific GPU-ului si CUDA version!\n"
            "  - FP16 recomandat pentru viteza maxima fara pierdere de acuratete\n"
            "  - INT8 necesita dataset de calibrare"
        ),
        "torchscript": (
            "TorchScript Export\n"
            "  - Compatibil cu: LibTorch (C++), mobile\n"
            "  - Bun pentru: integrare C++ sau embedded"
        ),
        "openvino": (
            "OpenVINO Export\n"
            "  - Optimizat pentru Intel CPU/GPU/VPU\n"
            "  - Bun pentru: deployment pe hardware Intel"
        ),
        "tflite": (
            "TFLite Export\n"
            "  - Optimizat pentru mobile / edge devices\n"
            "  - Bun pentru: Android, Raspberry Pi"
        ),
    }
    print(f"\n[INFO] {info.get(fmt, 'Format necunoscut')}\n")


def verify_export(export_path: Path, fmt: str):
    """Verifica fisierul exportat si afiseaza dimensiunea."""
    if not export_path.exists():
        # TensorRT poate produce un director
        candidates = list(export_path.parent.glob(f"*{export_path.stem}*"))
        if candidates:
            export_path = candidates[0]

    if export_path.exists():
        if export_path.is_dir():
            size_mb = sum(f.stat().st_size for f in export_path.rglob("*") if f.is_file()) / 1e6
        else:
            size_mb = export_path.stat().st_size / 1e6
        print(f"[OK] Export verificat: {export_path}  ({size_mb:.1f} MB)")
        return True
    else:
        print(f"[WARN] Fisierul exportat nu a fost gasit la: {export_path}")
        return False


def get_export_path(weights: Path, fmt: str) -> Path:
    ext_map = {
        "onnx":        ".onnx",
        "engine":      ".engine",
        "torchscript": ".torchscript",
        "openvino":    "_openvino_model",
        "tflite":      "_saved_model",
    }
    ext = ext_map.get(fmt, f".{fmt}")
    return weights.parent / (weights.stem + ext)


def main():
    args = parse_args()

    weights_path = Path(args.weights)
    if not weights_path.exists():
        raise FileNotFoundError(f"Weights negasite: {weights_path}")

    print_format_info(args.format)

    print(f"{'='*60}")
    print(f"  Export YOLOv11s")
    print(f"{'='*60}")
    print(f"  Weights:  {args.weights}")
    print(f"  Format:   {args.format.upper()}")
    print(f"  ImgSz:    {args.imgsz}")
    print(f"  Batch:    {args.batch}{'  (dinamic)' if args.dynamic else ''}")
    print(f"  FP16:     {args.half}")
    print(f"  INT8:     {args.int8}")
    print(f"  Device:   {args.device}")
    print(f"{'='*60}\n")

    model = YOLO(args.weights)

    export_kwargs = dict(
        format=args.format,
        imgsz=args.imgsz,
        device=args.device,
        half=args.half,
        int8=args.int8,
        batch=args.batch,
        dynamic=args.dynamic,
        simplify=args.simplify if args.format == "onnx" else False,
        opset=args.opset if args.format == "onnx" else None,
        workspace=args.workspace if args.format == "engine" else None,
        verbose=True,
    )

    # Elimina None
    export_kwargs = {k: v for k, v in export_kwargs.items() if v is not None}

    try:
        exported_path = model.export(**export_kwargs)
        print(f"\n[OK] Export finalizat: {exported_path}")
    except Exception as e:
        print(f"\n[ERROR] Export esuat: {e}")
        raise

    # Verifica exportul
    expected = get_export_path(weights_path, args.format)
    verify_export(expected, args.format)

    # Instructiuni de utilizare
    print(f"\n{'='*60}")
    print(f"  CUM SA FOLOSESTI MODELUL EXPORTAT")
    print(f"{'='*60}")

    if args.format == "onnx":
        print(f"""
  1. Cu Ultralytics (recomandat):
     python detect.py --weights {expected} --source imagine.jpg

  2. Cu ONNXRuntime direct:
     import onnxruntime as ort
     import numpy as np
     sess = ort.InferenceSession("{expected}")
     input_name = sess.get_inputs()[0].name
     output = sess.run(None, {{input_name: img_array}})  # img: (1,3,640,640) float32 /255
        """)
    elif args.format == "engine":
        print(f"""
  1. Cu Ultralytics:
     python detect.py --weights {expected} --source imagine.jpg

  ATENTIE: Engine-ul functioneaza DOAR pe GPU-ul si CUDA version pe care a fost creat!
        """)
    elif args.format == "openvino":
        print(f"""
  pip install openvino
  from ultralytics import YOLO
  model = YOLO("{expected}")
  model.predict("imagine.jpg")
        """)

    print(f"{'='*60}")


if __name__ == "__main__":
    main()