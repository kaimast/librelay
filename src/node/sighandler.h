#include <yael/EventLoop.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <csignal>
#include <cstdio>
#include <cxxabi.h>
#include <execinfo.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <ucontext.h>

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
struct sig_ucontext_t {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
};

void stop_handler(int i) {
    (void)i;
    LOG(INFO) << "Received signal. Stopping...";

    yael::EventLoop::get_instance().stop();
}

void null_handler(int sig) { (void)sig; }

void setup_sighandlers() {
    signal(SIGPIPE, null_handler);
    signal(SIGHUP, null_handler);
    signal(SIGSTOP, stop_handler);
    signal(SIGTERM, stop_handler);
}
