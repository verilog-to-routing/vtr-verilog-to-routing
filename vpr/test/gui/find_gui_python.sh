# shellcheck shell=bash
# Shared interpreter resolution for the layer-5 Python tooling.
#
# Source this and call resolve_gui_python; it echoes the path of the first
# interpreter that can actually import the imaging stack, or returns 1.
#
# The validation matters more than the search order: `make gui-tests` runs with
# whatever PATH the build was launched from, which is usually a system python
# without scikit-image. A caller that just runs `python3` silently degrades --
# and a test that skips itself for a missing dependency looks exactly like a
# test that passed.

resolve_gui_python() {
    local repo_root candidates=() cand resolved
    repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

    [[ -n "${VPR_GUI_PYTHON:-}" ]] && candidates+=("${VPR_GUI_PYTHON}")
    [[ -x "${repo_root}/.venv/bin/python3" ]] && candidates+=("${repo_root}/.venv/bin/python3")
    [[ -n "${VIRTUAL_ENV:-}" && -x "${VIRTUAL_ENV}/bin/python3" ]] \
        && candidates+=("${VIRTUAL_ENV}/bin/python3")
    candidates+=(python3 python)

    for cand in "${candidates[@]}"; do
        resolved="$(command -v "${cand}" 2>/dev/null || true)"
        [[ -z "${resolved}" ]] && continue
        if "${resolved}" -c 'import skimage, PIL, numpy' >/dev/null 2>&1; then
            echo "${resolved}"
            return 0
        fi
    done
    return 1
}

gui_python_hint() {
    echo "       Tried: \$VPR_GUI_PYTHON, <vtr>/.venv/bin/python3, \$VIRTUAL_ENV, python3, python." >&2
    echo "       Install with:  python3 -m pip install -r dev/gui_test_requirements.txt" >&2
    echo "       Or point at one explicitly:  VPR_GUI_PYTHON=/path/to/python ..." >&2
}
