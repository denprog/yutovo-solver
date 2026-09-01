/*
 * Yutovo Solver
 * Copyright (C) 2022-2026 Yutovo developers. All rights reserved.
 * This file is a part of the Yutovo project
 * SPDX-License-Identifier: GPL-3.0-only
 */

//Force-included (/FI) before any other header on Windows.
//The vcpkg sys/time.h shim used by the giac headers defines _WINSOCKAPI_ without
//winsock2, which the boost::asio include guard rejects; the shim also provides its
//own struct timeval, which the 10.0.19041 winsock2.h redefines unconditionally.
//Including the real winsock2.h first resolves both: the shim detects _WINSOCKAPI_
//and skips its timeval block, and asio sees a consistent winsock2 state.
#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif
