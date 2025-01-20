Write-Host "building docker image"
docker build . -t virtual_reflections_os_buildenv

Write-Host "running docker container" -ForegroundColor Green
docker run --name VirtualReflectionsOS --rm -it -v "${PWD}:/root/env" virtual_reflections_os_buildenv
