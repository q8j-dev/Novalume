/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
*/
#ifndef BOOST_PREDEF_ARCHITECTURE_X86_32_H
#define BOOST_PREDEF_ARCHITECTURE_X86_32_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__)
# undef BOOST_ARCH_X86_32
# if defined(__I86__)
#  define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER(__I86__,0,0)
# elif defined(_M_IX86)
#  define BOOST_ARCH_X86_32 BOOST_PREDEF_MAKE_10_VV00(_M_IX86)
# elif defined(__i686__)
#  define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER(6,0,0)
# elif defined(__i586__)
#  define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER(5,0,0)
# elif defined(__i486__)
#  define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER(4,0,0)
# elif defined(__i386__)
#  define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER(3,0,0)
# else
#  define BOOST_ARCH_X86_32 BOOST_VERSION_NUMBER_AVAILABLE
# endif
#endif
#if BOOST_ARCH_X86_32
# define BOOST_ARCH_X86_32_AVAILABLE
#endif
#define BOOST_ARCH_X86_32_NAME "Intel x86-32"
#include <boost/predef/architecture/x86.h>
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_X86_32,BOOST_ARCH_X86_32_NAME)
