/*
    Copyright 2019-2020 (C) Alexey Dynda

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

#include "hal/tiny_serial.h"
#include "TinyProtocol.h"
#include <stdio.h>
#include <time.h>
#include <chrono>
#include <thread>

enum class protocol_type_t : uint8_t
{
    HDLC = 0,
    FD = 2,
    LIGHT = 3,
};

static hdlc_crc_t s_crc = HDLC_CRC_8;
static char *s_port = nullptr;
static bool s_generatorEnabled = false;
static bool s_loopbackMode = true;
static protocol_type_t s_protocol = protocol_type_t::FD;
static int s_packetSize = 64;
static int s_windowSize = 7;
static bool s_terminate = false;
static bool s_runTest = false;
static bool s_isArduinoBoard = false;

static int s_receivedBytes = 0;
static int s_sentBytes = 0;

static void print_help()
{
    fprintf(stderr, "Usage: tiny_loopback -p <port> [-c <crc>]\n");
    fprintf(stderr, "Note: communication runs at 115200\n");
    fprintf(stderr, "    -p <port>, --port <port>   com port to use\n");
    fprintf(stderr, "                               COM1, COM2 ...  for Windows\n");
    fprintf(stderr, "                               /dev/ttyS0, /dev/ttyS1 ...  for Linux\n");
    fprintf(stderr, "    -t <proto>, --protocol <proto> type of protocol to use\n");
    fprintf(stderr, "                               fd - full duplex (default)\n");
    fprintf(stderr, "                               light - full duplex\n");
    fprintf(stderr, "    -c <crc>, --crc <crc>      crc type: 0, 8, 16, 32\n");
    fprintf(stderr, "    -g, --generator            turn on packet generating\n");
    fprintf(stderr, "    -s, --size                 packet size: 64 (by default)\n");
    fprintf(stderr, "    -w, --window               window size: 7 (by default)\n");
    fprintf(stderr, "    -r, --run-test             run 15 seconds speed test\n");
    fprintf(stderr, "    -a, --arduino-tty          delay test start by 2 seconds for Arduino ttyUSB interfaces\n");
}

static int parse_args(int argc, char *argv[])
{
    if ( argc < 2 )
    {
        return -1;
    }
    int i = 1;
    while ( i < argc )
    {
        if ( argv[i][0] != '-' )
        {
            break;
        }
        if ( (!strcmp(argv[i], "-p")) || (!strcmp(argv[i], "--port")) )
        {
            if ( ++i < argc )
                s_port = argv[i];
            else
                return -1;
        }
        else if ( (!strcmp(argv[i], "-c")) || (!strcmp(argv[i], "--crc")) )
        {
            if ( ++i >= argc )
                return -1;
            switch ( strtoul(argv[i], nullptr, 10) )
            {
                case 0: s_crc = HDLC_CRC_OFF; break;
                case 8: s_crc = HDLC_CRC_8; break;
                case 16: s_crc = HDLC_CRC_16; break;
                case 32: s_crc = HDLC_CRC_32; break;
                default: fprintf(stderr, "CRC type not supported\n"); return -1;
            }
        }
        else if ( (!strcmp(argv[i], "-s")) || (!strcmp(argv[i], "--size")) )
        {
            if ( ++i >= argc )
                return -1;
            s_packetSize = strtoul(argv[i], nullptr, 10);
            if ( s_packetSize < 32 )
            {
                fprintf(stderr, "Packets size less than 32 bytes are not supported\n");
                return -1;
                return -1;
            }
        }
        else if ( (!strcmp(argv[i], "-w")) || (!strcmp(argv[i], "--window")) )
        {
            if ( ++i >= argc )
                return -1;
            s_windowSize = strtoul(argv[i], nullptr, 10);
            if ( s_windowSize < 1 || s_windowSize > 7 )
            {
                fprintf(stderr, "Allowable window size is between 1 and 7 inclusively\n");
                return -1;
                return -1;
            }
        }
        else if ( (!strcmp(argv[i], "-g")) || (!strcmp(argv[i], "--generator")) )
        {
            s_generatorEnabled = true;
            s_loopbackMode = !s_generatorEnabled;
        }
        else if ( (!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "--run-test")) )
        {
            s_runTest = true;
        }
        else if ( (!strcmp(argv[i], "-a")) || (!strcmp(argv[i], "--arduino-tty")) )
        {
            s_isArduinoBoard = true;
        }
        else if ( (!strcmp(argv[i], "-t")) || (!strcmp(argv[i], "--protocol")) )
        {
            if ( ++i >= argc )
                return -1;
            else if ( !strcmp(argv[i], "fd") )
                s_protocol = protocol_type_t::FD;
            else if ( !strcmp(argv[i], "light") )
                s_protocol = protocol_type_t::LIGHT;
            else
                return -1;
        }
        i++;
    }
    if ( s_port == nullptr )
    {
        return -1;
    }
    return i;
}

//================================== FD ======================================

tiny_serial_handle_t s_serialFd;
tinyproto::FdD *s_protoFd = nullptr;

void onReceiveFrameFd(void *userData, uint8_t addr, tinyproto::IPacket &pkt)
{
    if ( !s_runTest )
        fprintf(stderr, "<<< Frame received payload len=%d\n", (int)pkt.size());
    s_receivedBytes += static_cast<int>(pkt.size());
    if ( !s_generatorEnabled )
    {
        if ( s_protoFd->write(pkt) < 0 )
        {
            fprintf(stderr, "Failed to send packet\n");
        }
    }
}

void onSendFrameFd(void *userData, uint8_t addr, tinyproto::IPacket &pkt)
{
    if ( !s_runTest )
        fprintf(stderr, ">>> Frame sent payload len=%d\n", (int)pkt.size());
    s_sentBytes += static_cast<int>(pkt.size());
}

static int run_fd(tiny_serial_handle_t port)
{
    s_serialFd = port;
    tinyproto::FdD proto(tiny_fd_buffer_size_by_mtu(s_packetSize, s_windowSize));
    proto.enableCrc(s_crc);
    // Set window size to 4 frames. This should be the same value, used by other size
    proto.setWindowSize(s_windowSize);
    // Set send timeout to 1000ms as we are going to use multithread mode
    // With generator mode it is ok to send with timeout from run_fd() function
    // But in loopback mode (!generator), we must resend frames from receiveCallback as soon as possible, use no timeout
    // then
    proto.setSendTimeout(s_generatorEnabled ? 1000 : 0);
    proto.setReceiveCallback(onReceiveFrameFd);
    proto.setSendCallback(onSendFrameFd);
    s_protoFd = &proto;

    proto.begin();
    std::thread rxThread(
        [](tinyproto::FdD &proto) -> void {
            while ( !s_terminate )
            {
                proto.run_rx([](void *u, void *b, int s) -> int { return tiny_serial_read(s_serialFd, b, s); });
            }
        },
        std::ref(proto));
    std::thread txThread(
        [](tinyproto::FdD &proto) -> void {
            while ( !s_terminate )
            {
                proto.run_tx([](void *u, const void *b, int s) -> int { return tiny_serial_send(s_serialFd, b, s); });
            }
        },
        std::ref(proto));

    auto startTs = std::chrono::steady_clock::now();
    auto progressTs = startTs;

    /* Run main cycle forever */
    while ( !s_terminate )
    {
        if ( s_generatorEnabled )
        {
            tinyproto::HeapPacket packet(s_packetSize);
            packet.put("Generated frame. test in progress");
            if ( proto.write(packet.data(), static_cast<int>(packet.size())) < 0 )
            {
                fprintf(stderr, "Failed to send packet\n");
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if ( s_runTest && s_generatorEnabled )
        {
            auto ts = std::chrono::steady_clock::now();
            if ( ts - startTs >= std::chrono::seconds(15) )
                s_terminate = true;
            if ( ts - progressTs >= std::chrono::seconds(1) )
            {
                progressTs = ts;
                fprintf(stderr, ".");
            }
        }
    }
    rxThread.join();
    txThread.join();
    proto.end();
    return 0;
}

//================================== LIGHT ======================================

static int run_light(tiny_serial_handle_t port)
{
    s_serialFd = port;
    tinyproto::Light proto;
    proto.enableCrc(s_crc);

    proto.begin([](void *a, const void *b, int c) -> int { return tiny_serial_send(s_serialFd, b, c); },
                [](void *a, void *b, int c) -> int { return tiny_serial_read(s_serialFd, b, c); });
    std::thread rxThread(
        [](tinyproto::Light &proto) -> void {
            tinyproto::HeapPacket packet(s_packetSize + 4);
            while ( !s_terminate )
            {
                if ( proto.read(packet) > 0 )
                {
                    s_receivedBytes += packet.size();
                    if ( !s_runTest )
                        fprintf(stderr, "<<< Frame received payload len=%d\n", (int)packet.size());
                    if ( !s_generatorEnabled )
                    {
                        if ( proto.write(packet) < 0 )
                        {
                            fprintf(stderr, "Failed to send packet\n");
                        }
                    }
                }
            }
        },
        std::ref(proto));

    auto startTs = std::chrono::steady_clock::now();
    auto progressTs = startTs;

    /* Run main cycle forever */
    while ( !s_terminate )
    {
        if ( s_generatorEnabled )
        {
            tinyproto::HeapPacket packet(s_packetSize);
            packet.put("Generated frame. test in progress");
            if ( proto.write(packet) < 0 )
            {
                fprintf(stderr, "Failed to send packet\n");
            }
            else
            {
                s_sentBytes += static_cast<int>(packet.size());
                if ( !s_runTest )
                    fprintf(stderr, ">>> Frame sent payload len=%d\n", (int)packet.size());
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if ( s_runTest && s_generatorEnabled )
        {
            auto ts = std::chrono::steady_clock::now();
            if ( ts - startTs >= std::chrono::seconds(15) )
                s_terminate = true;
            if ( ts - progressTs >= std::chrono::seconds(1) )
            {
                progressTs = ts;
                fprintf(stderr, ".");
            }
        }
    }
    rxThread.join();
    proto.end();
    return 0;
}

int main(int argc, char *argv[])
{
    if ( parse_args(argc, argv) < 0 )
    {
        print_help();
        return 1;
    }

    tiny_serial_handle_t hPort = tiny_serial_open(s_port, 115200);

    if ( hPort == TINY_SERIAL_INVALID )
    {
        fprintf(stderr, "Error opening serial port\n");
        return 1;
    }
    if ( s_isArduinoBoard )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    int result = -1;
    switch ( s_protocol )
    {
        case protocol_type_t::FD: result = run_fd(hPort); break;
        case protocol_type_t::LIGHT: result = run_light(hPort); break;
        default: fprintf(stderr, "Unknown protocol type"); break;
    }
    tiny_serial_close(hPort);
    if ( s_runTest )
    {
        printf("\nRegistered TX speed: %u bps\n", (s_sentBytes)*8 / 15);
        printf("Registered RX speed: %u bps\n", (s_receivedBytes)*8 / 15);
    }
    return result;
}
