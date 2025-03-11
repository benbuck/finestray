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

$content = (Get-Content CMakeLists.txt)
$content = $content -replace "VERSION $oldVersion", "VERSION $newVersion"
Set-Content -Path CMakeLists.txt -Value $content

$content = (Get-Content README.md)
$content = $content -replace "-$oldVersion-", "-$newVersion-"
$content = $content -replace "/v$oldVersion/", "/v$newVersion/"
Set-Content -Path README.md -Value $content

$dateString = Get-Date -Format "yyyy-MM-dd"
$content = (Get-Content src/Finestray.rc)
$content = $content -replace "#define APP_VERSION $($oldVersionComponents[0]),$($oldVersionComponents[1]),0,0", "#define APP_VERSION $($newVersionComponents[0]),$($newVersionComponents[1]),0,0"
$content = $content -replace "#define APP_VERSION_STRING ""$oldVersion.0.0""", "#define APP_VERSION_STRING ""$newVersion.0.0"""
$content = $content -replace "#define APP_VERSION_STRING_SIMPLE ""$oldVersion""", "#define APP_VERSION_STRING_SIMPLE ""$newVersion"""
$content = $content -replace "#define APP_DATE "".*""", "#define APP_DATE ""$dateString"""
Set-Content -Path src/Finestray.rc -Value $content

git commit --all --message "Update version to $newVersion"

.\build-all.bat

Write-Output "Starting new version, please exit when done testing"
$out = .\build\vstudio\Release\Finestray.exe | Out-String
Write-Output $out

Write-Output "Starting new installer, please exit when done installing"
$out = Invoke-Expression ".\build\vstudio\Finestray-$newVersion-win64.exe" | Out-String
Write-Output $out

git tag --force v$newVersion
