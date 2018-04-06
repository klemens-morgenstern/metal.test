/**
 * @file   test_runner.cpp
 * @date   28.06.2016
 * @author Klemens D. Morgenstern
 *



 */

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/core/lightweight_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>

using namespace std;
namespace bp = boost::process;
namespace fs = boost::filesystem;

int main(int argc, char * argv[])
{
    vector<fs::path> vec(argv + 1, argv+argc);

    for (auto & v : vec)
        cout << v << endl;

    auto itr = find_if(vec.begin(), vec.end(),
                       [](fs::path & p)
                       {
#if defined(BOOST_WINDOWS_API)
                            return p.extension() == ".dll";
#else
                            return p.extension() == ".so";
#endif
                       });

    fs::path dll;

    if (itr != vec.end())
        dll = *itr;
    else
    {
        cout << "No dll found" << endl;
        return 1;
    }

    itr = find_if(vec.begin(), vec.end(),
                       [](fs::path & p)
                       {
#if defined(BOOST_WINDOWS_API)
                            return p.filename() == "metal-dbg-runner.exe";
#else
                            return p.filename() == "mw-dbg-runner";
#endif
                       });
    fs::path exe;

    if (itr != vec.end())
        exe = *itr;
    else
    {
        cout << "No exe found" << endl;
        return 1;
    }

    itr = find_if(vec.begin(), vec.end(),
                       [](fs::path & p)
                       {
#if defined(BOOST_WINDOWS_API)
                            return p.filename() == "target.exe";
#else
                            return p.filename() == "target";
#endif
                       });
    fs::path target;

    if (itr != vec.end())
        target = *itr;
    else
    {
        cout << "No target found" << endl;
        return 1;
    }

    fs::path source;
    itr = find_if(vec.begin(), vec.end(),
                        [](fs::path & p)
                        {
                            return p.filename() == "target.cpp";
                        });
    if (itr != vec.end())
        source = fs::absolute(*itr);
    else
    {
        cout << "Target.cpp not found" << endl;
        return 1;
    }

    cout << "Dll:    " << dll    << endl;
    cout << "Exe:    " << exe    << endl;
    cout << "Target: " << target << endl;
    cout << "Source: " << source << endl;
    BOOST_TEST(boost::filesystem::exists(dll));
    BOOST_TEST(boost::filesystem::exists(exe));
    BOOST_TEST(boost::filesystem::exists(target));

    std::string source_dir = "--source-dir=" + source.parent_path().string();

    {
        cerr << "\n--------------------------- No-Plugin launch    -----------------------------" << endl;
        auto ret = bp::system(exe,  "--exe=" + target.string(), "--debug", "--timeout=5", source_dir);
        cerr << "\n-------------------------------------------------------------------------------\n" << endl;

        BOOST_TEST(ret == 0b11111);
        if (ret != 0b11111)
        {
            std::cerr << "Return value Error [" << ret << " != " << 0b11111 << "]" << std::endl;
        }
    }
    {
        cerr << "---------------------------    Plugin launch    -----------------------------" << endl;
        auto ret = bp::system(exe,  "--exe=" + target.string(), "--debug", "--timeout=5", "--lib=" + dll.string(), source_dir);
        cerr << "\n-------------------------------------------------------------------------------\n" << endl;

        BOOST_TEST(ret == 0);

        if (ret != 0)
        {
            std::cerr << "Return value Error [" << ret << " != " << 0 << "]" << std::endl;
        }

    }

    return boost::report_errors();
}
