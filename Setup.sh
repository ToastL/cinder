#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$(uname -s)" in
    Darwin)
        platform=macOS
        cxx="${CXX:-$(xcrun --find clang++)}"
        sysroot=(-isysroot "$(xcrun --show-sdk-path)")
        exports=(-Wl,-export_dynamic)
        ;;
    *)
        platform=Linux
        cxx="${CXX:-c++}"
        sysroot=()
        exports=(-rdynamic)
        ;;
esac

out="${CINDER_BINARIES:-$root/Binaries}/$platform"
mkdir -p "$out"

sources=("$root"/Source/Programs/CinderBuildTool/*.cpp)
while IFS= read -r -d '' file; do
    sources+=("$file")
done < <(find "$root/Source" "$root/Tests" \( -name '*.Build.cpp' -o -name '*.Target.cpp' \) -print0 2>/dev/null)

"$cxx" -std=c++20 -O2 -Wall -Wextra "${sysroot[@]}" -I"$root/Source/Programs" -DCBT_ROOT="\"$root\"" \
    "${sources[@]}" "${exports[@]}" -o "$out/cbt.tmp"
mv "$out/cbt.tmp" "$out/cbt"
echo "built $out/cbt"
