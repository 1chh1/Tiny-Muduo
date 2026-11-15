#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// 防止一个线程创建多个 EventLoop
// __thread 保证此变量在每一个线程都有一份独立的拷贝副本，不会冲突
__thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认的 Poller I/O 复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建 wakeupfd ，用来唤醒 subReactor 处理新来的 Channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error : %d", errno);
    }
    return evtfd;
}

EventLoop::EventLoop():m_looping(false), 
                       m_quit(false), 
                       m_callingPendingFunctors(false), 
                       m_threadid(CurrentThread::tid()), 
                       m_poller(Poller::newDefaultPoller(this)), 
                       m_wakeupfd(createEventfd()), 
                       m_wakeupChannel(new Channel(this, m_wakeupfd))
{
    LOG_DEBUG("EventLoop created %p in thread %d", this, m_threadid);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, m_threadid);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置 wakeupfd 的事件类型以及发生事件后的回调操作
    m_wakeupChannel->setReadCallBack(std::bind(&EventLoop::handleRead, this));
    // 每一个 EventLoop 都将监听 wakeupChannel 的 EPOLLIN 读事件
    m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    ::close(m_wakeupfd);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(m_wakeupfd, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

// 开启事件循环
void EventLoop::loop()
{
    m_looping = true;
    m_quit = false;

    LOG_INFO("EventLoop %p start looping", this);

    while(!m_quit)
    {
        m_activeChannels.clear();
        // 监听两类 fd ，一种是 client 的 fd ，一种是 wakeupfd
        m_pollReturnTime = m_poller->poll(kPollTimeMs, &m_activeChannels);
        for(Channel* channel : m_activeChannels)
        {
            // Poller 监听哪些 Channel 发生事件了，然后上报给 EventLoop ，通知 Channel 处理相应的事件
            channel->handleEvent(m_pollReturnTime);
        }

        // 执行当前 EventLoop 事件循环需要处理的回调操作
        /*
        I/O 线程，mainLoop 使用 accept 监听连接，当有新的连接连上来后，accept 返回一个 fd ，然后把这个 fd 封装成 Channel ，分配给某个 subLoop
        mainLoop 实现注册一个回调 cb ，需要 subLoop 来执行，wakeup subLoop 后，执行下面的方法，执行之前 mainLoop 注册的 cb 操作
        */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping", this);
    m_looping = false;
}

// 退出事件循环
void EventLoop::quit()
{
    m_quit = true;

    if(!isInLoopThread()) // 如果是在其他线程中，调用的 quit ，比如在一个 subLoop 中，调用了 mainLoop 的 quit
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()) // 在当前 loop 线程中执行 cb
    {
        cb();
    }
    else // 在非当前 loop 线程中执行 cb ，就需要唤醒 loop 所在线程，执行 cb
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pendingFunctors.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面回调操作的 loop 的线程
    // || m_callingPendingFunctors 意思是当前 loop 正在执行回调，但是 loop 又有了新的回调，
    if(!isInLoopThread() || m_callingPendingFunctors)
    {
        wakeup(); // 唤醒 loop 所在线程
    }
}

// 用来唤醒 loop 所在的线程的，向 wakeupfd 写一个数据
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(m_wakeupfd, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return m_poller->hasChannel(channel);
}

void EventLoop::doPendingFunctors() // 执行回调
{
    std::vector<Functor> functors;
    m_callingPendingFunctors = true;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        functors.swap(m_pendingFunctors);
    }

    for(const Functor& functor : functors)
    {
        functor(); // 执行当前 loop 需要执行的回调操作
    }

    m_callingPendingFunctors = false;
}