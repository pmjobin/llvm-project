# Native MSVC Windows x64: LLVM/Clang/LLD RC1 reissue candidate, portable ZIP.
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
Set-StrictMode -Version Latest

$source = Join-Path $env:GITHUB_WORKSPACE 'source'
$build = Join-Path $env:RUNNER_TEMP 'llvm-sh-rc1-build'
$stage = Join-Path $env:RUNNER_TEMP 'llvm-sh-rc1'
$out = Join-Path $env:GITHUB_WORKSPACE 'dist'
$expected = $env:LLVM_SH_CANDIDATE_COMMIT
$expectedTree = $env:LLVM_SH_CANDIDATE_TREE
if ($expected -notmatch '^[0-9a-f]{40}$' -or $expectedTree -notmatch '^[0-9a-f]{40}$' -or
    $expected -in @('090c682597f7155373681807330fbb0ac343a33c',
                    'c8f8d9181205796eaa58f38700906ee19a6444c8')) {
  throw 'Select full new RC1 candidate commit/tree identities; published RC1 stays unchanged'
}
$short = $expected.Substring(0, 12)
$stem = "llvm-sh-rc1-candidate-$short"

$actual = (& git -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'git rev-parse HEAD failed' }
$tree = (& git -C $source rev-parse 'HEAD^{tree}').Trim()
if ($LASTEXITCODE -ne 0) { throw 'git rev-parse tree failed' }
if ($actual -ne $expected -or $tree -ne $expectedTree) {
  throw "RC1 source mismatch: commit=$actual tree=$tree"
}
if (!(Test-Path (Join-Path $source 'lld/ELF/Arch/SH.cpp'))) {
  throw 'Missing SH LLD implementation in candidate source'
}
if (!(Test-Path (Join-Path $source 'runtimes/sh/picolibc.json'))) {
  throw 'Missing pinned SH C/C++ library recipe in candidate source'
}
& git -C $source diff --quiet
if ($LASTEXITCODE -ne 0) { throw 'Dirty candidate source' }
& git -C $source diff --cached --quiet
if ($LASTEXITCODE -ne 0) { throw 'Dirty candidate index' }
New-Item -ItemType Directory -Force -Path $out | Out-Null
$systemCompiler = Get-Command clang.exe -ErrorAction SilentlyContinue
$systemCompilerPath = if ($systemCompiler) { $systemCompiler.Source } else { $null }
$systemCompilerHash = if ($systemCompiler) { (Get-FileHash $systemCompilerPath -Algorithm SHA256).Hash } else { $null }
$timings = Join-Path $out 'timings-windows.tsv'
"phase`tseconds" | Set-Content -Path $timings -Encoding ascii
function Invoke-Timed {
  param([string]$Name, [scriptblock]$Operation)
  $watch = [Diagnostics.Stopwatch]::StartNew()
  try { & $Operation } finally {
    $watch.Stop()
    $seconds = [Math]::Round($watch.Elapsed.TotalSeconds, 2)
    "$Name`t$seconds" | Add-Content -Path $timings -Encoding ascii
    Write-Host "${Name}: ${seconds}s"
  }
}
$cmakeStage = $stage.Replace('\','/')
# Visual Studio is multi-configuration, so LLVM_DISTRIBUTION_COMPONENTS is
# not supported. Build the requested programs and install each CMake component.
$toolComponents = @(
  'clang', 'clang-resource-headers', 'llc', 'opt', 'llvm-as', 'llvm-dis',
  'llvm-readobj', 'llvm-objdump', 'llvm-ar', 'llvm-ranlib', 'llvm-mc', 'llvm-objcopy', 'llvm-nm', 'llvm-link', 'lld'
)
$buildTargets = @(Get-Content (Join-Path $source '.github/ci/llvm-sh-test-tools.txt'))

# Visual Studio generator supplies the 64-bit host toolchain without vcvars setup.
# Unlike Linux, Windows/MSVC does not support LLVM_BUILD_LLVM_DYLIB.
$cmakeArgs = @(
  '-S', (Join-Path $source 'llvm'), '-B', $build,
  '-G', 'Visual Studio 17 2022', '-A', 'x64', '-Thost=x64',
  '-DCMAKE_BUILD_TYPE=Release',
  "-DCMAKE_INSTALL_PREFIX=$cmakeStage",
  '-DLLVM_ENABLE_PROJECTS=clang;lld',
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
  '-DLLVM_BUILD_DOCS=OFF', '-DLLVM_INCLUDE_TESTS=ON',
  '-DLLVM_INCLUDE_EXAMPLES=OFF', '-DLLVM_INCLUDE_BENCHMARKS=OFF'
)
Invoke-Timed 'configure' {
  & cmake @cmakeArgs
  if ($LASTEXITCODE -ne 0) { throw 'Windows LLVM CMake configure failed' }
  $native = Get-ChildItem (Join-Path $build 'CMakeFiles/*/CMakeCXXCompiler.cmake') |
    Select-String -Pattern '^set\(CMAKE_CXX_COMPILER_ID "MSVC"\)'
  if (!$native) { throw 'Candidate must be built with native MSVC' }
}
# Multi-config Visual Studio: build named tools and install their components.
# Avoid a blanket 'cmake --install' which would include development libraries.
$buildArgs = @('--build', $build, '--config', 'Release', '--target') + $buildTargets + @('--parallel', '2')
Invoke-Timed 'build' {
  & cmake @buildArgs
  if ($LASTEXITCODE -ne 0) { throw 'Windows LLVM tool build failed' }
}
$script:sourceRegressionsExit = 0
Invoke-Timed 'source-regressions' {
  & python (Join-Path $source '.github/ci/check-llvm-sh-regressions.py') `
    --source $source --build $build --configuration Release --logs (Join-Path $out 'source-tests-windows')
  $script:sourceRegressionsExit = $LASTEXITCODE
  "source_tests_exit=$script:sourceRegressionsExit" |
    Set-Content (Join-Path $out 'source-tests-windows-status.txt')
}
# Retain package/link receipts for diagnosis even after a source-test failure.
# The final check still fails the job; an uploaded candidate is not a PASS.
Invoke-Timed 'stage' {
  foreach ($component in $toolComponents) {
    Write-Host "Installing CMake component: $component"
    & cmake --install $build --config Release --component $component --prefix $cmakeStage
    if ($LASTEXITCODE -ne 0) { throw "Windows component installation failed: $component" }
  }
}
Invoke-Timed 'target-runtime' {
  & python (Join-Path $source '.github/ci/stage-llvm-sh-freestanding.py') `
    --source $source --prefix $stage --build-dir (Join-Path $env:RUNNER_TEMP 'sh4-runtime-build') `
    --commit $actual --tree $tree --logs-dir (Join-Path $out 'target-runtime-build')
  if ($LASTEXITCODE -ne 0) { throw 'SH freestanding runtime staging failed' }
}

$clang = Join-Path $stage 'bin/clang.exe'
# Check the actual staged installation, not just the build-tree executables.
foreach ($tool in @('clang', 'clang++', 'ld.lld', 'lld', 'llc', 'opt', 'llvm-as', 'llvm-dis', 'llvm-readobj', 'llvm-objdump', 'llvm-ar', 'llvm-ranlib', 'llvm-mc', 'llvm-objcopy', 'llvm-nm', 'llvm-link')) {
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
LLVM SH RC1 Windows x64 reissue candidate (UNPUBLISHED)
published_tag_unchanged=llvm-sh-rc1
published_commit=090c682597f7155373681807330fbb0ac343a33c
commit=$actual
tree=$tree
automation_commit=$env:GITHUB_SHA
workflow_run=$env:LLVM_SH_RUN_URL
runner=windows-2022
llvm_enable_projects=clang;lld
llvm_targets=X86;SH(experimental)
target_runtime=shle-unknown-none-elf/sh4-soft (bounded compiler-rt/C/C++ subset)
target_libraries=SH4 little-endian soft argument ABI; Picolibc 1.8.12; static libc++/libc++abi; see SH4-LIBRARIES.rst
no GNU SH4 SDK/sysroot/libgcc; no packaged emulator; see SH4-FREESTANDING.rst
"@ | Set-Content -Path (Join-Path $stage 'BUILDINFO.txt') -Encoding utf8

$smoke = Join-Path $env:RUNNER_TEMP 'llvm-sh-smoke.c'
@'
#include <stddef.h>
int sh_add(int a,int b){return a+b;}
'@ | Set-Content -Path $smoke -Encoding ascii
function Invoke-SH4Smoke {
  param([string]$Root, [string]$Label)
  $compiler = Join-Path $Root 'bin/clang.exe'
  $inspector = Join-Path $Root 'bin/llvm-readobj.exe'
  $report = Join-Path $out "smoke-$Label.txt"
  $version = (& $compiler --version | Out-String)
  if ($LASTEXITCODE -ne 0) { throw "Compiler execution failed: $Label" }
  $version | Tee-Object -FilePath $report
  if (!$version.Contains($actual)) { throw "Packaged Clang source identity mismatch: $Label" }
  $resource = (& $compiler --print-resource-dir).Trim()
  if ($LASTEXITCODE -ne 0) { throw "Resource directory query failed: $Label" }
  $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
  if (![IO.Path]::GetFullPath($resource).StartsWith($rootPath, [StringComparison]::OrdinalIgnoreCase) -or
      !(Test-Path (Join-Path $resource 'include/stddef.h'))) {
    throw "Resource headers do not resolve inside tested package: $Label ($resource)"
  }
  $object = Join-Path $env:RUNNER_TEMP "$Label-sh4.o"
  "$compiler --target=shle-unknown-linux-gnu -mcpu=sh4 -c $smoke -o $object" |
    Tee-Object -FilePath $report -Append
  & $compiler '--target=shle-unknown-linux-gnu' '-mcpu=sh4' '-c' $smoke '-o' $object
  if ($LASTEXITCODE -ne 0) { throw "SH4 code generation failed: $Label" }
  $headers = (& $inspector '--file-headers' '--sections' '--symbols' $object | Out-String)
  if ($LASTEXITCODE -ne 0) { throw "SH4 object inspection failed: $Label" }
  $headers | Tee-Object -FilePath $report -Append
  foreach ($pattern in @('(?m)^Format: elf32-shl\r?$', '(?m)^Arch: shle\r?$',
      'Class: 32-bit \(0x1\)', 'DataEncoding: LittleEndian \(0x1\)',
      'Type: Relocatable \(0x1\)', 'Machine: EM_SH \(0x2A\)', 'Flags \[ \(0x9\)',
      '(?s)Name: \.text\b[^}]*\bSize: [1-9][0-9]*', 'Name: sh_add\b')) {
    if ($headers -notmatch $pattern) { throw "SH4 object check failed: $Label ($pattern)" }
  }
  # Inspect host DLL dependencies with the tested package's own tool.
  $dependencies = Join-Path $out "runtime-$Label.txt"
  foreach ($program in Get-ChildItem (Join-Path $Root 'bin/*.exe')) {
    $imports = (& $inspector '--coff-imports' $program.FullName | Out-String)
    if ($LASTEXITCODE -ne 0) { throw "PE import inspection failed: $program" }
    $program.Name | Add-Content -Path $dependencies
    $imports -split '\r?\n' | Where-Object { $_ -match '^\s*Name:.*\.dll\s*$' } |
      Add-Content -Path $dependencies
  }
  Get-Content $dependencies
}
Invoke-Timed 'staged-smoke' { Invoke-SH4Smoke $stage 'windows' }
function Invoke-Freestanding {
  param([string]$Root, [string]$Label)
  & python (Join-Path $Root 'share/llvm-sh/examples/SHFreestanding/check.py') `
    --prefix $Root --build-dir (Join-Path $out "freestanding-$Label")
  if ($LASTEXITCODE -ne 0) { throw "Packaged SH compile/archive/link qualification failed: $Label" }
  & python (Join-Path $Root 'share/llvm-sh/examples/SHFreestanding/libraries/check.py') `
    --prefix $Root --build-dir (Join-Path $out "libraries-$Label")
  if ($LASTEXITCODE -ne 0) { throw "Packaged SH C/C++ library qualification failed: $Label" }
  # No native SH Linux-user QEMU is available on Windows. receipt.json records
  # execution UNRUN/PARTIAL; the Linux qualification executes the same profile.
}
Invoke-Timed 'staged-freestanding' { Invoke-Freestanding $stage 'windows' }

$archive = Join-Path $out "$stem-windows-x64.zip"
Invoke-Timed 'zip' {
  Compress-Archive -Path $stage -DestinationPath $archive -CompressionLevel Optimal -Force
}
if (!(Test-Path $archive)) { throw 'Archive was not created' }
$replay = Join-Path $env:RUNNER_TEMP 'llvm-sh-rc1 replay with spaces'
Invoke-Timed 'extract' { Expand-Archive -LiteralPath $archive -DestinationPath $replay -Force }
$replayed = Join-Path $replay 'llvm-sh-rc1/bin/clang.exe'
if (!(Test-Path $replayed)) { throw 'Relocated clang.exe missing' }
$originalFiles = @(Get-ChildItem $stage -File -Recurse)
$relocatedFiles = @(Get-ChildItem (Join-Path $replay 'llvm-sh-rc1') -File -Recurse)
if ($originalFiles.Count -ne $relocatedFiles.Count) { throw 'ZIP file-count mismatch' }
foreach ($file in $originalFiles) {
  $relative = $file.FullName.Substring($stage.Length).TrimStart('\', '/')
  $relocatedFile = Join-Path (Join-Path $replay 'llvm-sh-rc1') $relative
  if (!(Test-Path -LiteralPath $relocatedFile) -or
      (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne
      (Get-FileHash -LiteralPath $relocatedFile -Algorithm SHA256).Hash) {
    throw "ZIP payload mismatch: $relative"
  }
}
try {
  Move-Item -LiteralPath $build -Destination "$build.hidden"
  Move-Item -LiteralPath $stage -Destination "$stage.hidden"
  Invoke-Timed 'relocated-smoke' {
    Invoke-SH4Smoke (Join-Path $replay 'llvm-sh-rc1') 'windows-relocated'
  }
  Invoke-Timed 'relocated-freestanding' {
    Invoke-Freestanding (Join-Path $replay 'llvm-sh-rc1') 'windows-relocated'
  }
} finally {
  if (Test-Path "$stage.hidden") { Move-Item -LiteralPath "$stage.hidden" -Destination $stage }
  if (Test-Path "$build.hidden") { Move-Item -LiteralPath "$build.hidden" -Destination $build }
}
$checksum = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$checksum  $(Split-Path $archive -Leaf)" | Set-Content -Path "$archive.sha256" -Encoding ascii
$checksumFile = Get-Content -LiteralPath "$archive.sha256" -Raw
if ($checksumFile -match '^([0-9a-f]{64})  llvm-sh-rc1-candidate-[0-9a-f]{12}-windows-x64\.zip\r?\n?$') {
  $expectedChecksum = $Matches[1]
} else { throw 'Invalid ZIP checksum file' }
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedChecksum) {
  throw 'ZIP checksum verification failed'
}
Get-Content "$archive.sha256"
$systemAfter = Get-Command clang.exe -ErrorAction SilentlyContinue
if ($systemCompilerPath) {
  if (!$systemAfter -or $systemAfter.Source -ne $systemCompilerPath -or
      (Get-FileHash $systemCompilerPath -Algorithm SHA256).Hash -ne $systemCompilerHash) {
    throw 'System Clang changed during ZIP qualification'
  }
} elseif ($systemAfter) { throw 'System Clang command unexpectedly changed' }
"System compiler preserved: $systemCompilerPath $systemCompilerHash" |
  Set-Content (Join-Path $out 'system-compiler-windows.txt')
Get-Content $timings
Get-ChildItem $out | Format-Table Name,Length
if ($script:sourceRegressionsExit -ne 0) { throw 'Native Windows SH regression failure; candidate remains unqualified' }
