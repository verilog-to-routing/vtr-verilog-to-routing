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
from typing import Optional


def _write_diff_triptych(img_a, img_b, diff_out: Path) -> None:
    """Write a ``[golden | current | amplified-diff]`` triptych PNG.

    The diff channel is ``|img_a − img_b|`` amplified 8× and clipped, so
    sub-pixel drift is visible to the eye — a pure 256-level absdiff is
    invisible at the SSIM thresholds we care about (~0.98+).
    """
    # pylint: disable=import-outside-toplevel,import-error
    from PIL import Image
    import numpy as np

    diff = np.clip(
        np.abs(img_a.astype(np.int16) - img_b.astype(np.int16)) * 8,
        0,
        255,
    ).astype(np.uint8)
    diff_out.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.concatenate([img_a, img_b, diff], axis=1)).save(diff_out)


def compare_ssim(
    path_a: Path,
    path_b: Path,
    diff_out: Optional[Path] = None,
    diff_only_below: Optional[float] = None,
) -> float:
    """Compute SSIM between two images. Returns value in [0, 1].

    When ``diff_out`` is provided, writes a triptych PNG
    ``[golden | current | amplified-diff]`` to that path so a human can
    eyeball the discrepancy after the test run.

    ``diff_only_below`` gates the write: ``None`` → always write the diff;
    a float ``T`` → write only when ``score < T``. The latter is what the
    ``--diff-on-fail-only`` runner flag wires up.
    """
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

    if diff_out is not None and (diff_only_below is None or score < diff_only_below):
        _write_diff_triptych(img_a, img_b, diff_out)

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
    parser.add_argument(
        "--diff-out",
        type=Path,
        default=None,
        help=(
            "Write a [golden | current | amplified-diff] triptych PNG to this "
            "path for manual inspection. By default writes for every case; "
            "see --diff-on-fail-only."
        ),
    )
    parser.add_argument(
        "--diff-on-fail-only",
        action="store_true",
        help=(
            "Only write --diff-out when SSIM < threshold. Lets the runner "
            "skip the triptych-write cost on passing cases while still "
            "capturing failures for post-mortem."
        ),
    )
    args = parser.parse_args()

    for p in (args.image_a, args.image_b):
        if not p.exists():
            print(f"ERROR: File not found: {p}", file=sys.stderr)
            return 2

    diff_only_below = args.threshold if args.diff_on_fail_only else None
    score = compare_ssim(
        args.image_a,
        args.image_b,
        diff_out=args.diff_out,
        diff_only_below=diff_only_below,
    )

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
