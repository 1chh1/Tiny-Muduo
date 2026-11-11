#pragma once

#include <iostream>
#include <string>

// 时间类
class Timestamp{
private:
    int64_t m_microSecondsSinceEpoch; // 这是一个类的成员变量，它记录了从 1970年1月1日到现在，总共过去了多少微秒

public:
    Timestamp();

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    static Timestamp now(); // 获取当前时间

    std::string toString() const; // 将时间转为可读格式
};