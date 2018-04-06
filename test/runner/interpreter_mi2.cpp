/**
 * @file   /gdb-runner/test/interpreter.cpp
 * @date   21.02.2017
 * @author Klemens D. Morgenstern
 *



 */

#include <boost/optional/optional_io.hpp>
#include <metal/gdb/mi2/interpreter.hpp>
#include <boost/variant/get.hpp>
#include <boost/process.hpp>

#define BOOST_TEST_MODULE interpreter_mi2_test

#include <boost/test/included/unit_test.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <sstream>
#include <typeinfo>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/core/demangle.hpp>

#include <boost/algorithm/string/predicate.hpp>

namespace bp = boost::process;
namespace fs = boost::filesystem;
namespace mi2 = metal::gdb::mi2;

struct MyProcess
{

    char* target()
    {
        using boost::unit_test::framework::master_test_suite;

        BOOST_ASSERT(master_test_suite().argc > 1);
        return master_test_suite().argv[1];
    }

    boost::asio::io_service ios;
    bp::async_pipe out{ios};
    bp::async_pipe in {ios};
    bp::async_pipe err{ios};
    fs::path gdb_path = bp::search_path("gdb");
    bp::child ch{gdb_path, bp::args={target(), "--interpreter=mi2"}, ios, bp::std_in < in, bp::std_out > out, bp::std_err > err};

    //std::thread thr{[this]{ios.run();}};
    std::atomic<bool> done{false};
    std::stringstream ss_err;

    bool first_run = true;
    static MyProcess * proc;

    MyProcess()
    {
        BOOST_ASSERT(ch.running());
        proc = this;
        std::cout << "starting process\n";
    }
    ~MyProcess()
    {
        std::cout << "terminating process\n";
        if (ch.running())
            ch.terminate();
        ch.wait();
        //thr.join();
    }

    template<typename T>
    void run(T & func)
    {
        BOOST_REQUIRE_MESSAGE(fs::exists(gdb_path), "Gdb exists " << gdb_path);
        BOOST_REQUIRE(ch.running());
        boost::asio::spawn(ios,
                [&](boost::asio::yield_context yield_)
                {
                    BOOST_TEST_PASSPOINT();
                    metal::gdb::mi2::interpreter intp{out, in, yield_, ss_err};
                    BOOST_TEST_PASSPOINT();

                    if (first_run == true)
                    {
                        BOOST_TEST_PASSPOINT();
                        intp.read_header();
                        BOOST_TEST_PASSPOINT();

                        first_run = false;
                    }

                    func(intp);
                    done.store(true);
                    BOOST_TEST_MESSAGE("Internal log of interpreter \n---------------------------------------------------------------------------\n"
                                       + ss_err.str() +
                                       "\n---------------------------------------------------------------------------\n");
                    ss_err.clear();
                });
        while (!done.load())
            ios.poll_one();

        done.store(false);
    }

};

MyProcess * MyProcess::proc = nullptr;

#define METAL_TEST_CASE(...) \
void BOOST_PP_CAT(metal_test_,  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) ) (metal::gdb::mi2::interpreter & mi); \
BOOST_AUTO_TEST_CASE (  __VA_ARGS__) \
{ \
    BOOST_REQUIRE(MyProcess::proc != nullptr); \
    MyProcess::proc->run( BOOST_PP_CAT(metal_test_,  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) ) ) ; \
} \
void BOOST_PP_CAT(metal_test_,  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) ) (metal::gdb::mi2::interpreter & mi)

BOOST_GLOBAL_FIXTURE(MyProcess);

METAL_TEST_CASE( create_bp )
{
    BOOST_TEST_PASSPOINT();
    BOOST_CHECK_THROW(mi.break_after(42, 2), metal::gdb::mi2::exception);
    BOOST_TEST_PASSPOINT();

    try {
        mi2::linespec_location ll;
        ll.linenum  = 34;
        ll.filename = "target.cpp";

        auto bp1 = mi.break_insert(ll);
        BOOST_CHECK(true);
        BOOST_TEST_PASSPOINT();
        BOOST_REQUIRE_EQUAL(bp1.size(), 1u);

        mi2::linespec_location ex;
        ex.function = "f";

        decltype(bp1) bp2;
        BOOST_REQUIRE_NO_THROW(bp2 = mi.break_insert(ex));
        BOOST_TEST_PASSPOINT();

        BOOST_REQUIRE_GE(bp2.size(), 1u);

        auto bp = bp1.front();

        BOOST_CHECK_NO_THROW(mi.break_condition(2, "42"));
        BOOST_CHECK_NO_THROW(mi.break_after(bp.number, 2));
        BOOST_CHECK_NO_THROW(mi.break_disable(bp.number));
        BOOST_CHECK_NO_THROW(BOOST_CHECK_EQUAL(mi.break_info(bp.number).number, bp.number));
        BOOST_CHECK_NO_THROW(mi.break_enable(bp.number));
        BOOST_CHECK_NO_THROW(mi.break_disable({bp.number})); //so we have the vector version tested.

        BOOST_CHECK_NO_THROW(mi.break_commands(bp.number, {"print argc"}));

        BOOST_REQUIRE_GE(bp2.size(), 1u);

        BOOST_CHECK_NO_THROW(mi.break_delete(bp.number));

        BOOST_CHECK_NO_THROW(mi.break_delete({bp2.front().number}));

        try
        {
            mi.exec_run();
        }
        catch (std::exception & e)
        {
            BOOST_CHECK_MESSAGE(false, e.what());
            std::terminate();
        }


        mi2::async_result ar;
        BOOST_CHECK_NO_THROW(ar = mi.wait_for_stop());

        BOOST_CHECK_EQUAL(ar.reason, "exited");

        BOOST_CHECK_NO_THROW(mi.gdb_exit());

       // BOOST_CHECK_THROW(mi.gdb_exit(), boost::system::system_error);
    }
    catch (std::exception & e)
    {
        std::cerr << "Exception '" << e.what() << std::endl;
        std::cerr << "Ex-Type: '" << boost::core::demangle(typeid(e).name()) << "'" << std::endl;
        BOOST_CHECK(false);
    }
}

