/*
    Copyright 2021 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TinySerial.h"

namespace tinyproto
{

#if defined(ARDUINO)
Serial::Serial(HardwareSerial &dev)
    : m_dev( reinterpret_cast<char *>(&dev) )
{
}
#endif

Serial::Serial(const char *dev)
    : m_dev(dev)
{
}

void Serial::setTimeout(uint32_t timeoutMs)
{
    m_timeoutMs = timeoutMs;
}

bool Serial::begin(uint32_t speed)
{
    m_handle = tiny_serial_open(m_dev, speed);
    return m_handle != TINY_SERIAL_INVALID;
}

void Serial::end()
{
    tiny_serial_close(m_handle);
    m_handle = TINY_SERIAL_INVALID;
}

int Serial::readBytes(uint8_t *buf, int len)
{
    return tiny_serial_read_timeout(m_handle, buf, len, m_timeoutMs);
}

int Serial::write(const uint8_t *buf, int len)
{
    return tiny_serial_send_timeout(m_handle, buf, len, m_timeoutMs);
}

} // namespace tinyproto
