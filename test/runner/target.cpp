/*
 * target.cpp
 *
 *  Created on: 13.06.2016
 *      Author: Klemens
 */

#include <string>

int error{0};


void f(int & ref)
{

}
void f(int * p)
{

}


int f() {return 0;}

int main(int argc, char * argv[])
{
    int value = 0;
    f(value);

    if (value != 42)
        error |= 0b00001;

    int arr[3] = {0,0,0};


    f(static_cast<int*>(arr));

    if (arr[0] != 1) error |= 0b00010;
    if (arr[1] != 2) error |= 0b00100;
    if (arr[2] != 3) error |= 0b01000;

    if (f() != 42)
        error |= 0b10000;

    return error;
}


