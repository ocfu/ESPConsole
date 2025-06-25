# bump_version.sh - A script to manage semantic versioning and Git tagging for a project.
#
# Usage:
#   ./bump_version.sh [major|minor|patch|git] [commit message (for git)]
#
# Description:
#   - This script automates version bumping (major, minor, patch) in a version header file (version.h).
#   - It ensures that errors are not silently ignored by using 'set -e'.
#   - If the version file does not exist, it creates one with an initial version of 0.0.0.
#   - For version bumps:
#       * 'major' increments the major version and resets minor and patch to 0.
#       * 'minor' increments the minor version and resets patch to 0.
#       * 'patch' increments the patch version.
#   - For 'git' argument:
#       * Checks for uncommitted changes (excluding version.h).
#       * Extracts the current version from version.h.
#       * Commits the version.h change, tags the commit with the version, and pushes to the remote repository.
#       * Optionally accepts a custom commit message.
#
# Arguments:
#   major   - Bump the major version (X.0.0).
#   minor   - Bump the minor version (X.Y.0).
#   patch   - Bump the patch version (X.Y.Z).
#   git     - Commit, tag, and push the current version in version.h.
#
# Requirements:
#   - Bash shell
#   - Git installed and initialized in the project directory
#
# Example:
#   ./bump_version.sh patch
#   ./bump_version.sh git "Release new patch version"
#!/bin/bash
# filepath: /Users/ocfu/Dev/VSCode/ESPConsolePlus/bump_version.sh

# This script will exit immediately if any command exits with a non-zero status.
# The 'set -e' command is used to ensure that errors are not silently ignored.
set -e

VERSION_FILE="version.h"
VERSION_DEFINE="LIB_VERSION"
LIB_PROPS="library.properties"

if [[ -f "$LIB_PROPS" ]]; then
  VERSION_DEFINE="LIB_VERSION"
fi

usage() {
  echo "Usage: $0 [major|minor|patch|git] [commit message (for git)]"
  exit 1
}

if [[ $# -ne 1 ]]; then
  usage
fi

# Handles version bumping and git operations.
# For "git": checks for uncommitted changes (except $VERSION_FILE), extracts version, commits, tags, and pushes.
if [[ "$1" == "git" ]]; then
  # Check for uncommitted changes except version.h
  git_status=$(git status --porcelain | grep -Ev '$VERSION_FILE|$LIB_PROPS' || true)
  if [[ -n "$git_status" ]]; then
    echo "Error: Uncommitted changes present (except $VERSION_FILE):"
    echo "$git_status"
    exit 1
  fi

  # Extract version from version.h
  version=$(grep "#define $VERSION_DEFINE" "$VERSION_FILE" | sed -E 's/.*"([0-9]+\.[0-9]+\.[0-9]+)".*/\1/')
  if [[ -z "$version" ]]; then
    echo "Error: Could not extract version from $VERSION_FILE"
    exit 1
  fi

  # Commit, tag, and push
  prev_version=$(git show HEAD:$VERSION_FILE | grep "#define $VERSION_DEFINE" | sed -E 's/.*"([0-9]+\.[0-9]+\.[0-9]+)".*/\1/')
  shift
  if [[ $# -ge 1 ]]; then
    git commit -m "$*"
  else
    git commit -m "Bump version: $prev_version → $version"
  fi
  git tag "v$version"           
  git push
  git push origin "v$version"
  echo "Committed, tagged, and pushed version $version"
  exit 0
fi

# Create version.h with initial version if it doesn't exist
if [[ ! -f $VERSION_FILE ]]; then
  echo "// Auto-generated version file" > "$VERSION_FILE"
  echo "#pragma once" >> "$VERSION_FILE"
  echo "#define $VERSION_DEFINE \"0.0.0\"" >> "$VERSION_FILE"
  echo "Created $VERSION_FILE with initial version 0.0.0"
fi

# Extract current version
current=$(grep "#define $VERSION_DEFINE" "$VERSION_FILE" | sed -E 's/.*"([0-9]+)\.([0-9]+)\.([0-9]+)".*/\1 \2 \3/')
read major minor patch <<< "$current"

case "$1" in
  major)
    major=$((major + 1))
    minor=0
    patch=0
    ;;
  minor)
    minor=$((minor + 1))
    patch=0
    ;;
  patch)
    patch=$((patch + 1))
    ;;
  *)
    usage
    ;;
esac

new_version="$major.$minor.$patch"

# Update the version in the file (with quotes)
sed -i '' -E "s/(#define $VERSION_DEFINE) \"[0-9]+\.[0-9]+\.[0-9]+\"/\1 \"$new_version\"/" "$VERSION_FILE"
git add "$VERSION_FILE"

# Also update library.properties if it exists (without quotes)
if [[ -f "$LIB_PROPS" ]]; then
  echo "Updating version in $LIB_PROPS to $new_version"
  sed -i.bak -E "s/^(version[[:space:]]*=[[:space:]]*).*/\1$new_version/" "$LIB_PROPS"
  rm -f "$LIB_PROPS.bak"
  git add "$LIB_PROPS"
fi

echo "Version updated to $new_version"