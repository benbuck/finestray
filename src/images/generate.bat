:: Copyright 2020 Benbuck Nason
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

pushd %~dp0

:: create icon for executable
magick convert -background none -define icon:auto-resize="16,24,32,48,64,72,96,128,256" -filter point emoji_u1f4e5.svg ../MinTray.ico

:: create icon for README.md
magick convert -background none -size 16x16 -filter point emoji_u1f4e5.svg icon.png

:: create installer header image (used at top of most installer screens)
magick -size 150x57 -define gradient:direction=east gradient:steelblue1-white installer_header.bmp
magick composite -background none -geometry 40x40+15+8 emoji_u1f4e5.svg installer_header.bmp BMP3:installer_header.bmp

:: create installer welcome image (used at left side of installer welcome screen)
magick -size 164x314 -define gradient:direction=east gradient:steelblue1-white installer_welcome.bmp
magick composite -background none -geometry 100x100+15+45 emoji_u1f4e5.svg installer_welcome.bmp BMP3:installer_welcome.bmp

popd
