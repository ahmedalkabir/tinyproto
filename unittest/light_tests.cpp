/*
    Copyright 2017-2020,2022 (C) Alexey Dynda

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

#include <functional>
#include <CppUTest/TestHarness.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helpers/tiny_light_helper.h"
#include "helpers/fake_connection.h"

TEST_GROUP(LIGHT){void setup(){
    // ...
}

                  void teardown(){
                      // ...
                  }};

#if 1
TEST(LIGHT, send_receive)
{
    FakeConnection conn;
    conn.endpoint1().setTimeout(100);
    conn.endpoint2().setTimeout(100);
    TinyLightHelper helper1(&conn.endpoint1());
    TinyLightHelper helper2(&conn.endpoint2());
    uint8_t txbuf[128]{};
    uint8_t rxbuf[128]{};

    int nsent = 0;
    while ( nsent < 256 )
    {
        snprintf((char *)txbuf, sizeof(txbuf) - 1, "This is frame Number %u (stream %i)", nsent, 0);
        int sent = helper1.send(txbuf, strlen((char *)txbuf) + 1);
        CHECK_EQUAL(sent, strlen((char *)txbuf) + 1);
        int received = helper2.read(rxbuf, sizeof(rxbuf));
        CHECK_EQUAL(received, strlen((char *)txbuf) + 1);
        nsent++;
    }
}

TEST(LIGHT, small_frames)
{
    FakeConnection conn;
    conn.endpoint1().setTimeout(100);
    conn.endpoint2().setTimeout(100);
    TinyLightHelper helper1(&conn.endpoint1());
    TinyLightHelper helper2(&conn.endpoint2());
    uint8_t txbuf[3]{};
    uint8_t rxbuf[3]{};
    txbuf[0] = 'T';
    int sent = helper1.send(txbuf, 1);
    CHECK_EQUAL(1, sent);
    int received = helper2.read(rxbuf, sizeof(rxbuf));
    CHECK_EQUAL(1, received);
    CHECK_EQUAL('T', rxbuf[0]);
}

#endif
