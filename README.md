# Tiny-Muduo

本项目是对陈硕老师经典开源网络库 **muduo** 的学习和简单还原，实现了核心功能，项目全程使用C++标准库，弃用boost库。意在学习Linux网络编程底层，打造基于 **one loop per thread** 架构的C++11高并发网络库。

## 核心架构设计

项目采用经典的 **主从Reactor** 模型 : 
- **Main Reactor** : 只负责监听 `listenfd` ，接受新连接 `accept`，并将新连接分发给 Sub Reactor。
- **Sub Reactor** : 每个线程维护一个独立的 `EventLoop` ，即一个独立的 `epoll` ，负责已连接 socket 的所有读写事件、超时管理等