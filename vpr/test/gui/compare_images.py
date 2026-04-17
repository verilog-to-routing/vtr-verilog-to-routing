#!/usr/bin/env python3
"""Layer 5 — Image comparison using SSIM (Structural Similarity Index).

Usage:
    python compare_images.py <image_a> <image_b> [--threshold 0.999]

Exit codes:
    0 — images match (SSIM >= threshold)
    1 — images differ (SSIM < threshold)
    2 — error (file not found, import error, etc.)

Dependencies:
    pip install scikit-image Pillow numpy
"""

import argparse
import sys
from pathlib import Path


def compare_ssim(path_a: Path, path_b: Path) -> float:
    """Compute SSIM between two images. Returns value in [0, 1]."""
    # These dependencies are optional (CI-only) — graceful error if missing.
    # pylint: disable=import-outside-toplevel,import-error
    try:
        from skimage.metrics import structural_similarity
        from PIL import Image
        import numpy as np
    except ImportError as exc:
        print(f"ERROR: Missing dependency: {exc}", file=sys.stderr)
        print("Install with: pip install scikit-image Pillow numpy", file=sys.stderr)
        sys.exit(2)

    img_a = np.array(Image.open(path_a).convert("RGB"))
    img_b = np.array(Image.open(path_b).convert("RGB"))

    # Resize to match if dimensions differ (cross-platform tolerance)
    if img_a.shape != img_b.shape:
        min_h = min(img_a.shape[0], img_b.shape[0])
        min_w = min(img_a.shape[1], img_b.shape[1])
        img_a = np.array(Image.fromarray(img_a).resize((min_w, min_h), Image.LANCZOS))
        img_b = np.array(Image.fromarray(img_b).resize((min_w, min_h), Image.LANCZOS))

    score = structural_similarity(img_a, img_b, channel_axis=2)
    return float(score)


def main() -> int:
    """CLI entry point: compare two images and report SSIM result."""
    parser = argparse.ArgumentParser(description="Compare two images via SSIM")
    parser.add_argument("image_a", type=Path, help="First image (e.g. golden)")
    parser.add_argument("image_b", type=Path, help="Second image (e.g. current)")
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.999,
        help="SSIM threshold for pass (default: 0.999)",
    )
    parser.add_argument("--quiet", action="store_true", help="Only print PASS/FAIL")
    args = parser.parse_args()

    for p in (args.image_a, args.image_b):
        if not p.exists():
            print(f"ERROR: File not found: {p}", file=sys.stderr)
            return 2

    score = compare_ssim(args.image_a, args.image_b)

    passed = score >= args.threshold

    if not args.quiet:
        print(f"SSIM:      {score:.6f}")
        print(f"Threshold: {args.threshold:.6f}")

    if passed:
        print("PASS")
        return 0

    print(f"FAIL (delta: {args.threshold - score:.6f})")
    return 1


if __name__ == "__main__":
    sys.exit(main())
