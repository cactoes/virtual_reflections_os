## Storage Controller Design (including VFS)
- [x] vfs is storage only
- [ ] supported file systems
    - [ ] fat32
    - [x] iso9660
- [ ] supported storage interfaces
    - [ ] ahci
    - [x] ide
- [x] drive addressing is clear
- [ ] drive addressing is absolute
- [ ] support partitioning
    - [ ] mbr

```
[ vfs ]                -> resolve full path -> "drive/part/test.txt"
  v
[ filesystem manager ] -> handles directory / file structures -> fat32
  v
[ block device ]       -> maps file blocks to disk lba (from its partition) -> block to lba
  v
[ storage controller ] -> execute read/write -> ahci
  v
[ physical disk ]      -> returns bytes
```