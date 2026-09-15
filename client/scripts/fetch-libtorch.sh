#!/usr/bin/env bash
set -euo pipefail

LIBTORCH_VERSION="2.7.1"
LIBTORCH_VARIANT="cpu"
LIBTORCH_URL="https://download.pytorch.org/libtorch/${LIBTORCH_VARIANT}/libtorch-cxx11-abi-shared-with-deps-${LIBTORCH_VERSION}%2B${LIBTORCH_VARIANT}.zip"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
THIRD_PARTY_DIR="${SCRIPT_DIR}/../third_party"
DEST_DIR="${THIRD_PARTY_DIR}/libtorch"

if [ -f "${DEST_DIR}/share/cmake/Torch/TorchConfig.cmake" ]; then
    echo "libtorch already present at ${DEST_DIR}, skipping."
    exit 0
fi

if [ -e "${DEST_DIR}" ]; then
    echo "error: ${DEST_DIR} exists but doesn't look like libtorch (missing share/cmake/Torch/TorchConfig.cmake)." >&2
    echo "Remove it and re-run this script." >&2
    exit 1
fi

TMP_ZIP="$(mktemp -t libtorch-XXXXXX.zip)"
trap 'rm -f "${TMP_ZIP}"' EXIT

echo "Downloading libtorch ${LIBTORCH_VERSION} (${LIBTORCH_VARIANT}) ..."
curl -fL --progress-bar -o "${TMP_ZIP}" "${LIBTORCH_URL}"

echo "Extracting into ${THIRD_PARTY_DIR} ..."
unzip -q "${TMP_ZIP}" -d "${THIRD_PARTY_DIR}"

echo "Done: ${DEST_DIR}"
