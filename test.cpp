//
// Created by Pluto on 2021/10/4.
//
#include "json.h"
#include<iostream>
//#include<fstream>




int main() {
    std::ifstream in("string");
    auto result=json::parse(in);



    result->write(std::cout,1);

    return 0;
}