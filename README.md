TNeoKernel: a real-time kernel
==============

A real-time kernel based on [TNKernel](http://www.tnkernel.com/ "TNKernel"), currently for PIC32 only, but can be ported to other architectures easily. Tested on PIC32MX.

Note: the project is in BETA stage for now. This means that API still may change. I have plans to get it out of BETA until december, 2014.

--------------

#Overview

  * Fixed a lot of bugs of original TNKernel, see below (especially with mutexes and associated priority algorithms)
  * Tested by detailed unit tests (I'm still working on it, not all tests are done yet, it's matter of time)
  * Separate stack for interrupts
  * Nested interrupts are supported
  * Shadow register set interrupts are supported
  * Recursive mutexes (optionally)
  * No timer task, so, system tick processing works much faster and takes less RAM

This project was initially a fork of PIC32 TNKernel port by Anders Montonen: [TNKernel-PIC32](https://github.com/andersm/TNKernel-PIC32 "TNKernel-PIC32"). I don't like several design decisions of original TNKernel, as well as **many** of the implementation details, but Anders wants to keep his port as close to original TNKernel as possible. So I decided to fork it and have fun implementing what I want.

The more I get into how TNKernel works, the less I like its code. There is a lot of code duplication and a lot of inconsistency, all of this leads to bugs. So I decided to rewrite it almost completely, and that's what I'm doing now.

I decided not to care much about compatibility with original TNKernel API because I really don't like several API decisions, so, I actually had to choose new name for this project, in order to avoid confusion, hence "TNeoKernel". You can read about differences from original TNKernel at the wiki page: [Differences from original TNKernel](/dfrank/tneokernel/wiki/diff_orig_tnkernel)

Together with almost totally re-writting TNKernel, I'm implementing detailed unit tests for it, to make sure I didn't break anything, and of course I've found several bugs in original TNKernel. Several of them:

  * If low-priority `task_low` locks mutex M1 with priority inheritance, high-priority `task_high` tries to lock mutex M1 and gets blocked -> `task_low`'s priority elevates to `task_high`'s priority; then `task_high` stops waiting for mutex by timeout -> priority of `task_low` remains elevated. The same happens if `task_high` is terminated by `tn_task_terminate()`;
  * If low-priority task `task_low1` locks mutex M1 with priority inheritance, another task `task_low2` with the same priority tries to lock M1 and gets blocked, then high-priority task `task_high` tries to lock M1 and gets blocked (priority of `task_low1` becomes equal to priority of `task_high`). Now, `task_low1` unlocks M1 -> priority of `task_low1` returns to base value, `task_low2` locks M1 (because it is the next task in the mutex's queue), and its priority should be elevated to priority of `task_high`, but it doesn't happen.
  * `tn_mutex_delete()` : `mutex->holder` is checked against `tn_curr_run_task` without disabling interrupts;
  * `tn_mutex_delete()` : if mutex is not locked, `TERR_ILUSE` is returned. I believe task should be able to delete non-locked mutex;
  * if task that waits for mutex is in `WAITING_SUSPENDED` state, and mutex is deleted, `TERR_NO_ERR` is returned after returning from `SUSPENDED` state, instead of `TERR_DLT`
  * `tn_sys_tclice_ticks()` : if wrong param is given, `TERR_WRONG_PARAM` is returned and interrupts remain disabled.

I'm rewriting it in centralized and consistent way, and test it carefully by unit tests, which are, of course, must-have for project like this. It's so strange but original TNKernel seems totally untested.

Note that PIC32-dependent routines (such as context switch and so on) are originally implemented by Anders Montonen; I examined them in detail and changed several things which I believe should be implemented differently. Anders, great thanks for sharing your job.

Another existing PIC32 port, [the one by Alex Borisov](http://www.tnkernel.com/tn_port_pic24_dsPIC_PIC32.html), also affected my port. In fact, I used to use Alex's port for a long time, but it has several concepts that I don't like, so I had to move eventually. Nevertheless, Alex's port has several nice ideas and solutions, so I didn't hesitate to take what I like from his port. Alex, great thanks to you too.

For a full description of the kernel API, please see the [TNKernel project documentation](http://www.tnkernel.com/tn_description.html "TNKernel project documentation"). Note though that this project has slightly different API from original TNKernel, see below.

#Wiki contents

  * [Building the project](/dfrank/tneokernel/wiki/building)
  * [PIC32 port implementation details](/dfrank/tneokernel/wiki/pic32_details)
  * [Differences from original TNKernel](/dfrank/tneokernel/wiki/diff_orig_tnkernel)
  * [Differences from the port by Alex Borisov](/dfrank/tneokernel/wiki/diff_alexb_tnkernel)
  * [Why refactor?](/dfrank/tneokernel/wiki/why_refactor)
  * [License](/dfrank/tneokernel/wiki/license)
