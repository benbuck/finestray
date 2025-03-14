# Copyright 2020 Benbuck Nason
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

param (
    [Parameter(Mandatory = $true)][string]$oldVersion,
    [Parameter(Mandatory = $true)][string]$newVersion
)

Write-Output "Updating version from $oldVersion to $newVersion"

$oldVersionComponents = $oldVersion -split '\.'
$newVersionComponents = $newVersion -split '\.'
if ($oldVersionComponents.Length -ne 2 -or $newVersionComponents.Length -ne 2) {
    Write-Error "Version must be in the format major.minor"
    exit 1
}

$oldVersion4 = "$oldVersion.0.0"
$newVersion4 = "$newVersion.0.0"
$oldVersion4Commas = "$($oldVersionComponents[0]),$($oldVersionComponents[1]),0,0"
$newVersion4Commas = "$($newVersionComponents[0]),$($newVersionComponents[1]),0,0"

$dateString = Get-Date -Format "yyyy-MM-dd"

$changeList = (git log --pretty=format:"- %s" v$oldVersion..HEAD) -join "`n"

Write-Output "Updating CHANGELOG.md"
$content = (Get-Content CHANGELOG.md)
$content = $content -replace `
    "\[unreleased\]: https://github.com/benbuck/finestray/compare/v$oldVersion...HEAD", `
    "[unreleased]: https://github.com/benbuck/finestray/compare/v$newVersion...HEAD`n[$newVersion]: https://github.com/benbuck/finestray/compare/v$oldVersion...v$newVersion"
if (!($content -match "\[$newVersion\] -")) {
    $content = $content -replace "## \[Unreleased\]", "## [Unreleased]`n`n## [$newVersion] - $dateString`n`n$changeList"
}
Set-Content -Path CHANGELOG.md -Value $content

Write-Output "Updating CMakeLists.txt"
$content = (Get-Content CMakeLists.txt)
$content = $content -replace "VERSION $oldVersion", "VERSION $newVersion"
Set-Content -Path CMakeLists.txt -Value $content

Write-Output "Updating README.md"
$content = (Get-Content README.md)
$content = $content -replace "-$oldVersion-", "-$newVersion-"
$content = $content -replace "/v$oldVersion/", "/v$newVersion/"
Set-Content -Path README.md -Value $content

Write-Output "Updating Finestray.rc"
$content = (Get-Content src/Finestray.rc)
$content = $content -replace "#define APP_VERSION $oldVersion4Commas", "#define APP_VERSION $newVersion4Commas"
$content = $content -replace "#define APP_VERSION_STRING ""$oldVersion4""", "#define APP_VERSION_STRING ""$newVersion4"""
$content = $content -replace "#define APP_VERSION_STRING_SIMPLE ""$oldVersion""", "#define APP_VERSION_STRING_SIMPLE ""$newVersion"""
$content = $content -replace "#define APP_DATE "".*""", "#define APP_DATE ""$dateString"""
Set-Content -Path src/Finestray.rc -Value $content

Write-Output "Building"
.\build-all.bat

Write-Output "Starting new version, please exit when done testing"
.\build\vstudio\Release\Finestray.exe
Start-Sleep -Seconds 2
# invoke twice to display the settings dialog
.\build\vstudio\Release\Finestray.exe
Start-Sleep -Seconds 2

# note that this is not the final installer, the real one will be built by github actions
Write-Output "Starting new installer, please exit when done installing"
$out = Invoke-Expression ".\build\vstudio\Finestray-$newVersion-win64.exe" | Out-String
Write-Output $out

Write-Output "Updating version in git"
git add --all
git commit --message "Update version to $newVersion"
git tag --force v$newVersion
