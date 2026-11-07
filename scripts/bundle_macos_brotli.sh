#!/usr/bin/env bash
set -euo pipefail

# Recent Homebrew FreeType bottles pull in Brotli. macdeployqt copies the
# decoder, but can miss its @rpath dependency on the common library.
app_path=${1:?Usage: bundle_macos_brotli.sh path/to/UrConnect.app}
frameworks="$app_path/Contents/Frameworks"
decoder="$frameworks/libbrotlidec.1.dylib"
if [ ! -f "$decoder" ]; then
  exit 0
fi

dependencies=$(otool -L "$decoder")
if ! grep -q '@rpath/libbrotlicommon.1.dylib' <<< "$dependencies"; then
  exit 0
fi

brotli_root=$(brew --prefix brotli)
cp -L "$brotli_root/lib/libbrotlicommon.1.dylib" "$frameworks/libbrotlicommon.1.dylib"
install_name_tool -change '@rpath/libbrotlicommon.1.dylib' \
  '@loader_path/libbrotlicommon.1.dylib' "$decoder"
install_name_tool -id '@rpath/libbrotlicommon.1.dylib' "$frameworks/libbrotlicommon.1.dylib"
mkdir -p "$app_path/Contents/Resources/licenses"
cp "$brotli_root/LICENSE" "$app_path/Contents/Resources/licenses/brotli-LICENSE.txt"
echo 'Bundled the transitive Brotli dependency for the Cocoa platform plugin.'
otool -L "$decoder"
