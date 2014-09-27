/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeoKernel.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * \file
 * 
 * Definitions used through the whole kernel.
 */

#ifndef _TN_COMMON_H
#define _TN_COMMON_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- Configuration constants
#define  TN_API_MAKE_ALIG_ARG__TYPE       1     //-- this way is used in the majority of ports.
#define  TN_API_MAKE_ALIG_ARG__SIZE       2     //-- this way is stated in TNKernel docs
                                                //   and used in port by AlexB.


//--- As a starting point, you might want to copy tn_cfg_default.h -> tn_cfg.h,
//    and then edit it if you want to change default configuration.
//    NOTE: the file tn_cfg.h is specified in .hgignore file, in order to not include
//    project-specific configuration in the common TNKernel repository.
#include "tn_cfg.h"

//--- default cfg file is included too, so that you are free to not set
//    all available options in your tn_cfg.h file.
#include "tn_cfg_default.h"


#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Magic number for object validity verification
 */
enum TN_ObjId {
   TN_ID_TASK           = 0x47ABCF69,
   TN_ID_SEMAPHORE      = 0x6FA173EB,
   TN_ID_EVENTGRP       = 0x5E224F25,
   TN_ID_DATAQUEUE      = 0x8C8A6C89,
   TN_ID_FSMEMORYPOOL   = 0x26B7CE8B,
   TN_ID_MUTEX          = 0x17129E45,
   TN_ID_RENDEZVOUS     = 0x74289EBD,
};

/**
 * Result code returned by kernel services.
 */
enum TN_RCode {
   ///
   /// Successful operation
   TN_RC_OK                   =   0,
   ///
   /// Timeout (consult `TN_Timeout` for details).
   /// @see `TN_Timeout`
   TN_RC_TIMEOUT              =  -1,
   ///
   /// This code is returned in the following cases:
   ///   * Trying to increment semaphore count more than its max count;
   ///   * Trying to return extra memory block to fixed memory pool.
   /// @see tn_sem.h
   /// @see tn_fmem.h
   TN_RC_OVERFLOW             =  -2,
   ///
   /// Wrong context error: returned if function is called from 
   /// non-acceptable context. Required context suggested for every
   /// function by badges:
   ///
   ///   * $(TN_CALL_FROM_TASK) - function can be called from task;
   ///   * $(TN_CALL_FROM_ISR) - function can be called from ISR.
   ///
   /// @see `tn_sys_context_get()`
   /// @see `enum TN_Context`
   TN_RC_WCONTEXT             =  -3,
   ///
   /// Wrong task state error: requested operation requires different 
   /// task state
   TN_RC_WSTATE               =  -4,
   ///
   /// This code is returned by most of the kernel functions when 
   /// wrong params were given to function. This error code can be returned
   /// if only build-time option `TN_CHECK_PARAM` is non-zero
   /// @see `TN_CHECK_PARAM`
   TN_RC_WPARAM               =  -5,
   ///
   /// Illegal usage. Returned in the following cases:
   /// * task tries to unlock or delete the mutex that is locked by different
   ///   task,
   /// * task tries to lock mutex with priority ceiling whose priority is
   ///   lower than task's priority
   /// @see tn_mutex.h
   TN_RC_ILLEGAL_USE          =  -6,
   ///
   /// Returned when user tries to perform some operation on invalid object
   /// (mutex, semaphore, etc).
   /// Object validity is checked by comparing special `id_...` value with the
   /// value from `enum TN_ObjId`
   /// @see `TN_CHECK_PARAM`
   TN_RC_INVALID_OBJ          =  -7,
   /// Object for whose event task was waiting is deleted.
   TN_RC_DELETED              =  -8,
   /// Task was released from waiting forcibly because some other task 
   /// called `tn_task_release_wait()`
   TN_RC_FORCED               =  -9,
   /// Internal kernel error, should never be returned by kernel services.
   /// If it is returned, it's a bug in the kernel.
   TN_RC_INTERNAL             = -10,
};

/**
 * The value representing maximum number of system ticks to wait.
 *
 * Assume user called some system function, and it can't perform its job 
 * immediately (say, it needs to lock mutex but it is already locked, etc).
 *
 * So, function can wait or return an error. There are possible `timeout`
 * values and appropriate behavior of the function:
 *
 *    * `timeout` is set to `0`: function doesn't wait at all, no context switch is performed,
 *      `TN_RC_TIMEOUT` is returned immediately.
 *    * `timeout` is set to `TN_WAIT_INFINITE`: function waits until it eventually **can** perform
 *      its job. Timeout is not taken in account, so `TN_RC_TIMEOUT`
 *      is never returned.
 *    * `timeout` is set to other value: function waits at most specified number of system ticks.
 *      Strictly speaking, it waits from `(timeout - 1)` to `timeout` ticks. So, if
 *      you specify that timeout is 1, be aware that it might actually don't
 *      wait at all: if system timer interrupt happens just while function is
 *      putting task to wait (with interrupts disabled), then ISR will be
 *      executed right after function puts task to wait. Then
 *      `tn_tick_int_processing()` will immediately remove the task from wait
 *      queue and make it runnable again.
 *
 *      So, to guarantee that task waits *at least* 1 system tick,
 *      you should specify timeout value of `2`.
 *
 * **Note** also that there are other possible ways to make task runnable:
 *
 *    * if task waits because of call to `tn_task_sleep()`, it may be woken up
 *      by some other task, by means of `tn_task_wakeup()`. In this case,
 *      `tn_task_sleep()` returns `TN_RC_OK`.  
 *    * independently of the wait reason, task may be released from wait
 *      forcibly, by means of `tn_task_release_wait()`. It this case,
 *      `TN_RC_FORCED` is returned by the waiting function.
 *      (the usage of this function is discouraged)
 */
typedef unsigned long TN_Timeout;


/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


#ifndef NULL
#  define NULL       ((void *)0)
#endif

#ifndef BOOL
#  define BOOL       int
#endif

#ifndef TRUE
#  define TRUE       (1 == 1)
#endif

#ifndef FALSE
#  define FALSE      (1 == 0)
#endif


//-- TN_MAKE_ALIG() macro
/**
 * Macro for making a number a multiple of `TN_ALIGN`, should be used with
 * fixed memory block pool.
 *
 * @see `tn_fmem_create()`
 * @see `TN_ALIGN`
 */
#define  TN_MAKE_ALIG_SIZE(a)  (((a) + (TN_ALIGN - 1)) & (~(TN_ALIGN - 1)))

//-- self-checking
#if (!defined TN_API_MAKE_ALIG_ARG)
#  error TN_API_MAKE_ALIG_ARG is not defined
#elif (!defined TN_API_MAKE_ALIG_ARG__TYPE)
#  error TN_API_MAKE_ALIG_ARG__TYPE is not defined
#elif (!defined TN_API_MAKE_ALIG_ARG__SIZE)
#  error TN_API_MAKE_ALIG_ARG__SIZE is not defined
#endif

//-- define MAKE_ALIG accordingly to config
#if (TN_API_MAKE_ALIG_ARG == TN_API_MAKE_ALIG_ARG__TYPE)
#  define  TN_MAKE_ALIG(a)  TN_MAKE_ALIG_SIZE(sizeof(a))
#elif (TN_API_MAKE_ALIG_ARG == TN_API_MAKE_ALIG_ARG__SIZE)
#  define  TN_MAKE_ALIG(a)  TN_MAKE_ALIG_SIZE(a)
#else
#  error wrong TN_API_MAKE_ALIG_ARG
#endif



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_COMMON_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


