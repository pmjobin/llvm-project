# Windows x64: LLVM + Clang RC1, portable ZIP. No installer, runtime, SDK or emulator.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$source = Join-Path $env:GITHUB_WORKSPACE 'source'
$build = Join-Path $env:RUNNER_TEMP 'llvm-sh-rc1-build'
$stage = Join-Path $env:RUNNER_TEMP 'llvm-sh-rc1'
$out = Join-Path $env:GITHUB_WORKSPACE 'dist'
$expected = 'a4ad03d58bd5a924de0ae286340ff7c6bb664ec2'
$expectedTree = 'afbc1ebbfcbae3811208ff4dcfb0930208d3f8a1'

$actual = (& git -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'git rev-parse HEAD failed' }
$tree = (& git -C $source rev-parse 'HEAD^{tree}').Trim()
if ($LASTEXITCODE -ne 0) { throw 'git rev-parse tree failed' }
if ($actual -ne $expected -or $tree -ne $expectedTree) {
  throw "RC1 source mismatch: commit=$actual tree=$tree"
}
if (!(Test-Path (Join-Path $source 'llvm/lib/Target/SH/CMakeLists.txt'))) {
  throw 'Missing SH backend in tagged source'
}
New-Item -ItemType Directory -Force -Path $out | Out-Null
$cmakeStage = $stage.Replace('\','/')
# Visual Studio is multi-configuration, so LLVM_DISTRIBUTION_COMPONENTS is
# not supported. Build the requested programs and install each CMake component.
$toolComponents = @(
  'clang', 'clang-resource-headers', 'llc', 'opt', 'llvm-as', 'llvm-dis',
  'llvm-readobj', 'llvm-objdump', 'llvm-ar', 'llvm-nm', 'llvm-link'
)
$buildTargets = @('clang', 'llc', 'opt', 'llvm-as', 'llvm-dis', 'llvm-readobj', 'llvm-objdump', 'llvm-ar', 'llvm-nm', 'llvm-link')

# Visual Studio generator supplies the 64-bit host toolchain without vcvars setup.
# Unlike Linux, Windows/MSVC does not support LLVM_BUILD_LLVM_DYLIB.
$cmakeArgs = @(
  '-S', (Join-Path $source 'llvm'), '-B', $build,
  '-G', 'Visual Studio 17 2022', '-A', 'x64', '-Thost=x64',
  '-DCMAKE_BUILD_TYPE=Release',
  "-DCMAKE_INSTALL_PREFIX=$cmakeStage",
  '-DLLVM_ENABLE_PROJECTS=clang',
  '-DLLVM_ENABLE_RUNTIMES=',
  '-DLLVM_TARGETS_TO_BUILD=X86',
  '-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SH',
  '-DLLVM_BUILD_LLVM_DYLIB=OFF',
  '-DLLVM_LINK_LLVM_DYLIB=OFF',
  '-DCLANG_LINK_CLANG_DYLIB=OFF',
  '-DLLVM_ENABLE_ASSERTIONS=ON',
  '-DLLVM_INSTALL_TOOLCHAIN_ONLY=OFF',
  '-DCLANG_ENABLE_CIR=OFF',
  '-DLLVM_ENABLE_LIBEDIT=OFF',
  '-DLLVM_ENABLE_LIBXML2=OFF',
  '-DLLVM_ENABLE_ZLIB=OFF',
  '-DLLVM_ENABLE_ZSTD=OFF',
  '-DLLVM_ENABLE_LZMA=OFF',
  '-DLLVM_ENABLE_LIBPFM=OFF',
  '-DLLVM_ENABLE_CURL=OFF',
  '-DLLVM_BUILD_DOCS=OFF', '-DLLVM_INCLUDE_TESTS=OFF',
  '-DLLVM_INCLUDE_EXAMPLES=OFF', '-DLLVM_INCLUDE_BENCHMARKS=OFF'
)
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw 'Windows LLVM CMake configure failed' }
# Multi-config Visual Studio: build named tools and install their components.
# Avoid a blanket 'cmake --install' which would include development libraries.
$buildArgs = @('--build', $build, '--config', 'Release', '--target') + $buildTargets + @('--parallel', '2')
& cmake @buildArgs
if ($LASTEXITCODE -ne 0) { throw 'Windows LLVM tool build failed' }
foreach ($component in $toolComponents) {
  Write-Host "Installing CMake component: $component"
  & cmake --install $build --config Release --component $component --prefix $cmakeStage
  if ($LASTEXITCODE -ne 0) { throw "Windows component installation failed: $component" }
}

$clang = Join-Path $stage 'bin/clang.exe'
$readobj = Join-Path $stage 'bin/llvm-readobj.exe'
# Check the actual staged installation, not just the build-tree executables.
foreach ($tool in @('clang', 'llc', 'opt', 'llvm-as', 'llvm-dis', 'llvm-readobj', 'llvm-objdump', 'llvm-ar', 'llvm-nm', 'llvm-link')) {
  $program = Join-Path $stage "bin/$tool.exe"
  if (!(Test-Path $program)) { throw "Staged tool missing after component installation: $program" }
}
# clang-resource-headers must be installed as well for a usable Clang package.
$resourceVersion = (& $clang --print-resource-dir).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Clang resource-dir query failed' }
if (!(Test-Path (Join-Path $resourceVersion 'include/stddef.h'))) {
  throw "Clang resource headers missing from staged installation: $resourceVersion"
}

$doc = Join-Path $stage 'share/doc/llvm-sh'
New-Item -ItemType Directory -Force -Path $doc | Out-Null
Copy-Item (Join-Path $source 'llvm/LICENSE.TXT') (Join-Path $doc 'LICENSE-LLVM.txt')
Copy-Item (Join-Path $source 'clang/LICENSE.TXT') (Join-Path $doc 'LICENSE-CLANG.txt')
@"
LLVM SH private RC1 CI Windows x64 build (not the originally qualified binary)
tag=llvm-sh-rc1
commit=$actual
tree=$tree
runner=windows-2022
llvm_enable_projects=clang
llvm_targets=X86;SH(experimental)
no GNU SH4 sysroot, no QEMU, no Doom, no LLVM runtimes
"@ | Set-Content -Path (Join-Path $stage 'BUILDINFO.txt') -Encoding utf8

& $clang --version
if ($LASTEXITCODE -ne 0) { throw 'clang.exe --version failed' }
$smoke = Join-Path $env:RUNNER_TEMP 'llvm-sh-smoke.c'
Set-Content -Path $smoke -Value 'int sh_add(int a,int b){return a+b;}' -Encoding ascii
$object = Join-Path $env:RUNNER_TEMP 'llvm-sh-smoke.o'
& $clang '--target=sh4-unknown-linux-gnu' '-m4' '-c' $smoke '-o' $object
if ($LASTEXITCODE -ne 0) { throw 'Windows-hosted SH4 code generation failed' }
$hdr = (& $readobj '--file-headers' $object | Out-String)
if ($LASTEXITCODE -ne 0 -or $hdr -notmatch 'EM_SH|SuperH') {
  throw 'SH4 ELF identification failed'
}
$hdr | Set-Content -Path (Join-Path $out 'smoke-windows.txt')

$archive = Join-Path $out 'llvm-sh-rc1-windows-x64.zip'
Compress-Archive -Path $stage -DestinationPath $archive -CompressionLevel Optimal -Force
if (!(Test-Path $archive)) { throw 'Archive was not created' }
$replay = Join-Path $env:RUNNER_TEMP 'llvm-sh-rc1-replay'
Expand-Archive -LiteralPath $archive -DestinationPath $replay -Force
$replayed = Join-Path $replay 'llvm-sh-rc1/bin/clang.exe'
if (!(Test-Path $replayed)) { throw 'Relocated clang.exe missing' }
& $replayed '--target=sh4-unknown-linux-gnu' '-m4' '-c' $smoke '-o' (Join-Path $env:RUNNER_TEMP 'replayed-sh4.o')
if ($LASTEXITCODE -ne 0) { throw 'Relocated compiler smoke failed' }
$checksum = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$checksum  $(Split-Path $archive -Leaf)" | Set-Content -Path "$archive.sha256" -Encoding ascii
Get-ChildItem $out | Format-Table Name,Length
