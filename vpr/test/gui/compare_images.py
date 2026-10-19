#!/usr/bin/env python3
"""Layer 5 — golden image comparison.

Layer 5 is a **same-host** gate. VPR's offscreen rendering is bit-exact on a
fixed host (42/42 outputs byte-identical across repeated runs), and label fonts
are pinned to a checked-in font, so immediate/deferred renders also match across
hosts. That makes an exact pixel comparison both the simplest and the most sensitive gate
available: it catches recolouring, a single wrong glyph in a block label, a
deleted wire and a moved block alike.

A drift-tolerant metric that survives a different host was measured and
rejected. Tolerating cross-host text rasterization costs ~9,000 differing
pixels; a genuinely wrong label digit costs ~33. No threshold separates them, so
a tolerant gate is blind to precisely the regressions this layer exists to
catch. If the render host changes, regenerate the goldens — do not widen the
gate.

SSIM is still computed and reported as a magnitude cue when reading a failure,
but it does not gate unless --threshold is passed explicitly.

Usage:
    python compare_images.py <image_a> <image_b> [--max-diff-pixels N]

Exit codes:
    0 — images match
    1 — images differ
    2 — error (file not found, unreadable image, missing dependency)

Dependencies:
    pip install scikit-image Pillow numpy
"""

import argparse
import sys
from pathlib import Path
from typing import Optional, Tuple

# Bit-exact on a fixed host, so anything above zero is a real change. Both stay
# exposed as knobs for deliberate one-off investigation.
DEFAULT_MAX_DIFF_PIXELS = 0
DEFAULT_INTENSITY_TOL = 0


def _deps():
    """Import the optional imaging stack, or exit 2 with an actionable message."""
    # pylint: disable=import-outside-toplevel,import-error
    try:
        from PIL import Image as pil_image
        import numpy as np
        from skimage.metrics import structural_similarity
    except ImportError as exc:
        print(f"ERROR: Missing dependency: {exc}", file=sys.stderr)
        print("Install with: pip install scikit-image Pillow numpy", file=sys.stderr)
        sys.exit(2)
    return pil_image, np, structural_similarity


def _load(path: Path):
    """Load as RGB, compositing alpha over white rather than discarding it."""
    pil_image, np, _ = _deps()
    try:
        img = pil_image.open(path)
        img.load()
    # pylint: disable=broad-exception-caught
    except Exception as exc:  # any decode failure is an infra fault, not a diff
        print(f"ERROR: cannot read image {path}: {exc}", file=sys.stderr)
        sys.exit(2)

    # The goldens are RGBA. A plain .convert("RGB") drops alpha, so a render
    # that went fully transparent would compare equal to its golden.
    if img.mode in ("RGBA", "LA") or (img.mode == "P" and "transparency" in img.info):
        background = pil_image.new("RGBA", img.size, (255, 255, 255, 255))
        img = pil_image.alpha_composite(background, img.convert("RGBA"))
    return np.array(img.convert("RGB"))


def _write_diff_triptych(img_a, img_b, diff_out: Path) -> None:
    """Write a ``[golden | current | amplified-diff]`` triptych PNG.

    The diff channel is amplified 8x and clipped so sub-pixel drift is visible.
    """
    pil_image, np, _ = _deps()
    diff = np.clip(np.abs(img_a.astype(np.int16) - img_b.astype(np.int16)) * 8, 0, 255).astype(
        np.uint8
    )
    diff_out.parent.mkdir(parents=True, exist_ok=True)
    pil_image.fromarray(np.concatenate([img_a, img_b, diff], axis=1)).save(diff_out)


def differing_pixels(a, b, intensity_tol: int = DEFAULT_INTENSITY_TOL) -> int:
    """Count pixels where any channel differs by more than ``intensity_tol``."""
    _, np, _ = _deps()
    delta = np.abs(a.astype(np.int16) - b.astype(np.int16)).max(axis=2)
    return int((delta > intensity_tol).sum())


def compare(
    path_a: Path,
    path_b: Path,
    intensity_tol: int = DEFAULT_INTENSITY_TOL,
) -> Tuple[int, Optional[float]]:
    """Return ``(differing pixel count, SSIM)``; count is -1 on a size mismatch."""
    _, _, structural_similarity = _deps()
    a = _load(path_a)
    b = _load(path_b)

    if a.shape != b.shape:
        # Never resize to compare — that launders a resolution regression into
        # a pass.
        print(
            f"ERROR: dimension mismatch: {path_a.name} is {a.shape[1]}x{a.shape[0]}, "
            f"{path_b.name} is {b.shape[1]}x{b.shape[0]}",
            file=sys.stderr,
        )
        return -1, None

    return differing_pixels(a, b, intensity_tol), float(structural_similarity(a, b, channel_axis=2))


def main() -> int:
    """CLI entry point: compare a render against its golden."""
    parser = argparse.ArgumentParser(description="Compare a render against its golden")
    parser.add_argument("image_a", type=Path, help="First image (e.g. golden)")
    parser.add_argument("image_b", type=Path, help="Second image (e.g. current)")
    parser.add_argument(
        "--max-diff-pixels",
        type=int,
        default=DEFAULT_MAX_DIFF_PIXELS,
        help="Fail above this many differing pixels (default: %(default)s).",
    )
    parser.add_argument(
        "--intensity-tol",
        type=int,
        default=DEFAULT_INTENSITY_TOL,
        help="Per-channel delta treated as unchanged (default: %(default)s).",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=None,
        help=(
            "Optional SSIM floor, off by default. SSIM is reported for context "
            "but is a poor gate here: it rates a benign 1 px shift (0.951) "
            "worse than a visibly wrong label (0.999). The gate is "
            "--max-diff-pixels."
        ),
    )
    parser.add_argument("--quiet", action="store_true", help="Only print PASS/FAIL")
    parser.add_argument(
        "--diff-out",
        type=Path,
        default=None,
        help="Write a [golden | current | amplified-diff] triptych PNG here.",
    )
    parser.add_argument(
        "--diff-on-fail-only",
        action="store_true",
        help="Only write --diff-out when the comparison fails.",
    )
    args = parser.parse_args()

    if args.max_diff_pixels < 0:
        parser.error("--max-diff-pixels must be >= 0")
    if not 0 <= args.intensity_tol <= 255:
        parser.error("--intensity-tol must be in [0, 255]")

    for p in (args.image_a, args.image_b):
        if not p.exists():
            print(f"ERROR: File not found: {p}", file=sys.stderr)
            return 2

    ndiff, score = compare(args.image_a, args.image_b, intensity_tol=args.intensity_tol)

    if ndiff < 0:  # dimension mismatch, already reported on stderr
        return 1

    diff_ok = ndiff <= args.max_diff_pixels
    ssim_ok = args.threshold is None or score >= args.threshold
    passed = diff_ok and ssim_ok

    if not args.quiet:
        print(f"Diff px:   {ndiff} (budget {args.max_diff_pixels})")
        floor = "" if args.threshold is None else f" (floor {args.threshold:.6f})"
        print(f"SSIM:      {score:.6f}{floor}")

    # Key the triptych off the actual verdict, not off SSIM.
    if args.diff_out is not None and (not args.diff_on_fail_only or not passed):
        _write_diff_triptych(_load(args.image_a), _load(args.image_b), args.diff_out)

    if passed:
        print("PASS")
        return 0

    reasons = []
    if not diff_ok:
        reasons.append(f"{ndiff} differing px > {args.max_diff_pixels}")
    if not ssim_ok:
        reasons.append(f"SSIM {score:.6f} < {args.threshold:.6f}")
    print(f"FAIL ({'; '.join(reasons)})")
    return 1


if __name__ == "__main__":
    sys.exit(main())
