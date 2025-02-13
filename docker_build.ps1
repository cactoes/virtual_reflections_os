Write-Host "running docker container" -ForegroundColor Green
docker run --name VirtualReflectionsOS --rm -v "${PWD}:/root/env" virtual_reflections_os_buildenv