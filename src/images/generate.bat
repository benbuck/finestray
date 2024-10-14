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

set APP_IMAGE_SOURCE=finestray.svg
set ABOUT_IMAGE_SOURCE=emoji_u2139.svg
set EXIT_IMAGE_SOURCE=emoji_u274c.svg
set SETTINGS_IMAGE_SOURCE=emoji_u2699.svg

:: create app icon
magick -background none -define icon:auto-resize="16,24,32,48,64,72,96,128,256" -filter point %APP_IMAGE_SOURCE% ../Finestray.ico

:: create app bitmap
magick -background none -filter point -size 16x16 %APP_IMAGE_SOURCE% -define bmp3:alpha=true BMP3:app.bmp

:: create about bitmap
magick -background none -filter point -size 16x16 %ABOUT_IMAGE_SOURCE% -define bmp3:alpha=true BMP3:about.bmp

:: create exit bitmap
magick -background none -filter point -size 16x16 %EXIT_IMAGE_SOURCE% -define bmp3:alpha=true BMP3:exit.bmp

:: create settings bitmap
magick -background none -filter point -size 16x16 %SETTINGS_IMAGE_SOURCE% -define bmp3:alpha=true BMP3:settings.bmp
magick identify settings.bmp

:: create icon for README.md
magick -background none -define png:exclude-chunks=date,time -filter point -size 16x16 %APP_IMAGE_SOURCE% icon.png

:: create installer header image (used at top of most installer screens)
magick -size 150x57 -define gradient:direction=east gradient:grey50-white installer_header.bmp
magick composite -background none -geometry 40x40+15+8 %APP_IMAGE_SOURCE% installer_header.bmp BMP3:installer_header.bmp

:: create installer welcome image (used at left side of installer welcome screen)
magick -size 164x314 -define gradient:direction=east gradient:grey50-white installer_welcome.bmp
magick composite -background none -geometry 100x100+15+45 %APP_IMAGE_SOURCE% installer_welcome.bmp BMP3:installer_welcome.bmp

popd
