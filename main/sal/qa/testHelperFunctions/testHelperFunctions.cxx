/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/




// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sal.hxx"
// This is a test of helperfunctions

#include <osl/time.h>
#include <osl/thread.hxx>

#include "stringhelper.hxx"

#include <testshl/simpleheader.hxx>

// void isJaBloed()
// {
//     t_print("Ist ja echt bloed.\n");
// }

inline sal_Int64 t_abs64(sal_Int64 _nValue)
{
    // std::abs() seems to have some ambiguity problems (so-texas)
    // return abs(_nValue);
    t_print("t_abs64(%ld)\n", _nValue);
    // CPPUNIT_ASSERT(_nValue < 2147483647);

    if (_nValue < 0)
    {
        _nValue = -_nValue;
    }
    return _nValue;
}

void t_print64(sal_Int64 n)
{
    if (n < 0)
    {
        // negativ
        printf("-");
        n = -n;
    }
    if (n > 2147483647)
    {
        sal_Int64 n64 = n >> 32;
        sal_uInt32 n32 = n64 & 0xffffffff;
        printf("0x%.8x ", n32);
        n32 = n & 0xffffffff;
        printf("%.8x (64bit)", n32);
    }
    else
    {
        sal_uInt32 n32 = n & 0xffffffff;
        printf("0x%.8x (32bit) ", n32);
    }
    printf("\n");
}

// -----------------------------------------------------------------------------
namespace testOfHelperFunctions
{
    class test_t_abs64 : public CppUnit::TestFixture
    {
    public:
        void test0();
        void test1_0();
        void test1();
        void test1_1();
        void test2();
        void test3();
        void test4();

        CPPUNIT_TEST_SUITE( test_t_abs64 );
        CPPUNIT_TEST( test0 );
        CPPUNIT_TEST( test1_0 );
        CPPUNIT_TEST( test1 );
        CPPUNIT_TEST( test1_1 );
        CPPUNIT_TEST( test2 );
        CPPUNIT_TEST( test3 );
        CPPUNIT_TEST( test4 );
        CPPUNIT_TEST_SUITE_END( );
    };

    void test_t_abs64::test0()
    {
        // this values has an overrun!
        sal_Int32 n32 = 2147483648;
        t_print("n32 should be -2^31 is: %d\n", n32);
        CPPUNIT_ASSERT_MESSAGE("n32!=2147483648", n32 == -2147483648 );
    }


    void test_t_abs64::test1_0()
    {
        sal_Int64 n;
        n = 1073741824;
        n <<= 9;
        t_print("Value of n is ");
        t_print64(n);
        CPPUNIT_ASSERT_MESSAGE("n=2^30 << 9", t_abs64(n) > 0 );
    }

    void test_t_abs64::test1()
    {
        sal_Int64 n;
        n = 2147483648 << 8;
        t_print("Value of n is ");
        t_print64(n);
        CPPUNIT_ASSERT_MESSAGE("n=2^31 << 8", t_abs64(n) > 0 );
    }
    void test_t_abs64::test1_1()
    {
        sal_Int64 n;
        n = sal_Int64(2147483648) << 8;
        t_print("Value of n is ");
        t_print64(n);
        CPPUNIT_ASSERT_MESSAGE("n=2^31 << 8", t_abs64(n) > 0 );
    }

    void test_t_abs64::test2()
    {
        sal_Int64 n;
        n = 2147483648 << 1;
        t_print("Value of n is ");
        t_print64(n);

        CPPUNIT_ASSERT_MESSAGE("(2147483648 << 1) is != 0", n != 0 );

        sal_Int64 n2 = 2147483648 * 2;
        CPPUNIT_ASSERT_MESSAGE("2147483648 * 2 is != 0", n2 != 0 );

        sal_Int64 n3 = 4294967296LL;
        CPPUNIT_ASSERT_MESSAGE("4294967296 is != 0", n3 != 0 );

        CPPUNIT_ASSERT_MESSAGE("n=2^31 << 1, n2 = 2^31 * 2, n3 = 2^32, all should equal!", n == n2 && n == n3 );
    }


    void test_t_abs64::test3()
    {
        sal_Int64 n = 0;
        CPPUNIT_ASSERT_MESSAGE("n=0", t_abs64(n) == 0 );

        n = 1;
        CPPUNIT_ASSERT_MESSAGE("n=1", t_abs64(n) > 0 );

        n = 2147483647;
        CPPUNIT_ASSERT_MESSAGE("n=2^31 - 1", t_abs64(n) > 0 );

        n = 2147483648;
        CPPUNIT_ASSERT_MESSAGE("n=2^31", t_abs64(n) > 0 );
    }

    void test_t_abs64::test4()
    {
        sal_Int64 n = 0;
        n = -1;
        t_print("Value of n is -1 : ");
        t_print64(n);
        CPPUNIT_ASSERT_MESSAGE("n=-1", t_abs64(n) > 0 );

        n = -2147483648;
        t_print("Value of n is -2^31 : ");
        t_print64(n);
        CPPUNIT_ASSERT_MESSAGE("n=-2^31", t_abs64(n) > 0 );

        n = -8589934592LL;
        t_print("Value of n is -2^33 : ");
        t_print64(n);
        CPPUNIT_ASSERT_MESSAGE("n=-2^33", t_abs64(n) > 0 );
    }


// -----------------------------------------------------------------------------
    class test_t_print : public CppUnit::TestFixture
    {
    public:
        void t_print_001();

        CPPUNIT_TEST_SUITE( test_t_print );
        CPPUNIT_TEST( t_print_001 );
        CPPUNIT_TEST_SUITE_END( );
    };

    void test_t_print::t_print_001( )
    {
        t_print("This is only a test of some helper functions\n");
        sal_Int32 nValue = 12345;
        t_print("a value %d (should be 12345)\n", nValue);

        rtl::OString sValue("foo bar");
        t_print("a String '%s' (should be 'foo bar')\n", sValue.getStr());

        rtl::OUString suValue(rtl::OUString::createFromAscii("a unicode string"));
        sValue <<= suValue;
        t_print("a String '%s'\n", sValue.getStr());
    }


    class StopWatch
    {
        TimeValue m_aStartTime;
        TimeValue m_aEndTime;
        bool m_bStarted;
    public:
        StopWatch()
                :m_bStarted(false)
            {
            }

        void start()
            {
                m_bStarted = true;
                osl_getSystemTime(&m_aStartTime);
            }
        void stop()
            {
                osl_getSystemTime(&m_aEndTime);
                OSL_ENSURE(m_bStarted, "Not Started.");
                m_bStarted = false;
            }
        rtl::OString makeTwoDigits(rtl::OString const& _sStr)
            {
                rtl::OString sBack;
                if (_sStr.getLength() == 0)
                {
                    sBack = "00";
                }
                else
                {
                    if (_sStr.getLength() == 1)
                    {
                        sBack = "0" + _sStr;
                    }
                    else
                    {
                        sBack = _sStr;
                    }
                }
                return sBack;
            }
        rtl::OString makeThreeDigits(rtl::OString const& _sStr)
            {
                rtl::OString sBack;
                if (_sStr.getLength() == 0)
                {
                    sBack = "000";
                }
                else
                {
                    if (_sStr.getLength() == 1)
                    {
                        sBack = "00" + _sStr;
                    }
                    else
                    {
                        if (_sStr.getLength() == 2)
                        {
                            sBack = "0" + _sStr;
                        }
                        else
                        {
                            sBack = _sStr;
                        }
                    }
                }
                return sBack;
            }

        void  showTime(const rtl::OString & aWhatStr)
            {
                OSL_ENSURE(!m_bStarted, "Not Stopped.");

                sal_Int32 nSeconds = m_aEndTime.Seconds - m_aStartTime.Seconds;
                sal_Int32 nNanoSec = sal_Int32(m_aEndTime.Nanosec) - sal_Int32(m_aStartTime.Nanosec);
                // printf("Seconds: %d Nanosec: %d ", nSeconds, nNanoSec);
                if (nNanoSec < 0)
                {
                    nNanoSec = 1000000000 + nNanoSec;
                    nSeconds--;
                    // printf(" NEW Seconds: %d Nanosec: %d\n", nSeconds, nNanoSec);
                }

                rtl::OString aStr = "Time for ";
                aStr += aWhatStr;
                aStr += " ";
                aStr += makeTwoDigits(rtl::OString::valueOf(nSeconds / 3600));
                aStr += ":";
                aStr += makeTwoDigits(rtl::OString::valueOf((nSeconds % 3600) / 60));
                aStr += ":";
                aStr += makeTwoDigits(rtl::OString::valueOf((nSeconds % 60)));
                aStr += ":";
                aStr += makeThreeDigits(rtl::OString::valueOf((nNanoSec % 1000000000) / 1000000));
                aStr += ":";
                aStr += makeThreeDigits(rtl::OString::valueOf((nNanoSec % 1000000) / 1000));
                aStr += ":";
                aStr += makeThreeDigits(rtl::OString::valueOf((nNanoSec % 1000)));

                printf("%s\n", aStr.getStr());
                // cout << aStr.getStr() << endl;
            }

    };

static sal_Bool isEqualTimeValue ( const TimeValue* time1,  const TimeValue* time2)
{
	if( time1->Seconds == time2->Seconds &&
		time1->Nanosec == time2->Nanosec)
		return sal_True;
	else
		return sal_False;
}

static sal_Bool isGreaterTimeValue(  const TimeValue* time1,  const TimeValue* time2)
{
	sal_Bool retval= sal_False;
	if ( time1->Seconds > time2->Seconds)
		retval= sal_True;
	else if ( time1->Seconds == time2->Seconds)
	{
		if( time1->Nanosec > time2->Nanosec)
			retval= sal_True;
	}
	return retval;
}

static sal_Bool isGreaterEqualTimeValue( const TimeValue* time1, const TimeValue* time2)
{
	if( isEqualTimeValue( time1, time2) )
		return sal_True;
	else if( isGreaterTimeValue( time1, time2))
		return sal_True;
	else
		return sal_False;
}

bool isBTimeGreaterATime(TimeValue const& A, TimeValue const& B)
{
	if (B.Seconds > A.Seconds) return true;
	if (B.Nanosec > A.Nanosec) return true;

	// lower or equal
	return false;
}
    // -----------------------------------------------------------------------------


    class test_TimeValues : public CppUnit::TestFixture
    {
    public:

        void t_time1();
        void t_time2();
        void t_time3();

        CPPUNIT_TEST_SUITE( test_TimeValues );
        CPPUNIT_TEST( t_time1 );
        CPPUNIT_TEST( t_time2 );
        CPPUNIT_TEST( t_time3 );
        CPPUNIT_TEST_SUITE_END( );
    };

void test_TimeValues::t_time1()
{
    StopWatch aWatch;
    aWatch.start();
    TimeValue aTimeValue={3,0};
    osl::Thread::wait(aTimeValue);
    aWatch.stop();
    aWatch.showTime("Wait for 3 seconds");
}

void test_TimeValues::t_time2()
{
    t_print("Wait repeats 20 times.\n");
    int i=0;
    while(i++<20)
    {
        StopWatch aWatch;
        aWatch.start();
        TimeValue aTimeValue={0,1000 * 1000 * 500};
        osl::Thread::wait(aTimeValue);
        aWatch.stop();
        aWatch.showTime("wait for 500msec");
    }
}

void test_TimeValues::t_time3()
{
    t_print("Wait repeats 100 times.\n");
    int i=0;
    while(i++<20)
    {
        StopWatch aWatch;
        aWatch.start();
        TimeValue aTimeValue={0,1000*1000*100};
        osl::Thread::wait(aTimeValue);
        aWatch.stop();
        aWatch.showTime("wait for 100msec");
    }
}

    // void demoTimeValue()
    // {
    //     TimeValue aStartTime, aEndTime;
    //     osl_getSystemTime(&aStartTime);
    //     // testSession(xORB, false);
    //     osl_getSystemTime(&aEndTime);
    //
    //     sal_Int32 nSeconds = aEndTime.Seconds - aStartTime.Seconds;
    //     sal_Int32 nNanoSec = aEndTime.Nanosec - aStartTime.Nanosec;
    //     if (nNanoSec < 0)
    //     {
    //         nNanoSec = 1000000000 - nNanoSec;
    //         nSeconds++;
    //     }
    //
    //     // cout << "Time: " << nSeconds << ". " << nNanoSec << endl;
    // }


} // namespace testOfHelperFunctions

// -----------------------------------------------------------------------------
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( testOfHelperFunctions::test_t_print, "helperFunctions" );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( testOfHelperFunctions::test_t_abs64, "helperFunctions" );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( testOfHelperFunctions::test_TimeValues, "helperFunctions" );

// -----------------------------------------------------------------------------
NOADDITIONAL;
