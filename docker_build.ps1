$CommitHash = git rev-parse --short HEAD

Write-Host "running docker container" -ForegroundColor Green
docker run --name VirtualReflectionsOS --rm -v "${PWD}:/root/env" -e GIT_COMMIT_HASH=$CommitHash virtual_reflections_os_buildenv