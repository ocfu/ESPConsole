
# Version Bumping
.PHONY: major minor patch git
major:
	./bump_version.sh major
minor:
	./bump_version.sh minor
patch:
	./bump_version.sh patch
git:
	./bump_version.sh git

