#pragma once

#include <iostream>
#include <string>

class Timestamp{
private:
    int64_t m_microSecondsSinceEpoch; // 这是一个类的成员变量，它记录了从 1970年1月1日到现在，总共过去了多少微秒

public:
    Timestamp();

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    static Timestamp now();

    std::string toString() const;
};