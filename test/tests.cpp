#include <stdio.h>
#include "8253.h"

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

int test_CounterUnit()
{
    TestOfCounterUnit fuc;
    fuc.test_SetMode();
    fuc.test_loadvalue();

    return 1;
}

int main(int argc, char ** argv)
{
    test_tobcd();
    test_frombcd();
    test_CounterUnit();
}
