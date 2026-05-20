import cv2
import numpy as np
from skimage.morphology import skeletonize

def collect_lane_data(mask_da, mask_lane, img_shape) -> dict:
    H, W = img_shape[:2]
    return {
        "drivable_polygons": _mask_to_polygons(mask_da, W, H),
        "lane_lines": _mask_to_polylines(mask_lane, W, H),
    }

def _mask_to_polygons(mask, W: int, H: int, max_contours: int = 2) -> list:
    if mask is None:
        return []
    m = _binarize(mask)
    contours, _ = cv2.findContours(m, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        return []
    min_area = W * H * 0.005
    contours = [c for c in contours if cv2.contourArea(c) >= min_area]
    contours = sorted(contours, key=cv2.contourArea, reverse=True)[:max_contours]
    polygons = []
    for cnt in contours:
        epsilon = 0.02 * cv2.arcLength(cnt, True)
        approx = cv2.approxPolyDP(cnt, epsilon, True)
        if len(approx) < 3 or len(approx) > 10:
            continue
        pts = [[round(float(p[0][0]) / W, 4), round(float(p[0][1]) / H, 4)] for p in approx]
        polygons.append(pts)
    return polygons

def _mask_to_polylines(mask, W: int, H: int) -> list:
    if mask is None:
        return []

    m = _binarize(mask)

    HN_W, HN_H = 640, 384
    m_small = cv2.resize(m, (HN_W, HN_H), interpolation=cv2.INTER_NEAREST)

    skeleton = skeletonize(m_small > 0).astype(np.uint8) * 255

    skel = skeleton.copy()
    for _ in range(20):
        nb = cv2.filter2D((skel > 0).astype(np.uint8), -1, np.ones((3, 3), np.uint8))
        skel[(skel > 0) & (nb == 2)] = 0

    n_labels, labels = cv2.connectedComponents(skel)

    result = []
    for label_id in range(1, n_labels):
        component = (labels == label_id).astype(np.uint8)
        ys, xs = np.where(component > 0)
        if len(xs) < 10:
            continue

        span_x = int(xs.max()) - int(xs.min())
        span_y = int(ys.max()) - int(ys.min())

        if span_x > HN_W * 0.3 and span_y > HN_H * 0.1:
            x_mid = int(xs.mean())

            mask_left = component.copy()
            mask_left[:, x_mid:] = 0

            mask_right = component.copy()
            mask_right[:, :x_mid] = 0
            sub_components = [mask_left, mask_right]
        else:
            sub_components = [component]

        for sub in sub_components:
            ys_s, xs_s = np.where(sub > 0)
            if len(xs_s) < 5:
                continue
            span_y_s = int(ys_s.max()) - int(ys_s.min())
            if span_y_s < HN_H * 0.10:
                continue

        pts = sorted(zip(xs.tolist(), ys.tolist()), key=lambda p: p[1])

        n_samples = 15
        y_min, y_max = pts[0][1], pts[-1][1]
        y_bands = np.linspace(y_min, y_max, n_samples)

        sampled = []
        for y_target in y_bands:
            closest = min(pts, key=lambda p: abs(p[1] - y_target))
            nx = round(float(closest[0]) / HN_W, 4)
            ny = round(float(closest[1]) / HN_H, 4)
            if not sampled or sampled[-1] != [nx, ny]:
                sampled.append([nx, ny])

        if len(sampled) < 3:
            continue

        if sampled[-1][1] < 0.95:
            dx = sampled[-1][0] - sampled[-2][0]
            dy = sampled[-1][1] - sampled[-2][1]
            if abs(dy) > 0.001:
                steps = int((1.0 - sampled[-1][1]) / dy) + 1
                for s in range(1, min(steps + 1, 8)):
                    nx_ext = round(sampled[-1][0] + dx * s, 4)
                    ny_ext = round(sampled[-1][1] + dy * s, 4)
                    if ny_ext > 1.0:
                        break
                    nx_ext = max(0.0, min(1.0, nx_ext))
                    sampled.append([nx_ext, ny_ext])

        result.append(sampled)

    return result

def _binarize(mask) -> np.ndarray:
    m = np.asarray(mask)
    if m.dtype != np.uint8:
        m = (m > 0.5).astype(np.uint8) * 255
    return m