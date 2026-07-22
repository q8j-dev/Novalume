/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
*/
#ifndef BOOST_PREDEF_ARCHITECTURE_X86_64_H
#define BOOST_PREDEF_ARCHITECTURE_X86_64_H
#include <boost/predef/version_number.h>
#define BOOST_ARCH_X86_64 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
# undef BOOST_ARCH_X86_64
# define BOOST_ARCH_X86_64 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_ARCH_X86_64
# define BOOST_ARCH_X86_64_AVAILABLE
#endif
#define BOOST_ARCH_X86_64_NAME "Intel x86-64"
#include <boost/predef/architecture/x86.h>
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_X86_64,BOOST_ARCH_X86_64_NAME)
