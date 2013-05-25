#include "DevFs.h"

RandomFs RandomFs::m_Instance;
NullFs NullFs::m_Instance;

uint64_t RandomFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<RandomFs*>(m_pFilesystem)->read(this, location, size, buffer, bCanBlock);
}
uint64_t RandomFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<RandomFs*>(m_pFilesystem)->write(this, location, size, buffer, bCanBlock);
}

uint64_t NullFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<NullFs*>(m_pFilesystem)->read(this, location, size, buffer, bCanBlock);
}
uint64_t NullFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return reinterpret_cast<NullFs*>(m_pFilesystem)->write(this, location, size, buffer, bCanBlock);
}
