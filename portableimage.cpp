#include "portableimage.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <cstdlib>

static unsigned char *writeValue(unsigned char *p, unsigned char whiteSpace, int value)
{
    unsigned int v = value;
    *p++ = whiteSpace;
    bool first = true;
    unsigned int k = 10000;
    while (k > 0) {
        if (v >= k || !first) {
            *p++ = '0' + v / k;
            v %= k;
            first = false;
        }
        k /= 10;
    }
    return p;
}

static const unsigned char *readValue(const unsigned char *p, int &value)
{
    // skip white space
    unsigned char c;
    while (c = *p, true) {
        if (c == ' ' || (c >= '\011' && c <= '\015')) ++p;
        else if (c == '#') do ++p; while (*p != '\n');
        else break;
    }
    int v = 0;
    unsigned int d;
    while (d = *p - '0', d < 10U) {
        ++p;
        v = 10 * v + d;
    }
    value = v;
    return p;
}

unsigned char *PortableImage::writeHeader(unsigned char *p)
{
    *p++ = 'P';

    switch (m_components) {
    case 3:
        *p++ = '6';
        break;
    case 1:
        *p++ = '5';
        break;
    default:
        return p;
    }

    p = writeValue(p, '\n', m_width);
    p = writeValue(p, ' ', m_height);
    p = writeValue(p, '\n', 255);

    *p++ = '\n';
    return p;
}

bool PortableImage::readHeader(const unsigned char *p)
{
    if (*p++ != 'P')
        return false;

    switch (*p++) {
    case '6':
        m_components = 3;
        break;
    case '5':
        m_components = 1;
        break;
    default:
        return false;
    }

    p = readValue(p, m_width);
    p = readValue(p, m_height);
    p = readValue(p, m_maxVal);

    if (m_maxVal != 255)
        return false;

    ++p; // skip single space
    m_data = p;
    return true;
}

PortableImage::PortableImage()
    : m_components(0)
    , m_buffer(MAP_FAILED)
{
}

PortableImage::~PortableImage()
{
#if 1
    if (m_buffer != MAP_FAILED)
        munmap(m_buffer, m_bufferSize);
#else
    free(m_buffer);
#endif
}

bool PortableImage::read(int fd)
{
#if 1
    struct stat sb;
    fstat(fd, &sb);
    m_bufferSize = sb.st_size;
    m_buffer = mmap(0, m_bufferSize, PROT_READ/* | PROT_WRITE*/, MAP_PRIVATE, fd, 0);
    if (m_buffer == MAP_FAILED || !readHeader((unsigned char *) m_buffer)) {
        m_components = 0;
        return false;
    }
    return true;
#else
    struct stat sb;
    fstat(fd, &sb);
    m_bufferSize = sb.st_size;
    m_buffer = malloc(m_bufferSize);
    if (::read(fd, m_buffer, m_bufferSize) != m_bufferSize || !readHeader((unsigned char *) m_buffer)) {
        m_components = 0;
        return false;
    }
    return true;
#endif
}

bool PortableImage::write(int fd)
{
    unsigned char buf[32];
    int size = writeHeader(buf) - buf;
    if (::write(fd, buf, size) != size)
        return false;
    size = m_components * m_width * m_height;
    if (::write(fd, m_data, size) != size)
        return false;
    return true;
}
