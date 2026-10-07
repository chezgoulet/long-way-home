#!/usr/bin/env bash
# Fetch the official authoring content G0's checks run against, into build/gdk (gitignored, durable):
# the Game Development Kit's two entity dictionaries, the level-script corpus, and the published
# map sources. All from the Internet Archive item the docs cite; none of it enters this repository.
#
#   scripts/fetch-gdk.sh
#
# Then:  scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
#
# The GDK is a Windows installer: an MSI holding one LZX-compressed cabinet. It is unpacked, never
# run -- the cabinet is lifted out with a pinned Python package and expanded with cabextract, which
# is used from PATH or fetched rootlessly (apt-get download) into build/tools.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GDK="$ROOT/build/gdk"
BASE="https://archive.org/download/star-trek-voyager-elite-force/Editing"
mkdir -p "$GDK/dl" "$GDK/maps" "$GDK/scripts" "$GDK/raw"

# sha256, local name, path under $BASE
FILES="
87262731e56a0b16868bba8fd4deb353a8f79f3aec5b86ddee2be8ead279dedf gdk.zip Game%20Development%20Kit%201.2/eliteforceGDK2.zip
93e63e0c1bbd766e4ebc29cbb33939dce772ec648d0a885933d69eada6362e0d real_scripts.zip Level%20Scripts/real_scripts.zip
8d99f039100e2a65ad91377c9c21a8ab09bef3f72e33919020b2d64e1e247079 brig-map.zip Map%20Sources/brig-map.zip
2cc062c11c46aa6717cd77ed61accc078ae55e21fd02dd059457a5f536b5990c eliteforce_borg_maps.zip Map%20Sources/eliteforce_borg_maps.zip
5c5627b63a675ce3575efe98a6635ec24341a7975d801b3d7baf940376f9f8df eliteforce_ctf_maps.zip Map%20Sources/eliteforce_ctf_maps.zip
12fdcb79daa1f998f361bd323c2961fb02670ebed69adaa05d15585428d31154 eliteforce_dreadnought_maps.zip Map%20Sources/eliteforce_dreadnought_maps.zip
c657ee24d138a104bdf2d6b7e4a34c5c988ade6c92a5ebc122e1d25542091fa8 eliteforce_forge_maps.zip Map%20Sources/eliteforce_forge_maps.zip
1cd013c7e02c2bff420edb55956c5e16e54b0172382e8315568820471a528e7a eliteforce_holodeck_maps.zip Map%20Sources/eliteforce_holodeck_maps.zip
66e7e84ed280fcc2aaf2d9f63607a904ad58773ba67ab840389a4b78d8ba07bf eliteforce_holomatch_maps.zip Map%20Sources/eliteforce_holomatch_maps.zip
c09333a42d349d3be590dd05c66d8b571e284b71352c6a5e9ad9b3f5fa943eae eliteforce_scav_maps.zip Map%20Sources/eliteforce_scav_maps.zip
a0caa9f9085375dc77491ee0eebd400c96cb190acfab9adad5c0b4255d62f1ab eliteforce_stasis_maps.zip Map%20Sources/eliteforce_stasis_maps.zip
d0c4a92ba0e5036fe418cf7b304fd258b65a80ae6ba8c431d76e51620b84fa6e eliteforce_virtualvoyager_maps.zip Map%20Sources/eliteforce_virtualvoyager_maps.zip
b2587dbd1500f3070f3baea93823351be9c7e6ae1e35bffdd6c0665502dd04f9 eliteforce_voyager_maps.zip Map%20Sources/eliteforce_voyager_maps.zip
"

echo "==> downloading (skipping what is already here and intact)"
while read -r sum name path; do
  [ -n "$sum" ] || continue
  if ! echo "$sum  $GDK/dl/$name" | sha256sum --check --quiet 2>/dev/null; then
    curl -fsSL -o "$GDK/dl/$name" "$BASE/$path"
    echo "$sum  $GDK/dl/$name" | sha256sum --check --quiet || { echo "checksum mismatch: $name" >&2; exit 1; }
  fi
done <<<"$FILES"

echo "==> level scripts and map sources"
unzip -oq "$GDK/dl/real_scripts.zip" -d "$GDK/scripts"
for z in "$GDK"/dl/*map*.zip; do
  unzip -oq "$z" -d "$GDK/maps/$(basename "$z" .zip)"
done

echo "==> entity dictionaries, from the installer"
CABEXTRACT="$(command -v cabextract || true)"
LIBS=""
if [ -z "$CABEXTRACT" ]; then
  SYSROOT="$ROOT/build/tools/sysroot"
  if [ ! -x "$SYSROOT/usr/bin/cabextract" ]; then
    command -v apt-get >/dev/null 2>&1 || { echo "cabextract not found, and no apt-get to fetch it with: install cabextract" >&2; exit 1; }
    mkdir -p "$ROOT/build/tools/debs"
    (cd "$ROOT/build/tools/debs" && apt-get download cabextract libmspack0t64 >/dev/null)
    for deb in "$ROOT"/build/tools/debs/*.deb; do dpkg -x "$deb" "$SYSROOT"; done
  fi
  CABEXTRACT="$SYSROOT/usr/bin/cabextract"
  LIBS="$(dirname "$(find "$SYSROOT" -name 'libmspack.so*' | head -1)")"
fi
command -v uv >/dev/null 2>&1 || { echo "uv not found: it runs the pinned MSI reader (https://docs.astral.sh/uv/)" >&2; exit 1; }

unzip -oq "$GDK/dl/gdk.zip" -d "$GDK/msi"
uv run --quiet --no-project --with 'olefile==0.47' python -c '
import os, sys, olefile
d = sys.argv[1]
ole = olefile.OleFileIO(os.path.join(d, "eliteforceGDK.msi"))
cabs = [s for s in (ole.openstream(e).read() for e in ole.listdir()) if s[:4] == b"MSCF"]
assert len(cabs) == 1, "expected one cabinet in the installer, found %d" % len(cabs)
with open(os.path.join(d, "payload.cab"), "wb") as f:
    f.write(cabs[0])
' "$GDK/msi"
LD_LIBRARY_PATH="$LIBS${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$CABEXTRACT" -q -d "$GDK/raw" "$GDK/msi/payload.cab"

# The cabinet names its files by installer key, not by name. The dictionaries are the two files
# with the most QUAKED blocks (a few tools mention the word); single-player has twice Holomatch's.
SP=""; HM=""
while IFS=: read -r f _; do
  if [ -z "$SP" ]; then SP="$f"; elif [ -z "$HM" ]; then HM="$f"; fi
done < <(grep -ac '^/\*QUAKED' "$GDK"/raw/* | sort -t: -k2 -nr | head -2)
[ -n "$SP" ] && [ -n "$HM" ] || { echo "the installer did not yield two entity dictionaries" >&2; exit 1; }
cp "$SP" "$GDK/SP_entities.def"
cp "$HM" "$GDK/HM_entities-def.txt"
# check.sh looks for the dictionaries beside the upstream checkout
if [ -d "$ROOT/../upstream" ]; then cp "$GDK/SP_entities.def" "$GDK/HM_entities-def.txt" "$ROOT/../upstream/"; fi

echo "    SP_entities.def:     $(grep -ac '^/\*QUAKED' "$GDK/SP_entities.def") classes"
echo "    HM_entities-def.txt: $(grep -ac '^/\*QUAKED' "$GDK/HM_entities-def.txt") classes"
echo "    scripts:             $(find "$GDK/scripts" -iname '*.txt' | wc -l) files"
echo "    map sources:         $(find "$GDK/maps" -iname '*.map' | wc -l) files"
