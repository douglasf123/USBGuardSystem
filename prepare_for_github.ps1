param(
    [string]$RepoRoot = "$(Resolve-Path .)"
)

Write-Host "Cleaning build artifacts..."
Get-ChildItem -Path $RepoRoot -Include bin,obj -Recurse -Force | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "Creating .zip release package..."
$zip = Join-Path $RepoRoot "USBGuard.Release.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $RepoRoot "*") -DestinationPath $zip -Force
Write-Host "Done: $zip"