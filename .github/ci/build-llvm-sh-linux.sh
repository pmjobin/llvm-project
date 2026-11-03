#!/usr/bin/env bash
# Linux native distribution packaging: DEB on Ubuntu 26.04, RPM on Fedora 45.
# All packages install under /opt/llvm-sh/rc1, never replacing system clang.
set -euo pipefail

format="${1:?need deb or rpm}"
case "$format" in deb|rpm) ;; *) echo 'format must be deb or rpm' >&2; exit 2;; esac

repo="${GITHUB_WORKSPACE:?}/source"
out="${GITHUB_WORKSPACE}/dist"
bld="${RUNNER_TEMP:?}/llvm-sh-rc1-build"
pkgroot="${RUNNER_TEMP}/llvm-sh-rc1-stage"
prefix='/opt/llvm-sh/rc1'
stage="${pkgroot}${prefix}"
expected='a4ad03d58bd5a924de0ae286340ff7c6bb664ec2'
expected_tree='afbc1ebbfcbae3811208ff4dcfb0930208d3f8a1'

cd "$repo"
actual="$(git rev-parse HEAD)"
tree="$(git rev-parse 'HEAD^{tree}')"
[[ "$actual" == "$expected" && "$tree" == "$expected_tree" ]] || {
  echo "RC1 source mismatch: commit=$actual tree=$tree" >&2; exit 3;
}
[[ -f llvm/lib/Target/SH/CMakeLists.txt && -d clang ]] || exit 4

mkdir -p "$out" "$pkgroot"
printf 'phase\tseconds\n' > "$out/timings.tsv"
timed() {
  local phase="$1" start=$SECONDS
  shift
  "$@"
  printf '%s\t%s\n' "$phase" "$((SECONDS - start))" | tee -a "$out/timings.tsv"
}
printf 'tag=llvm-sh-rc1\ncommit=%s\ntree=%s\n' "$actual" "$tree" \
  | tee "$out/source-$format.txt"
# Remember the system compiler before package installation.
system_clang="$(command -v clang)"
system_clang_real="$(readlink -f "$system_clang")"
system_clang_sha="$(sha256sum "$system_clang_real")"
df -h .
# Build a deliberately small host distribution; NO GNU cross-SDK, QEMU, runtimes or Doom.
# Linux uses shared LLVM/Clang libraries, as in the qualified P07 build profile.
# LLVM_INSTALL_TOOLCHAIN_ONLY=OFF is essential: this distribution explicitly
# requests development tools (llc, opt, llvm-as/dis/link), whose installation
# targets are omitted by the toolchain-only setting.
components='LLVM;clang-cpp;clang;clang-resource-headers;llc;opt;llvm-as;llvm-dis;llvm-readobj;llvm-objdump;llvm-ar;llvm-nm;llvm-link'
timed configure cmake -S llvm -B "$bld" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_INSTALL_PREFIX="$prefix" \
  '-DCMAKE_INSTALL_RPATH=$ORIGIN/../lib;$ORIGIN/../lib64' \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_ENABLE_RUNTIMES= \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SH \
  -DLLVM_BUILD_LLVM_DYLIB=ON \
  -DLLVM_LINK_LLVM_DYLIB=ON \
  -DCLANG_LINK_CLANG_DYLIB=ON \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_INSTALL_TOOLCHAIN_ONLY=OFF \
  "-DLLVM_DISTRIBUTION_COMPONENTS=${components}" \
  -DCLANG_ENABLE_CIR=OFF \
  -DLLVM_ENABLE_LIBEDIT=ON \
  -DLLVM_ENABLE_LIBXML2=ON \
  -DLLVM_ENABLE_ZLIB=ON \
  -DLLVM_ENABLE_ZSTD=ON \
  -DLLVM_ENABLE_LZMA=ON \
  -DLLVM_ENABLE_LIBPFM=OFF \
  -DLLVM_ENABLE_CURL=OFF \
  -DLLVM_PARALLEL_LINK_JOBS=1 \
  -DLLVM_BUILD_DOCS=OFF -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF

timed build cmake --build "$bld" --target distribution --parallel 2
timed stage env DESTDIR="$pkgroot" cmake --build "$bld" --target install-distribution --parallel 2

test -x "$stage/bin/clang"
test -x "$stage/bin/llc"
test -x "$stage/bin/llvm-readobj"
mkdir -p "$stage/share/doc/llvm-sh"
cp llvm/LICENSE.TXT "$stage/share/doc/llvm-sh/LICENSE-LLVM.txt"
cp clang/LICENSE.TXT "$stage/share/doc/llvm-sh/LICENSE-CLANG.txt"
cat > "$stage/BUILDINFO.txt" <<EOF
LLVM SH private RC1 CI build (not the originally qualified binary)
tag=llvm-sh-rc1
commit=$actual
tree=$tree
package_format=$format
host=$(uname -srm)
llvm_enable_projects=clang
llvm_targets=X86;SH(experimental)
no GNU SH4 sysroot, no QEMU, no Doom, no LLVM runtimes
EOF

# Test the staged binaries without LD_LIBRARY_PATH overriding RPATH behavior.
unset LD_LIBRARY_PATH LD_PRELOAD
printf 'int sh_add(int a,int b){return a+b;}\n' > "$RUNNER_TEMP/llvm-sh-smoke.c"
# RC1 Triple.cpp recognizes sh/shle, not GCC's sh4 triple spelling.
# DefaultCPU is sh2: explicitly select sh4, preserving little endian SH4.
smoke() {
  local root="$1" label="$2" report="$out/smoke-$2.txt"
  local object="$RUNNER_TEMP/$label-sh4.o"
  "$root/bin/clang" --version | tee "$report"
  printf '%s --target=shle-unknown-linux-gnu -mcpu=sh4 -c %s -o %s\n' \
    "$root/bin/clang" "$RUNNER_TEMP/llvm-sh-smoke.c" "$object" | tee -a "$report"
  "$root/bin/clang" --target=shle-unknown-linux-gnu -mcpu=sh4 \
    -### -c "$RUNNER_TEMP/llvm-sh-smoke.c" -o "$object" 2>&1 | tee -a "$report"
  "$root/bin/clang" --target=shle-unknown-linux-gnu -mcpu=sh4 \
    -c "$RUNNER_TEMP/llvm-sh-smoke.c" -o "$object"
  "$root/bin/llvm-readobj" --file-headers "$object" | tee -a "$report"
  grep -Fxq 'Format: elf32-shl' "$report"
  grep -Fxq 'Arch: shle' "$report"
  grep -Fq 'Class: 32-bit (0x1)' "$report"
  grep -Fq 'DataEncoding: LittleEndian (0x1)' "$report"
  grep -Fq 'Type: Relocatable (0x1)' "$report"
  grep -Fq 'Machine: EM_SH (0x2A)' "$report"
  # EF_SH4 = 9; reject an accidentally defaulted SH2 object as well.
  grep -Fq 'Flags [ (0x9)' "$report"
}
check_libraries() {
  local root="$1" label="$2" report="$out/libraries-$2.txt" tool
  : > "$report"
  for tool in "$root"/bin/* "$root"/lib*/lib*.so*; do
    [[ -f "$tool" ]] || continue
    readelf -h "$tool" > /dev/null 2>&1 || continue
    {
      printf '\n%s\n' "$tool"
      readelf -d "$tool"
      ldd "$tool"
    } >> "$report"
  done
  cat "$report"
  if grep -F 'not found' "$report"; then
    echo "Unresolved runtime library in $label" >&2; exit 11
  fi
}
timed staged-smoke smoke "$stage" "staged-$format"

# Package the already staged installation with CPack's native DEB/RPM generators.
# It must occupy /opt/llvm-sh/rc1 inside the package (not the CI build path).
config="$RUNNER_TEMP/CPack-llvm-sh-$format.cmake"
cpackdir="$RUNNER_TEMP/cpack-output-$format"
mkdir -p "$cpackdir"
cat > "$config" <<EOF
set(CPACK_PACKAGE_NAME "llvm-sh-clang")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_CONTACT "Pierre-Marc Jobin <pmjobin@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Experimental LLVM/Clang SuperH RC1 host toolchain")
set(CPACK_PACKAGE_DESCRIPTION "Experimental LLVM and Clang RC1 toolchain with SH4 code-generation support. Does not include a target GNU sysroot or emulator.")
set(CPACK_PACKAGE_VENDOR "LLVM SH project")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_INSTALLED_DIRECTORIES "$stage;opt/llvm-sh/rc1")
set(CPACK_PACKAGE_DIRECTORY "$cpackdir")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
EOF
if [[ "$format" == deb ]]; then
  cat >> "$config" <<EOF
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_FILE_NAME "llvm-sh-rc1-ubuntu2604-amd64")
set(CPACK_DEBIAN_PACKAGE_NAME "llvm-sh-clang")
set(CPACK_DEBIAN_PACKAGE_VERSION "0.1.0~rc1")
set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Pierre-Marc Jobin <pmjobin@users.noreply.github.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "$stage/lib;$stage/lib64")
EOF
else
  cat >> "$config" <<EOF
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_FILE_NAME "llvm-sh-rc1-fedora45-x86_64")
set(CPACK_RPM_PACKAGE_NAME "llvm-sh-clang")
set(CPACK_RPM_PACKAGE_VERSION "0.1.0")
set(CPACK_RPM_PACKAGE_RELEASE "0.rc1.1")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0 WITH LLVM-exception")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
EOF
fi
timed package cpack --config "$config" --verbose
case "$format" in
  deb) cp "$cpackdir"/*.deb "$out/" ;;
  rpm) cp "$cpackdir"/*.rpm "$out/" ;;
esac

if [[ "$format" == deb ]]; then
  file="$(find "$out" -maxdepth 1 -name '*.deb' -print -quit)"
  test -f "$file"
  dpkg-deb --field "$file" | tee "$out/metadata-$format.txt"
  dpkg-deb --contents "$file" > "$out/contents-$format.txt"
  dpkg-deb --fsys-tarfile "$file" | tar -tf - > "$out/paths-$format.txt"
  [[ "$(dpkg-deb -f "$file" Package)" == llvm-sh-clang ]]
  [[ "$(dpkg-deb -f "$file" Version)" == '0.1.0~rc1-1' ]]
  [[ "$(dpkg-deb -f "$file" Architecture)" == amd64 ]]
  test -n "$(dpkg-deb -f "$file" Depends)"
  grep -F '/opt/llvm-sh/rc1/bin/clang' "$out/contents-$format.txt" || {
    echo 'Package missing /opt/llvm-sh/rc1/bin/clang' >&2; exit 10;
  }
  # The package manager verifies dependencies and actual installation.
  timed install sudo apt-get install -y "$file"
  dpkg-query -W -f='${Package} ${Version} ${Architecture} ${Status}\n' llvm-sh-clang \
    | tee "$out/installation-$format.txt"
  dpkg -V llvm-sh-clang
else
  file="$(find "$out" -maxdepth 1 -name '*.rpm' -print -quit)"
  test -f "$file"
  rpm -qpi "$file" | tee "$out/metadata-$format.txt"
  rpm -qp --requires "$file" | tee "$out/dependencies-$format.txt"
  rpm -qpl "$file" > "$out/paths-$format.txt"
  rpm -qplv "$file" > "$out/contents-$format.txt"
  [[ "$(rpm -qp --qf '%{NAME} %{VERSION}-%{RELEASE} %{ARCH}' "$file")" \
    == 'llvm-sh-clang 0.1.0-0.rc1.1 x86_64' ]]
  grep -F '/opt/llvm-sh/rc1/bin/clang' "$out/paths-$format.txt" || {
    echo 'Package missing /opt/llvm-sh/rc1/bin/clang' >&2; exit 10;
  }
  timed install dnf install -y "$file"
  rpm -qi llvm-sh-clang | tee "$out/installation-$format.txt"
  rpm -V llvm-sh-clang
fi
# All payload files belong to the private prefix; ancestor directories are OK.
if grep -Ev '^(\./)?(/)?(opt(/llvm-sh(/rc1(/.*)?)?)?/?|\.)$' "$out/paths-$format.txt"; then
  echo 'Package contains paths outside the private prefix' >&2; exit 10
fi
# Smoke-test the actual installed package as well as the staged install.
timed installed-smoke smoke "$prefix" "$format"
check_libraries "$prefix" "$format"
[[ "$(command -v clang)" == "$system_clang" ]]
[[ "$(readlink -f "$system_clang")" == "$system_clang_real" ]]
[[ "$(sha256sum "$system_clang_real")" == "$system_clang_sha" ]]
printf 'System compiler preserved: %s\n%s\n' "$system_clang" "$system_clang_sha" \
  | tee -a "$out/installation-$format.txt"
(cd "$out" && sha256sum "$(basename "$file")" > "$(basename "$file").sha256")
(cd "$out" && sha256sum --check "$(basename "$file").sha256")
stat --printf='%n %s bytes\n' "$file"

# The Fedora RPM job additionally publishes a movable Linux ZIP of the SAME
# installed LLVM + Clang files. Do not recompile or rebuild the compiler.
# Keep Unix executable permissions and symbolic links (zip -y / --symlinks).
if [[ "$format" == rpm ]]; then
  portable_zip="$out/llvm-sh-rc1-fedora45-linux-x86_64.zip"
  zip_base="${pkgroot}${prefix%/*}" # contains the installed rc1/ directory
  test -d "$zip_base/rc1/bin"
  # Reject absolute installed symlinks: they would break when the ZIP moves.
  while IFS= read -r -d '' link; do
    target="$(readlink "$link")"
    if [[ "$target" == /* ]]; then
      echo "Non-relocatable absolute symlink: $link -> $target" >&2
      exit 12
    fi
  done < <(find "$stage" -type l -print0)

  create_zip() { (cd "$zip_base" && zip -q -r -y "$portable_zip" rc1); }
  timed portable-zip create_zip
  unzip -tq "$portable_zip"

  # Unzip into a completely different directory and smoke-test from there.
  portable_test="$RUNNER_TEMP/llvm-sh-rc1-zip-relocation"
  mkdir -p "$portable_test"
  ( cd "$portable_test" && unzip -q "$portable_zip" )
  portable="$portable_test/rc1"
  test -x "$portable/bin/clang"
  test -x "$portable/bin/llc"
  test -x "$portable/bin/llvm-readobj"
  # Check every file, mode and symlink against the RPM's original staging tree.
  diff -qr --no-dereference "$stage" "$portable"
  (cd "$stage" && find . -printf '%P %m %l\n' | sort) > "$RUNNER_TEMP/stage-modes.txt"
  (cd "$portable" && find . -printf '%P %m %l\n' | sort) > "$RUNNER_TEMP/zip-modes.txt"
  diff -u "$RUNNER_TEMP/stage-modes.txt" "$RUNNER_TEMP/zip-modes.txt"

  # Make every original binary/library location unavailable during relocation.
  # A trap restores them even when a check fails; no second compiler build.
  restore_originals() {
    [[ ! -d "$bld.hidden" ]] || mv "$bld.hidden" "$bld"
    [[ ! -d "$pkgroot.hidden" ]] || mv "$pkgroot.hidden" "$pkgroot"
    [[ ! -d "$prefix.hidden" ]] || mv "$prefix.hidden" "$prefix"
  }
  trap restore_originals EXIT
  mv "$bld" "$bld.hidden"
  mv "$pkgroot" "$pkgroot.hidden"
  mv "$prefix" "$prefix.hidden"
  timed portable-smoke smoke "$portable" linux-zip
  check_libraries "$portable" linux-zip
  if grep -E '/opt/llvm-sh/rc1|llvm-sh-rc1-(build|stage)' "$out/libraries-linux-zip.txt"; then
    echo 'Portable runtime depends on an original location' >&2; exit 14
  fi
  restore_originals
  trap - EXIT

  ( cd "$out" && sha256sum "$(basename "$portable_zip")" \
    > "$(basename "$portable_zip").sha256" )
  (cd "$out" && sha256sum --check "$(basename "$portable_zip").sha256")
  stat --printf='%n %s bytes\n' "$portable_zip"
fi
cat "$out"/*.sha256
cat "$out/timings.tsv"
ls -lah "$out"
