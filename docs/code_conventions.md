**files**: camel_case.ext
**folders**: camel_case
**builds**: PascalCase.ext

**code**:
```c++
struct vfs_t {
    void variable;
};

bool vfs_list_directory(vfs_t* vfs, const std::string& path, std::dynamic_array<vfs_node_t*>* out_array) {
    mutex_lock_guard guard(&vfs->mutex);

    vfs_node_t* root_node = vfs_resolve_path(vfs, path);
    if (!root_node)
        return false;

    out_array->resize(root_node->children.length());
    for (auto& child : root_node->children)
        out_array->insert_back(child.get());

    return true;
}
```
or
```c++
struct TVFS {
    TMutex mutex;
};

bool VFSListDirectory(TVFS* vfs, const std::CString& path, std::CDynamicArray<TVFSNode*>* outArray) {
    CMutexLockGuard guard(&vfs->mutex);

    TVFSNode* rootNode = VFSResolvePath(vfs, path);
    if (!rootNode)
        return false;

    outArray->Resize(rootNode->children.Length());
    for (auto& child : rootNode->children)
        outArray->InsertBack(child.Get());

    return true;
}
```
or
```c++
struct TVFS {
    TMutex m_Mutex;
};

bool VFSListDirectory(TVFS* pVFS, const std::CString& strPath, std::CDynamicArray<TVFSNode*>* pOutArray) {
    CMutexLockGuard Guard(&pVFS->m_Mutex);

    TVFSNode* pRootNode = VFSResolvePath(pVFS, strPath);
    if (!pRootNode)
        return false;

    pOutArray->Resize(pRootNode->m_vecChildren.Length());
    for (auto& Child : pRootNode->m_vecChildren)
        pOutArray->InsertBack(Child.Get());

    return true;
}
```