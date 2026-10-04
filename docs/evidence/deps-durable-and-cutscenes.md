# Evidence — cutscenes enabled, dependencies durable, and the static-archive trap closed

Date: 2026-10-04. Closes two residuals: "no cutscene playback" and "dependencies live in /tmp".

## Cutscenes are on

```
configure: -- Bink cutscene playback: enabled (FFmpeg found at .../deps/.../libavcodec.so)
build:     rc=0   0 errors
```

**A correction to go with it:** I had reported that FFmpeg sat behind Ubuntu Pro because
`apt-get download libavcodec-dev` returned `401 Unauthorized`. That was wrong. The ESM package was
merely the *preferred* candidate; pinning the base version downloads from the plain archive:

```
apt-get download libavcodec-dev=7:6.1.1-3ubuntu5     # and the runtime packages, pinned the same way
```

The lesson is about reading a package-manager answer: *"the candidate is an ESM build"* is not
*"the package is unavailable"*. Pinning the version asks the question that matters.

## The third appearance of the static-archive trap

FFmpeg failed twice before it worked, in two different ways that looked unrelated:

- With default library suffixes: CMake picked `libavcodec.a`, and a static FFmpeg needs zlib — so the
  build died on `undefined reference to uncompress`, an error that names a zlib symbol for what is
  actually a *which-library-was-chosen* problem.
- With suffixes narrowed to `.so`: FFmpeg was reported simply *not found*, and Bink was silently
  disabled.

**Both had the same cause: a dangling soname.** The dev package installs `libavcodec.so ->
libavcodec.so.60`, and the runtime package that provides `.so.60` had never been fetched. A dangling
symlink is invisible to `find_library` but *not* to the fallback search, which then takes the static
archive.

Worth keeping because the diagnostic I used first was worthless: **`readlink -f` prints a final target
whether or not it exists.** It reported the chain as fine. `readlink -e`, or a plain `test -e`, is the
check that answers the question.

The structural fix: FFmpeg's finds narrow `CMAKE_FIND_LIBRARY_SUFFIXES` to `.so`, so a static archive
cannot be chosen by accident, and the honest fallback when only an archive exists is "Bink disabled"
rather than a broken link line.

## Dependencies are durable, and the binary needs no environment

The rootless dependency tree lived in `/tmp` — which does not survive a reboot — and the engine's
runtime search path had been baked against it. Both are fixed: the tree is copied to `build/deps`
(92 MB, headers included), the build points there, and the result is verifiable:

```
RPATH:  $ORIGIN:/home/c/big/git/long-way-home/build/deps/usr/lib/x86_64-linux-gnu
ldd with LD_LIBRARY_PATH unset:  0 missing
  libSDL2-2.0.so.0     => .../build/deps/.../libSDL2-2.0.so.0
  libavcodec.so.60     => .../build/deps/.../libavcodec.so.60
```

So the engine starts with **no environment help at all**, whether launched through `run-engine.sh`, from
a file manager, or after a reboot. The script still sets the variable for older builds; it is no longer
what makes it work.
