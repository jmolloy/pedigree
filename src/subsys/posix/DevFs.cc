#include "DevFs.h"

NullFs NullFs::m_Instance;

uint64_t NullFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<NullFs*>(m_pFilesystem)->read(this, location, size, buffer, bCanBlock);
}
uint64_t NullFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<NullFs*>(m_pFilesystem)->write(this, location, size, buffer, bCanBlock);
}
