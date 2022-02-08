/*
    Copyright 2019-2020,2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#include "tiny_light_helper.h"

uint32_t TinyLightHelper::s_handleOffset;

TinyLightHelper::TinyLightHelper(FakeEndpoint *endpoint, int rx_buf_size)
    : m_handle{}
{
    s_handleOffset = (uint8_t *)this - (uint8_t *)(&m_handle);
    m_endpoint = endpoint;

    tiny_light_init(&m_handle, write_data, read_data, this);
}

int TinyLightHelper::send(uint8_t *buf, int len)
{
    return tiny_light_send(&m_handle, buf, len);
}

int TinyLightHelper::read(uint8_t *buf, int len)
{
    return tiny_light_read(&m_handle, buf, len);
}

int TinyLightHelper::read_data(void *appdata, void *data, int length)
{
    TinyLightHelper *helper = reinterpret_cast<TinyLightHelper *>(appdata);
    return helper->m_endpoint->read((uint8_t *)data, length);
}

int TinyLightHelper::write_data(void *appdata, const void *data, int length)
{
    TinyLightHelper *helper = reinterpret_cast<TinyLightHelper *>(appdata);
    return helper->m_endpoint->write((const uint8_t *)data, length);
}

TinyLightHelper::~TinyLightHelper()
{
    tiny_light_close(&m_handle);
}
