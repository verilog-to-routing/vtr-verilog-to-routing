#!/usr/bin/env python3
"""Self-test for the layer-5 comparator.

Layer 5 is an exact same-host gate, so there is no tolerance to calibrate and
no "environment noise" class to model. What has to be pinned instead is
detection: every class of regression the gate claims to catch must produce a
non-zero differing-pixel count, on a real golden.

The classes below are the ones a previous tolerant metric silently passed —
whole-canvas recolouring and a single wrong glyph in a block label both scored
0 against it. They are here so that reintroducing any tolerance fails loudly.

Run: python3 test_compare_images.py   (exit 0 pass, 1 fail, 2 skipped)
"""

import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

# pylint: disable=wrong-import-position,import-error
from compare_images import (  # noqa: E402
    DEFAULT_INTENSITY_TOL,
    DEFAULT_MAX_DIFF_PIXELS,
    compare,
)

# The immediate renderer is always available, so its golden exists on any host.
BASE_PNG = SCRIPT_DIR / "golden" / "immediate" / "placement_default.png"


def _deps():
    """Import the optional imaging stack, or exit 2 so ctest reports Skipped."""
    # pylint: disable=import-outside-toplevel,import-error
    try:
        from PIL import Image as pil_image
        import numpy as np
    except ImportError as exc:  # pragma: no cover - environment problem
        # Exit 2, not 0: ctest's SKIP_RETURN_CODE renders this as Skipped. A
        # silent exit 0 is how layer 5 stayed "green" while never running.
        print(f"SKIP: missing dependency: {exc}")
        sys.exit(2)
    return pil_image, np


def _build_regressions(np, base):
    """One perturbation per class of regression the gate must catch."""
    cases = {}

    # Uniform recolour under any plausible tolerance — the class that defeated
    # the previous metric completely (it scored 0 for all of these).
    grey_bg = base.copy()
    white = (base > 240).all(axis=2)
    grey_bg[white] = [200, 200, 200]
    cases["background recoloured (delta 55)"] = grey_bg

    subtle = base.astype(np.int16).copy()
    subtle[(base < 240).any(axis=2)] -= 40
    cases["foreground darkened (delta 40)"] = np.clip(subtle, 0, 255).astype(np.uint8)

    # A single wrong glyph. Coordinates are the first block's upper label band
    # (y 730-742, x 708-763 in placement_default.png); shifting one character
    # cell over its neighbour is the "clb[0] -> clb[8]" class that a tolerant
    # metric scored at ~33 px and passed.
    glyph = base.copy()
    glyph[730:743, 708:722] = base[730:743, 722:736]
    cases["one glyph overwritten"] = glyph

    # Small localised loss - a deleted pin marker or congestion dot, painted
    # over solid block fill so the fixture cannot degenerate into a no-op.
    erased = base.copy()
    erased[600:610, 600:610] = [255, 255, 255]
    cases["10x10 region erased"] = erased

    # Geometry: everything nudged one pixel.
    cases["1 px global shift"] = np.roll(base, 1, axis=1)

    return cases


def main() -> int:
    """Run every detection case against a real golden."""
    pil_image, np = _deps()

    if not BASE_PNG.exists():
        print(f"SKIP: no golden to perturb at {BASE_PNG}")
        return 2

    base = np.array(pil_image.open(BASE_PNG).convert("RGB"))
    failures = []

    with tempfile.TemporaryDirectory() as td:
        base_path = Path(td) / "base.png"
        pil_image.fromarray(base).save(base_path)

        def ndiff(img):
            p = Path(td) / "cand.png"
            pil_image.fromarray(img).save(p)
            n, _ = compare(base_path, p)
            return n

        # An unchanged render must be clean, or the gate is unusable.
        n = ndiff(base.copy())
        print(f"  identical{'':<34} {n:>9} px  {'ok' if n == 0 else 'FAILED'}")
        if n != 0:
            failures.append(f"identical images reported {n} differing px")

        for name, img in _build_regressions(np, base).items():
            # A fixture that perturbs nothing would "pass" for the wrong reason
            # -- most likely because the golden's layout moved under it.
            if np.array_equal(img, base):
                print(f"  {name:<42} {'--':>9}      DEGENERATE")
                failures.append(
                    f"fixture {name!r} is a no-op against the current golden; "
                    "its hardcoded coordinates no longer hit any content"
                )
                continue

            n = ndiff(img)
            caught = n > DEFAULT_MAX_DIFF_PIXELS
            print(f"  {name:<42} {n:>9} px  {'ok' if caught else 'FAILED'}")
            if not caught:
                failures.append(
                    f"{name!r} scored {n} px, within the {DEFAULT_MAX_DIFF_PIXELS} "
                    "px budget - it would not be caught"
                )

    # The defaults are what make the above hold; a future edit that reintroduces
    # tolerance must fail here rather than quietly weakening the gate.
    if DEFAULT_MAX_DIFF_PIXELS != 0 or DEFAULT_INTENSITY_TOL != 0:
        failures.append(
            f"defaults are no longer exact (max_diff_pixels="
            f"{DEFAULT_MAX_DIFF_PIXELS}, intensity_tol={DEFAULT_INTENSITY_TOL}). "
            "Layer 5 is a same-host gate; widening it silently reintroduces "
            "blindness to recolouring and single-glyph label errors."
        )

    if failures:
        print("\nFAIL:")
        for f in failures:
            print(f"  - {f}")
        return 1

    print("\nPASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
