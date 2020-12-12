#include <stdio.h>
#include "8253.h"
#include "SDL.h"
#include "fsimage.h"
#include "util.h"
#include <iostream>

using namespace std;

class Test {
private:
    const char * title;
    int errors;
public:
    Test(const char * _title) : title(_title), errors(0)
    {
        printf("\033[0;35mENTER TEST: %s\033[0m\n", _title);
    }

    ~Test()
    {
        printf("EXIT TEST: %s ", this->title);
        if (this->errors) {
            printf("\033[41;97m %d ERRORS \033[0m\n", this->errors);
        } else {
            printf("\033[46;30m PASS \033[0m\n");
        }
    }

    Test & operator()(const bool& result) {
        if (!result) ++this->errors;
        return *this;
    }

    int operator()(void) {
        return this->errors;
    }
};

bool compare_u16x(uint16_t exp, uint16_t act, const char * msg)
{
    printf("\t%s: ", msg);
    if (exp == act) {
        printf("pass\n");
        return true;
    }
    printf("\033[41;97m ERROR: \033[0m EXP=0x%04x ACT=0x%04x\n", exp, act);
    return false;
}

bool compare_u16d(uint16_t exp, uint16_t act, const char * msg)
{
    printf("\t%s: ", msg);
    if (exp == act) {
        printf("pass\n");
        return true;
    }
    printf("\033[41;97m ERROR: \033[0m EXP=%04d ACT=%04d\n", exp, act);
    return false;
}

bool compare_bool(bool exp, bool act, const char * msg)
{
    printf("\t%s: ", msg);
    if (exp == act) {
        printf("pass\n");
        return true;
    }
    printf("\033[41;97m ERROR: \033[0m EXP=%s ACT=%s\n", 
            exp ? "true" : "false", act ? "true" : "false");
    return false;
}

bool compare_str(string exp, string act, const char * msg)
{
    printf("\t%s: ", msg);
    if (exp == act) {
        printf("pass\n");
        return true;
    }
    printf("\033[41;97m ERROR: \033[0m EXP=\"%s\" ACT=\"%s\"\n", 
            exp.c_str(), act.c_str());
    return false;
}


int test_tobcd() 
{
    return Test("test_tobcd()")
    (compare_u16x(0, CounterUnit::tobcd(0), "tobcd(0)"))
    (compare_u16x(0, CounterUnit::tobcd(10000), "tobcd(10000)"))
    (compare_u16x(1, CounterUnit::tobcd(1), "tobcd(1)"))
    (compare_u16x(0x1234, CounterUnit::tobcd(1234), "tobcd(1234)")) 
    (compare_u16x(0x9876, CounterUnit::tobcd(9876), "tobcd(9875)"))
    ();
}

int test_frombcd()
{
    Test t("test_frombcd()");
    return compare_u16d(0, CounterUnit::frombcd(0x0), "frombcd(0)")
        && compare_u16d(9999, CounterUnit::frombcd(0x9999), "frombcd(0x9999)")
        && compare_u16d(1234, CounterUnit::frombcd(0x1234), "frombcd(0x1234)")
        && compare_u16d(9876, CounterUnit::frombcd(0x9876), "frombcd(0x9876)")
        && compare_u16d(9999, CounterUnit::frombcd(0xffff), "frombcd(0xffff)");
}

class TestOfCounterUnit {
public:
    bool test_SetMode()
    {
        Test t("CounterUnit::SetMode()");

        bool res = true;
        CounterUnit cu;
        cu.SetMode(0, 0, 0); 
        res &= compare_u16x(0, cu.mode_int, "set mode 0, mode_int=0");
        res &= compare_u16x(0, cu.out, "out=0");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(1, 0, 0);
        res &= compare_u16x(1, cu.mode_int, "set mode 1, mode_int=1");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(2, 0, 0);
        res &= compare_u16x(2, cu.mode_int, "set mode 2, mode_int=2");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(3, 0, 0);
        res &= compare_u16x(3, cu.mode_int, "set mode 3, mode_int=3");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(4, 0, 0);
        res &= compare_u16x(4, cu.mode_int, "set mode 4, mode_int=4");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(5, 0, 0);
        res &= compare_u16x(5, cu.mode_int, "set mode 5, mode_int=5");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(6, 0, 0);
        res &= compare_u16x(2, cu.mode_int, "set mode 6, mode_int=2");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        cu.SetMode(7, 0, 0);
        res &= compare_u16x(3, cu.mode_int, "set mode 7, mode_int=3");
        res &= compare_u16x(1, cu.out, "out=1");
        res &= compare_bool(true, cu.armed, "armed");
        res &= compare_bool(false, cu.enabled, "enabled");

        return res;
    }

    bool test_loadvalue16(int mode, int lsb, int msb, int counts, int expect)
    {
        char msg[128];
        Test t("CounterUnit load value mode 1 LSB-MSB");

        bool res = true;

        CounterUnit cu;
        cu.SetMode(mode, 3, 0);    // latch mode 3: read/load LSB, then MSB
        cu.write_value(lsb);
        sprintf(msg, "load mode %d lsb write value", mode);
        res &= compare_u16x(0, cu.value, msg);

        cu.write_value(msb);
        sprintf(msg, "load mode %d msb write value", mode);
        res &= compare_u16x(0, cu.value, msg);
        
        for (int i = 0; i < counts; ++i) cu.Count(1);

        sprintf(msg, "load mode %d lsb-msb write value", mode);
        res &= compare_u16x(expect, cu.value, msg);

        return res;
    }

    bool test_loadvalue()
    {
        return test_loadvalue16(0, 0x34, 0x12, 4, 0x1233) &&
            test_loadvalue16(1, 0x34, 0x12, 4, 0x1234) &&
            test_loadvalue16(2, 0x34, 0x12, 4, 0x1233) &&
            test_loadvalue16(3, 0x34, 0x12, 4, 0x1232);
    }
};

struct UtilTest
{
    bool test_split_path()
    {
        Test t("util::split_path");
        {
        auto [path, name, ext] = util::split_path("/path/to/stem.ext");
        compare_str("/path/to/", path, "path");
        compare_str("stem", name, "name");
        compare_str(".ext", ext, "ext");
        }

        {
        auto [path, name, ext] = util::split_path("/path/to/.ext");
        printf("path: [%s] name: [%s] ext: [%s]\n",
                path.c_str(), name.c_str(), ext.c_str());
        compare_str("/path/to/", path, "path");
        compare_str("", name, "name");
        compare_str(".ext", ext, "ext");
        }

        {
        auto [path, name, ext] = util::split_path(".ext");
        printf("path: [%s] name: [%s] ext: [%s]\n",
                path.c_str(), name.c_str(), ext.c_str());
        compare_str("", path, "path");
        compare_str("", name, "name");
        compare_str(".ext", ext, "ext");
        }

        {
        auto [path, name, ext] = util::split_path("stem");
        printf("path: [%s] name: [%s] ext: [%s]\n",
                path.c_str(), name.c_str(), ext.c_str());
        compare_str("", path, "path");
        compare_str("stem", name, "name");
        compare_str("", ext, "ext");
        }

        {
        auto [path, name, ext] = util::split_path("/path/to/");
        printf("path: [%s] name: [%s] ext: [%s]\n",
                path.c_str(), name.c_str(), ext.c_str());
        compare_str("/path/to/", path, "path");
        compare_str("", name, "name");
        compare_str("", ext, "ext");
        }

        return true;
    }

    bool test()
    {
        bool res = test_split_path();
        return res;
    }
};

struct FilesystemTest
{
    bool test1()
    {
        vector<uint8_t> dummy(MDHeader::SIZE);
        MDHeader hello(&dummy[0]);
        hello.init_with_filename("hello.com");
        string name = string(hello.fields->Name, 8);
        string ext = string(hello.fields->Ext, 3);
        compare_str("HELLO   ", name, "hello.fields.Name");
        compare_str("COM", ext, "hello.fields.Ext");

        return true;
    }

    bool test2()
    {
        vector<uint8_t> data{0, 
            'H','E','L','L','O',32,32,32, 
            'J','P','G',
            0, 0, 0, 0, 
            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
        MDHeader hello(&data[0]);

        string name = string(hello.fields->Name, 8);
        string ext = string(hello.fields->Ext, 3);
        compare_str("HELLO   ", name, "hello.fields.Name");
        compare_str("JPG", ext, "hello.fields.Ext");

        return true;
    }

    vector<uint8_t> load_image()
    {
        vector<uint8_t> image;
        for (auto pth : {"../testroms/test.fdd", "testroms/test.fdd"}) {
            image = util::load_binfile(pth);
            if (image.size() > 0) {
                printf("Loaded %s\n", pth);
                break;
            }
        }
        if (image.size() == 0) {
            compare_bool(true, false, "test.fdd not available");
        }
        return image;
    }

    bool test3()
    {
        auto image = load_image();
        FilesystemImage fs(image);
        for (auto it = fs.begin(); it != fs.end(); ++it) {
            if ((*it).user() < 0x10 && (*it).fields->Extent == 0) {
                string user;
                user += '0' + (*it).user();
                cout << user << " " <<
                    (*it).name() << "." <<
                    (*it).ext() << "\t";

                Dirent de = fs.load_dirent(*it);
                cout << de.size << endl;
                auto contents = fs.read_bytes(de);
                //for (int i = 0; i < contents.size(); ++i) {
                //    printf("%02x ", contents[i]);
                //}
                //printf("\n");
            }
        }

        return true;
    }

    bool test4()
    {
        std::vector<uint8_t> glob(100*1024);
        for (int i = 0; i < glob.size(); ++i) {
            glob[i] = 255 & rand();
        }

        auto image = load_image();
        FilesystemImage fs(image);
        fs.save_file("glob.dat", glob);

        fs.listdir([](const Dirent & d) {
            printf("%2d: %s.%s\t%d\n", d.user(), d.name().c_str(), 
                    d.ext().c_str(), d.size);
                });

        auto readbakc = fs.read_file("glob.dat");
        for (int i = 0; i < readbakc.size(); ++i) {
            if (glob[i] != readbakc[i]) {
                compare_bool(true, false, "content mismatch");
                return false;
            }
        }

        return true;
    }

    bool test5()
    {
        FilesystemImage fs(840*1024);
        fs.mount_local_dir("testroms");
        fs.listdir([](const Dirent & d) {
            printf("%2d: %s.%s\t%d\n", d.user(), d.name().c_str(), 
                    d.ext().c_str(), d.size);
                });
    }

    bool test()
    {
        return
            test1() &
            test2() &
            test3() & 
            test4() &
            test5();
    }
};

int test_CounterUnit()
{
    TestOfCounterUnit fuc;
    fuc.test_SetMode();
    fuc.test_loadvalue();

    return 1;
}

int main(int argc, char ** argv)
{
    std::ios::sync_with_stdio(true);

    test_tobcd();
    test_frombcd();
    test_CounterUnit();
    UtilTest().test();
    FilesystemTest().test();
}
