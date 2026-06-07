#!/bin/bash
set -e

APP_NAME="Tico Limiter"
VERSION="1.0.0"
DMG_NAME="TicoLimiter-${VERSION}-macOS.dmg"
BUILD_DIR="/Users/midimac/KawaiiLimiter/build/TicoLimiter_artefacts/Release"
OUTPUT_DIR="/Users/midimac/KawaiiLimiter/dist"
STAGING_DIR="/tmp/tico-dmg-staging"

echo "=== Packaging Tico Limiter DMG ==="

# Clean previous
rm -rf "$STAGING_DIR" "$OUTPUT_DIR"
mkdir -p "$STAGING_DIR" "$OUTPUT_DIR"

# Copy plugins
echo "Copying VST3..."
cp -R "$BUILD_DIR/VST3/$APP_NAME.vst3" "$STAGING_DIR/"

echo "Copying AU..."
cp -R "$BUILD_DIR/AU/$APP_NAME.component" "$STAGING_DIR/"

echo "Copying Standalone..."
cp -R "$BUILD_DIR/Standalone/$APP_NAME.app" "$STAGING_DIR/"

# Applications symlink for drag-to-install
ln -s /Applications "$STAGING_DIR/Applications"

# Remove quarantine attributes (so users don't get Gatekeeper warnings)
echo "Removing quarantine attributes..."
xattr -r -d com.apple.quarantine "$STAGING_DIR" 2>/dev/null || true

# Verify staging
echo "Staging contents:"
ls -la "$STAGING_DIR"

# Create compressed read-only DMG
echo "Creating DMG..."
rm -f "$OUTPUT_DIR/$DMG_NAME"
hdiutil create \
  -volname "$APP_NAME" \
  -srcfolder "$STAGING_DIR" \
  -fs HFS+ \
  -format UDZO \
  -imagekey zlib-level=9 \
  -quiet \
  "$OUTPUT_DIR/$DMG_NAME"

# Remove quarantine from the DMG itself
xattr -d com.apple.quarantine "$OUTPUT_DIR/$DMG_NAME" 2>/dev/null || true

# Verify
echo "Verifying..."
hdiutil verify "$OUTPUT_DIR/$DMG_NAME"

# Clean staging
rm -rf "$STAGING_DIR"

echo ""
echo "=== Done! ==="
echo "DMG: $OUTPUT_DIR/$DMG_NAME"
ls -lh "$OUTPUT_DIR/$DMG_NAME"
