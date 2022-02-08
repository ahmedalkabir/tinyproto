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

#pragma once

#include <functional>
#include <stdint.h>
#include <thread>
#include <atomic>
#include "fake_endpoint.h"

class BaseHelper
{
public:
    BaseHelper(FakeEndpoint *endpoint, int rxBufferSize);
    virtual ~BaseHelper();
    virtual int run_rx() = 0;
    virtual int run_tx() = 0;
    int run(bool forked);
    void stop();

protected:
    FakeEndpoint *m_endpoint;
    std::thread *m_receiveThread = nullptr;
    std::thread *m_sendThread = nullptr;
    std::atomic<bool> m_forceStop;
    uint8_t *m_buffer;
    uint8_t *m_bufferOriginPtr;

    static void receiveThread(BaseHelper *p);
    void wait_until_rx_count(int count, uint32_t timeout);
    static void sendThread(BaseHelper *p);
};

template <typename T> class IBaseHelper: public BaseHelper
{
public:
    using BaseHelper::BaseHelper;

protected:
    static int read_data(void *appdata, void *data, int length)
    {
        T *helper = reinterpret_cast<T *>(appdata);
        IBaseHelper<T> *base = helper;
        return base->m_endpoint->read((uint8_t *)data, length);
    }

    static int write_data(void *appdata, const void *data, int length)
    {
        T *helper = reinterpret_cast<T *>(appdata);
        IBaseHelper<T> *base = helper;
        return base->m_endpoint->write((const uint8_t *)data, length);
    }
};
