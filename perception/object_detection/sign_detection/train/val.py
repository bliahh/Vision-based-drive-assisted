"""
val.py - Evaluare si metrici YOLOv11s pentru detectie semne de circulatie
Genereaza: mAP50, mAP50-95, precision, recall, confusion matrix, per-class stats
"""

import argparse
import json
from pathlib import Path
import numpy as np
from ultralytics import YOLO


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


def parse_args():
    parser = argparse.ArgumentParser(description="Evaluare YOLOv11s - Traffic Signs")
    parser.add_argument("--weights",  type=str, required=True,         help="Calea catre best.pt")
    parser.add_argument("--data",     type=str, default="config/data.yaml",   help="Calea catre data.yaml")
    parser.add_argument("--imgsz",    type=int, default=640,           help="Dimensiune imagine")
    parser.add_argument("--batch",    type=int, default=16,            help="Batch size")
    parser.add_argument("--device",   type=str, default="0",           help="GPU index sau 'cpu'")
    parser.add_argument("--split",    type=str, default="test",        help="Split de evaluat: val / test")
    parser.add_argument("--conf",     type=float, default=0.001,       help="Confidence threshold (scazut pentru evaluare corecta)")
    parser.add_argument("--iou",      type=float, default=0.6,         help="IoU threshold NMS")
    parser.add_argument("--project",  type=str, default="runs/val",    help="Director rezultate")
    parser.add_argument("--name",     type=str, default="eval",        help="Nume experiment")
    parser.add_argument("--save-json",action="store_true",             help="Salveaza metrici in JSON")
    return parser.parse_args()


def print_metrics_table(metrics, class_names):
    """Afiseaza tabel per-clasa cu Precision, Recall, mAP50"""
    print(f"\n{'='*80}")
    print(f"  PER-CLASS METRICS")
    print(f"{'='*80}")
    print(f"  {'Clasa':<35} {'Precision':>10} {'Recall':>10} {'mAP50':>10} {'mAP50-95':>10}")
    print(f"  {'-'*75}")

    # rezultatele per-clasa din ultralytics
    try:
        box = metrics.box
        # Metrici globale
        print(f"  {'ALL CLASSES':<35} {box.mp:>10.4f} {box.mr:>10.4f} {box.map50:>10.4f} {box.map:>10.4f}")
        print(f"  {'-'*75}")

        # Per clasa
        if hasattr(box, 'ap_class_index') and box.ap_class_index is not None:
            ap_per_class = box.ap_class_index
            maps = box.maps  # mAP50-95 per class

            for i, cls_idx in enumerate(ap_per_class):
                cls_name = class_names[cls_idx] if cls_idx < len(class_names) else f"class_{cls_idx}"
                map50    = box.ap50[i]   if hasattr(box, 'ap50')  else 0.0
                map5095  = maps[cls_idx] if maps is not None      else 0.0
                # P si R per class nu sunt direct expuse, folosim valorile globale ca fallback
                print(f"  {cls_name:<35} {'N/A':>10} {'N/A':>10} {map50:>10.4f} {map5095:>10.4f}")
    except Exception as e:
        print(f"  [WARN] Nu s-au putut afisa metrici per-clasa: {e}")

    print(f"{'='*80}\n")


def main():
    args = parse_args()

    weights_path = Path(args.weights)
    if not weights_path.exists():
        raise FileNotFoundError(f"Weights negasite: {weights_path}")

    print(f"\n{'='*60}")
    print(f"  Evaluare YOLOv11s - Traffic Signs ({args.split} split)")
    print(f"{'='*60}")
    print(f"  Weights: {args.weights}")
    print(f"  Data:    {args.data}")
    print(f"  Split:   {args.split}")
    print(f"  Conf:    {args.conf}")
    print(f"  IoU:     {args.iou}")
    print(f"{'='*60}\n")

    model = YOLO(args.weights)

    metrics = model.val(
        data=args.data,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device,
        split=args.split,
        conf=args.conf,
        iou=args.iou,
        project=args.project,
        name=args.name,
        plots=True,      # confusion matrix, PR curve, F1 curve
        save_json=True,
        verbose=True,
        exist_ok=True,
    )

    # Afiseaza sumar
    box = metrics.box
    print(f"\n{'='*60}")
    print(f"  REZULTATE GLOBALE")
    print(f"{'='*60}")
    print(f"  mAP50:       {box.map50:.4f}  ({box.map50*100:.1f}%)")
    print(f"  mAP50-95:    {box.map:.4f}   ({box.map*100:.1f}%)")
    print(f"  Precision:   {box.mp:.4f}")
    print(f"  Recall:      {box.mr:.4f}")
    print(f"  F1-Score:    {2 * box.mp * box.mr / (box.mp + box.mr + 1e-9):.4f}")
    print(f"{'='*60}\n")

    # Afiseaza tabel per-clasa
    print_metrics_table(metrics, CLASS_NAMES)

    # Salveaza in JSON daca e cerut
    if args.save_json:
        out_json = Path(args.project) / args.name / "metrics_summary.json"
        out_json.parent.mkdir(parents=True, exist_ok=True)
        summary = {
            "weights": str(args.weights),
            "split": args.split,
            "mAP50":      round(float(box.map50), 4),
            "mAP50_95":   round(float(box.map),   4),
            "precision":  round(float(box.mp),     4),
            "recall":     round(float(box.mr),     4),
            "f1":         round(2 * float(box.mp) * float(box.mr) / (float(box.mp) + float(box.mr) + 1e-9), 4),
            "per_class_mAP50": {
                CLASS_NAMES[int(i)]: round(float(v), 4)
                for i, v in zip(metrics.box.ap_class_index, metrics.box.ap50)
            } if hasattr(metrics.box, 'ap_class_index') and metrics.box.ap_class_index is not None else {}
        }
        with open(out_json, "w") as f:
            json.dump(summary, f, indent=2)
        print(f"[OK] Metrici salvate in: {out_json}")

    print(f"[OK] Grafice (confusion matrix, PR curve) salvate in: {args.project}/{args.name}/")
    return metrics


if __name__ == "__main__":
    main()