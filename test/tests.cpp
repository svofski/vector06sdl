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

int main(int argc, char ** argv)
{
    test_tobcd();
    test_frombcd();
}
