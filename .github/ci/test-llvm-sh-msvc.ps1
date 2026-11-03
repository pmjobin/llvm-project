# Run from a VS 2022 x64 native tools environment with Ninja and sccache on PATH.
param(
  [ValidateSet('Backend', 'Validate', 'Report')]
  [string]$Stage = 'Backend',
  [string]$BuildDir,
  [string]$LogDir
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
Set-StrictMode -Version Latest
$source = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$tempRoot = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { [IO.Path]::GetTempPath() }
if (!$BuildDir) { $BuildDir = Join-Path $tempRoot 'llvm-sh-msvc-dev-build' }
if (!$LogDir) { $LogDir = Join-Path $tempRoot 'llvm-sh-msvc-dev-logs' }
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$timings = Join-Path $LogDir 'timings.jsonl'

function Invoke-Logged {
  param([string]$Name, [string]$Program, [string[]]$Arguments)
  Write-Host "::group::$Name"
  Write-Host "$Program $($Arguments -join ' ')"
  $watch = [Diagnostics.Stopwatch]::StartNew()
  $exitCode = -1
  try {
    & $Program @Arguments 2>&1 | Tee-Object -FilePath (Join-Path $LogDir "$Name.log")
    $exitCode = $LASTEXITCODE
  } finally {
    $watch.Stop()
    $seconds = [Math]::Round($watch.Elapsed.TotalSeconds, 2)
    [ordered]@{ stage = $Stage; command = $Name; seconds = $seconds; exit_code = $exitCode } |
      ConvertTo-Json -Compress | Add-Content -Path $timings
    Write-Host "${Name}: ${seconds}s, exit=$exitCode"
    Write-Host '::endgroup::'
  }
  if ($exitCode -ne 0) { throw "$Name failed (exit $exitCode); see $LogDir" }
}

if ($Stage -eq 'Report') {
  if (Get-Command sccache -ErrorAction SilentlyContinue) {
    Invoke-Logged 'sccache-final' 'sccache' @('--show-stats')
  }
  $summary = @('## LLVM SH native MSVC development', '',
    'Backend compilation and full validation are separate results. See command exit codes and lit reports.', '',
    '| Stage | Command | Seconds | Exit |', '| --- | --- | ---: | ---: |')
  if (Test-Path $timings) {
    foreach ($entry in Get-Content $timings) {
      $row = $entry | ConvertFrom-Json
      $summary += "| $($row.stage) | $($row.command) | $($row.seconds) | $($row.exit_code) |"
    }
  }
  foreach ($name in @('source-revision', 'compiler-identity', 'sccache-final', 'test-llvm-sh', 'test-clang-sh', 'test-lld-sh')) {
    $path = Join-Path $LogDir "$name.log"
    if (Test-Path $path) {
      $lines = if ($name.StartsWith('test-')) { @(Get-Content $path -Tail 12) } else { @(Get-Content $path) }
      $summary += @('', "### $name", '', '```text') + $lines + @('```')
    }
  }
  $summary | Set-Content (Join-Path $LogDir 'summary.md')
  if ($env:GITHUB_STEP_SUMMARY) { $summary | Add-Content $env:GITHUB_STEP_SUMMARY }
  exit 0
}

if ($Stage -eq 'Backend') {
  $marker = Join-Path $BuildDir 'sh-backend-passed.txt'
  if (Test-Path $marker) { Remove-Item $marker }
  Invoke-Logged 'source-revision' 'git' @('-C', $source, 'rev-parse', 'HEAD')
  Invoke-Logged 'compiler-path' 'where.exe' @('cl.exe')
  # A successful native compile plus /Bv identifies the real compiler, even
  # when other compiler drivers are preinstalled on the runner.
  $probe = Join-Path $LogDir 'compiler-probe.cpp'
  @'
#if !defined(_MSC_VER) || defined(__clang__)
#error Native Microsoft C++ is required
#endif
int sh_msvc_probe;
'@ | Set-Content $probe -Encoding ascii
  Invoke-Logged 'compiler-identity' 'cl.exe' @('/nologo', '/Bv', '/c', $probe, "/Fo$(Join-Path $LogDir 'compiler-probe.obj')")
  Invoke-Logged 'cmake-version' 'cmake' @('--version')
  Invoke-Logged 'ninja-version' 'ninja' @('--version')
  Invoke-Logged 'sccache-version' 'sccache' @('--version')
  Invoke-Logged 'sccache-reset' 'sccache' @('--zero-stats')
  $configure = @(
    '-S', (Join-Path $source 'llvm'), '-B', $BuildDir, '-G', 'Ninja',
    '-DCMAKE_C_COMPILER=cl.exe', '-DCMAKE_CXX_COMPILER=cl.exe',
    '-DCMAKE_C_COMPILER_LAUNCHER=sccache', '-DCMAKE_CXX_COMPILER_LAUNCHER=sccache',
    '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_POLICY_DEFAULT_CMP0141=NEW',
    '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded',
    '-DLLVM_ENABLE_PROJECTS=', '-DLLVM_ENABLE_RUNTIMES=',
    '-DLLVM_TARGETS_TO_BUILD=', '-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SH',
    '-DLLVM_ENABLE_ASSERTIONS=ON', '-DLLVM_ENABLE_LTO=OFF',
    '-DLLVM_BUILD_LLVM_DYLIB=OFF', '-DLLVM_LINK_LLVM_DYLIB=OFF',
    '-DLLVM_INCLUDE_TESTS=ON', '-DLLVM_BUILD_TESTS=OFF',
    '-DLLVM_INCLUDE_EXAMPLES=OFF', '-DLLVM_INCLUDE_BENCHMARKS=OFF',
    '-DLLVM_BUILD_DOCS=OFF', '-DLLVM_ENABLE_BINDINGS=OFF',
    '-DLLVM_ENABLE_LIBEDIT=OFF', '-DLLVM_ENABLE_LIBXML2=OFF',
    '-DLLVM_ENABLE_ZLIB=OFF', '-DLLVM_ENABLE_ZSTD=OFF',
    '-DLLVM_ENABLE_LZMA=OFF', '-DLLVM_ENABLE_LIBPFM=OFF', '-DLLVM_ENABLE_CURL=OFF'
  )
  Invoke-Logged 'configure-backend' 'cmake' $configure
  $identity = Get-ChildItem (Join-Path $BuildDir 'CMakeFiles/*/CMakeCXXCompiler.cmake') |
    Select-String -Pattern '^set\(CMAKE_CXX_COMPILER_ID "MSVC"\)'
  if (!$identity) { throw 'CMake did not select native MSVC' }
  # This is deliberately the first build invocation. Ninja stops scheduling
  # new work on the first error (-k 1); no Clang sources or packages are built.
  Invoke-Logged 'build-backend' 'cmake' @('--build', $BuildDir, '--target', 'LLVMSHCodeGen', '--', '-k', '1')
  Invoke-Logged 'sccache-backend' 'sccache' @('--show-stats')
  Set-Content (Join-Path $BuildDir 'sh-backend-passed.txt') 'LLVMSHCodeGen compiled successfully'
  exit 0
}

if (!(Test-Path (Join-Path $BuildDir 'sh-backend-passed.txt'))) {
  throw 'Run the Backend stage successfully before Validate'
}
Invoke-Logged 'lit-dependencies' 'python' @('-m', 'pip', 'install', '--disable-pip-version-check', 'psutil')
# Reuse the backend build tree and cache. Enabling Clang happens only here.
Invoke-Logged 'configure-validation' 'cmake' @('-S', (Join-Path $source 'llvm'), '-B', $BuildDir, '-DLLVM_ENABLE_PROJECTS=clang;lld', '-DCLANG_ENABLE_CIR=OFF')
Invoke-Logged 'build-validation' 'cmake' @('--build', $BuildDir, '--target', 'clang', 'lld', 'llvm-test-depends', 'clang-test-depends', 'lld-test-depends', '--', '-k', '1')
$bin = Join-Path $BuildDir 'bin'
Invoke-Logged 'built-clang-version' (Join-Path $bin 'clang.exe') @('--version')
$smoke = Join-Path $LogDir 'sh-smoke.c'
'int sh_add(int a, int b) { return a + b; }' | Set-Content $smoke -Encoding ascii
foreach ($triple in @('shle-unknown-linux-gnu', 'sh-unknown-linux-gnu')) {
  $object = Join-Path $LogDir "$triple.o"
  Invoke-Logged "smoke-$triple" (Join-Path $bin 'clang.exe') @("--target=$triple", '-mcpu=sh4', '-c', $smoke, '-o', $object)
  Invoke-Logged "elf-$triple" (Join-Path $bin 'llvm-readobj.exe') @('--file-headers', '--sections', '--symbols', $object)
  # Check the binary ELF header as well as the diagnostic text: ELF32,
  # requested byte order, ET_REL, EM_SH, and a nonempty .text section.
  $bytes = [IO.File]::ReadAllBytes($object)
  if ($bytes.Length -lt 52) { throw "Truncated ELF header: $triple" }
  $little = $triple.StartsWith('shle-')
  $encoding = if ($little) { 1 } else { 2 }
  $type = if ($little) { $bytes[16] + 256 * $bytes[17] } else { 256 * $bytes[16] + $bytes[17] }
  $machine = if ($little) { $bytes[18] + 256 * $bytes[19] } else { 256 * $bytes[18] + $bytes[19] }
  $flags = if ($little) { $bytes[36] + 256 * $bytes[37] + 65536 * $bytes[38] + 16777216 * $bytes[39] } else { 16777216 * $bytes[36] + 65536 * $bytes[37] + 256 * $bytes[38] + $bytes[39] }
  if ($bytes[0] -ne 127 -or [Text.Encoding]::ASCII.GetString($bytes, 1, 3) -ne 'ELF' -or
      $bytes[4] -ne 1 -or $bytes[5] -ne $encoding -or $type -ne 1 -or $machine -ne 42 -or $flags -ne 9) {
    throw "Invalid SuperH ELF32 relocatable header: $triple"
  }
  $headers = Get-Content (Join-Path $LogDir "elf-$triple.log") -Raw
  if ($headers -notmatch '(?s)Name: \.text\b[^}]*\bSize: [1-9][0-9]*' -or $headers -notmatch 'Name: sh_add\b') {
    throw "Missing code or sh_add symbol: $triple"
  }
}
$lit = Join-Path $bin 'llvm-lit.py'
$llvmTests = @((Join-Path $source 'llvm/test/CodeGen/SH'), (Join-Path $source 'llvm/test/MC/SH'))
Invoke-Logged 'test-llvm-sh' 'python' (@($lit, '-v', '-j', '2', '--timeout=120', '--xunit-xml-output', (Join-Path $LogDir 'llvm-sh.xml')) + $llvmTests)
$clangTests = @(Get-ChildItem (Join-Path $source 'clang/test/*/sh*') -File |
  Where-Object { $_.BaseName -eq 'sh' -or $_.Name.StartsWith('sh-') } |
  Select-Object -ExpandProperty FullName)
$clangTests += (Join-Path $source 'clang/test/CodeGen/SH')
Invoke-Logged 'test-clang-sh' 'python' (@($lit, '-v', '-j', '2', '--timeout=120', '--xunit-xml-output', (Join-Path $LogDir 'clang-sh.xml')) + $clangTests)
$lldTests = @(Get-ChildItem (Join-Path $source 'lld/test/ELF/sh-*') -File | Select-Object -ExpandProperty FullName)
Invoke-Logged 'test-lld-sh' 'python' (@($lit, '-v', '-j', '2', '--timeout=120', '--xunit-xml-output', (Join-Path $LogDir 'lld-sh.xml')) + $lldTests)
