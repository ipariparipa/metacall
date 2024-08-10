/*
    This file is part of metacall.

    sasCore is free software: you can redistribute it and/or modify
    it under the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    sasCore is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with sasCore.  If not, see <http://www.gnu.org/licenses/>
 */

#include <iostream>

#include <metacall/application.h>
#include <metacall/delegate.h>
#include <metacall/timer.h>
#include <functional>
#ifdef __unix__
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * (x))
#endif

struct Struct
{
    std::string str;
    int i = 0;
    float f = 0.0;
};

class A : public METACALL_NS::object
{
    METACALL_NS::delegate::ptr async_delegate;
public:
    virtual ~A() override
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void sync_call_noarg(METACALL_NS::delegate::ptr delegate)
    {
        delegate->invoke<>();
    }

    float sync_call_noarg_ret(METACALL_NS::delegate::ptr delegate)
    {
        return delegate->invoke<float>();
    }

    void sync_call(METACALL_NS::delegate::ptr delegate)
    {
        delegate->invoke<int, std::string>(1, "sdf");
    }

    void async_call(METACALL_NS::delegate::ptr delegate)
    {
        async_delegate = delegate;
    }

    void async_invoke()
    {
        async_delegate->invoke<int, std::string>(2, "sdf");
    }

    float async_invoke_ret()
    {
        return async_delegate->invoke<float, int, std::string>(3, "sdf");
    }

    float sync_call_ret(METACALL_NS::delegate::ptr delegate)
    {
        return delegate->invoke<float, int, std::string>(4, "sdf");
    }

    void src_1()
    {
        app()->emit(METACALL_NS::application::calling_rule::automatic, this, &A::src_1);
    }

    void src_2(int i, const std::string & str)
    {
        app()->emit(METACALL_NS::application::calling_rule::automatic, this, &A::src_2, i, str);
    }

    float src_3(int i, const std::string & str)
    {
        return app()->emit(this, &A::src_3, i, str);
    }

    float src_4(int i)
    {
        return app()->emit(this, METACALL_NS::overload<int>(&A::src_4), i);
    }

    float src_4()
    {
        return app()->emit(this, METACALL_NS::overload<>(&A::src_4));
    }

    void src_5(const Struct & s)
    {
        return app()->emit(this, &A::src_5, s);
    }

    void src_6(const std::shared_ptr<Struct> & s)
    {
        return app()->emit(METACALL_NS::application::calling_rule::queued, this, &A::src_6, s);
    }

    float dest_3(int i, const std::string & str)
    {
        std::cout << "A::" << __FUNCTION__ << " " << i << " " << str << std::endl;
        return 10.3f;
    }

    void dest_5(const Struct & s)
    {
        std::cout << "A::" << __FUNCTION__ << " " << s.str << " " << s.i  << " " << s.f << std::endl;
    }

    void dest_6(const std::shared_ptr<Struct> & s)
    {
        std::cout << "A::" << __FUNCTION__ << " " << s->str << " " << s->i  << " " << s->f << std::endl;
    }
};

class B : public METACALL_NS::object
{
public:
    B()
    { }

    virtual ~B() override
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dest_1()
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dest_2(int i, const std::string & str)
    {
        std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
    }

    float dest_3(int i, const std::string & str)
    {
        std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
        return 30.3f;
    }

    float dest_4()
    {
        std::cout << __FUNCTION__ << std::endl;
        return 10.4f;
    }

    float dest_4(int i)
    {
        std::cout << __FUNCTION__ << " " << i << std::endl;
        return 11.4f;
    }
};

class Timer : public METACALL_NS::timer
{
    const char * m_name;
public:
    template<typename ...ArgT>
    Timer(const char * name, ArgT && ... args)
        : METACALL_NS::timer(std::move(args)...)
        , m_name(name)
    { }

    virtual ~Timer() override
    {
        std::cout << __FUNCTION__ << ": " << m_name << std::endl;
    }
};

int main()
{
    auto app = METACALL_NS::application::get();
    {
        auto a = METACALL_NS::object::make<A>();
        auto b = METACALL_NS::object::make<B>();

    // Delegate
        a->sync_call(METACALL_NS::empty_delegate::make());

        a->sync_call_noarg(METACALL_NS::cpp_delegate::make(b.get(), &B::dest_1));

        std::cout << __FUNCTION__ << " " <<
            a->sync_call_noarg_ret(METACALL_NS::cpp_delegate::make(b.get(), METACALL_NS::overload<>(&B::dest_4)))
        << std::endl;

        a->sync_call(METACALL_NS::cpp_delegate::make(b.get(), &B::dest_2));

        std::cout << __FUNCTION__ << " " <<
            a->sync_call_ret(METACALL_NS::cpp_delegate::make(b.get(), &B::dest_3))
        << std::endl;

        a->async_call(METACALL_NS::cpp_delegate::make(b.get(), &B::dest_2));
        a->async_invoke();

        a->async_call(METACALL_NS::cpp_delegate::make(b.get(), &B::dest_3));
        std::cout << __FUNCTION__ << " " <<
            a->async_invoke_ret()
        << std::endl;

        a->sync_call(METACALL_NS::cpp_delegate::make(b.get(), std::function([](int i, const std::string & str){
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
        })));

        a->async_call(METACALL_NS::cpp_delegate::make(b.get(), std::function([](int i, const std::string & str){
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
        })));
        a->async_invoke();

        std::cout << __FUNCTION__ << " " <<
            a->sync_call_ret(METACALL_NS::cpp_delegate::make(b.get(), std::function([](int i, const std::string & str)->float{
                std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
                return 12.1;
            })))
        << std::endl;

        a->async_call(METACALL_NS::cpp_delegate::make(b.get(), std::function([](int i, const std::string & str){
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
        })));
        a->async_invoke();

        a->async_call(METACALL_NS::cpp_delegate::make(b.get(), std::function([](int i, const std::string & str)->float{
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
            return 45.1;
        })));
        std::cout << __FUNCTION__ << " " <<
            a->async_invoke_ret()
        << std::endl;

    // Delegate + thread
        {
             auto th_a = METACALL_NS::object::make<A>();
             std::thread th([th_a](){
                 th_a->move_to_thread();
                 sleep(3);
                 th_a->async_invoke();
                 th_a->app()->exit(0);
             });

             std::cout << "main thread: " << std::this_thread::get_id() << std::endl;
             th_a->async_call(METACALL_NS::cpp_delegate::make(b.get(), std::function([](int i, const std::string & str){
                 std::cout << __FUNCTION__ << " thread: " << std::this_thread::get_id() << std::endl;
                 std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
             })));

             app->exec();
             th.join();
             th_a->move_to_thread();
        }

        app->final();

    // connect
        METACALL_NS::connect(a.get(), &A::src_1, b.get(), &B::dest_1);
        METACALL_NS::connect(a.get(), &A::src_2, b.get(), &B::dest_2);
        METACALL_NS::connect(a.get(), &A::src_2, b.get(), &B::dest_3);
        METACALL_NS::connect(a.get(), &A::src_3, b.get(), &B::dest_3);
        METACALL_NS::connect(a.get(), &A::src_3, a.get(), &A::dest_3);
        METACALL_NS::connect(a.get(), METACALL_NS::overload<int>(&A::src_4), b.get(), METACALL_NS::overload<int>(&B::dest_4));
        METACALL_NS::connect(a.get(), METACALL_NS::overload<>(&A::src_4), b.get(), METACALL_NS::overload<>(&B::dest_4));

        METACALL_NS::connect(a.get(), &A::src_3, b.get(), [b = b.get()](int i, const std::string & str){ return b->dest_3(i, str); });
        METACALL_NS::connect(a.get(), &A::src_3, b.get(), [](int i, const std::string & str) {
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
            return 23.4f;
        });

        METACALL_NS::connect(a.get(), &A::src_2, [](int i, const std::string & str) {
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
        });

        METACALL_NS::connect(a.get(), &A::src_3, [](int i, const std::string & str) {
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
        });

        METACALL_NS::connect(a.get(), &A::src_2, a.get(), &A::src_3);
        METACALL_NS::connect(a.get(), &A::src_5, a.get(), &A::dest_5);
        METACALL_NS::connect(a.get(), &A::src_6, a.get(), &A::dest_6);

        a->src_1();
        a->src_2(42, "asd");
        auto ret = a->src_3(42, "asd");
        std::cout << "ret:" << ret << std::endl;

        auto ret_2 = a->src_4(20);
        std::cout << "ret_2:" << ret_2 << std::endl;

        auto ret_3 = a->src_4();
        std::cout << "ret_3:" << ret_3 << std::endl;

        a->src_5({ "asd", 32, 45.6 });

        a->src_6(std::make_shared<Struct>(Struct{ "sdf", 42, 56.7 }));
        app->run();

        app->call(METACALL_NS::application::calling_rule::direct, nullptr, b.get(), &B::dest_1);

        app->call(METACALL_NS::application::calling_rule::priority, nullptr, b.get(), &B::dest_2, 10, "sdf");

        app->call(METACALL_NS::application::calling_rule::queued, nullptr, b.get(), &B::dest_1);

        app->call(METACALL_NS::application::calling_rule::queued, nullptr, a.get(), std::function([a = a.get()](){a->dest_3(12, "ert");}));

        app->run();

    }

    app->run();

    {
    // timers
        auto t1 = METACALL_NS::object::make<Timer>("t1", app);
        auto t2 = METACALL_NS::object::make<Timer>("t2", app);

        METACALL_NS::connect(t1.get(), &METACALL_NS::timer::trigger, t1.get(), [](){
            std::cout << "trigger: t1" << std::endl;
        });
        METACALL_NS::connect(t1.get(), &METACALL_NS::timer::stopped, t2.get(),[_t2 = t2.get()](){
            if(!_t2->active())
                _t2->app()->exit(0);
        });

        METACALL_NS::connect(t2.get(), &METACALL_NS::timer::trigger, t2.get(), [](){
            std::cout << "trigger: t2" << std::endl;
        });
        METACALL_NS::connect(t2.get(), &METACALL_NS::timer::stopped, t1.get(), [_t1 = t1.get()](){
            if(!_t1->active())
                _t1->app()->exit(0);
        });

        t1->start(std::chrono::milliseconds(300), 5);
        t2->start(std::chrono::milliseconds(500), 3);

        app->exec();
    }
    app->final();

    {
    // timer + thread
        auto t = METACALL_NS::object::make<Timer>("t");

        METACALL_NS::application::get()->connect(t.get(), &METACALL_NS::timer::trigger, [] () {
            std::cout << "Timer: trigger" << std::endl;
        });

        METACALL_NS::application::get()->connect(t.get(), &METACALL_NS::timer::stopped, [] () {
            std::cout << "Timer: stopped" << std::endl;
        });

        t->start(std::chrono::milliseconds(700), 3);

        std::cout << "main thread: " << std::this_thread::get_id() << std::endl;

        auto th_a = METACALL_NS::object::make<A>();

        METACALL_NS::connect(th_a.get(), &A::src_1, METACALL_NS::application::get().get(), [](){
            std::cout << __FUNCTION__ << " thread: " << std::this_thread::get_id() << std::endl;
        });

        std::thread th([th_a](){
            th_a->move_to_thread();
            th_a->src_1();
            sleep(2);
            th_a->src_1();
            sleep(2);
            th_a->src_1();
            sleep(1);
            th_a->app()->exit(100);
        });

        app->exec();
        th.join();
    }
    app->final();

    {
    //all-in-one
        auto t = METACALL_NS::object::make<Timer>("t");
        auto a = METACALL_NS::object::make<A>();
        auto b = METACALL_NS::object::make<B>();

        METACALL_NS::connect(a.get(), &A::src_1, b.get(), &B::dest_1);
        METACALL_NS::connect(t.get(), &Timer::trigger, a.get(), &A::src_1);

        t->start(std::chrono::milliseconds(750));

        auto th_a = METACALL_NS::object::make<A>();
        std::thread th([th_a](){
            th_a->move_to_thread();
            sleep(3);
            th_a->async_invoke();
        });

        std::cout << "main thread: " << std::this_thread::get_id() << std::endl;
        th_a->async_call(METACALL_NS::cpp_delegate::make(b.get(), std::function([&](int i, const std::string & str){
            std::cout << __FUNCTION__ << " thread: " << std::this_thread::get_id() << std::endl;
            std::cout << __FUNCTION__ << " " << i << " " << str << std::endl;
            t->stop();
            th_a->app()->exit(0);
        })));

        app->exec();
        th.join();
        th_a->move_to_thread();
    }

    return app->final();
}
