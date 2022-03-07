//
// Created by 17336 on 2022/2/28.
//

#include <mutex>
#include <iostream>
#include <thread>
#include <math.h>

using namespace std;
void func(){}

int main() {
    float m, n;
    thread t(func);
    cout<<t.joinable();
    t.detach();
    cout<<t.joinable();
}

