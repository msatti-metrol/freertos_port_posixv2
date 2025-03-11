#pragma once
#define PORTMACRO_H

#include <limits.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define portSTACK_TYPE unsigned char
#define portBASE_TYPE long
#define portPOINTER_SIZE_TYPE uintptr_t
#define portMAX_DELAY ((TickType_t)ULONG_MAX)
#define portTICK_TYPE_IS_ATOMIC (ATOMIC_LONG_LOCK_FREE == 2)
#define portSTACK_GROWTH 1
#define portHAS_STACK_OVERFLOW_CHECKING 0
#define portTICK_PERIOD_MS ((TickType_t)1000 / configTICK_RATE_HZ)
#define portTICK_RATE_MICROSECONDS ((TickType_t)1000000 / configTICK_RATE_HZ)
#define portBYTE_ALIGNMENT 8
#define portHAS_NESTED_INTERRUPTS 0
#define portYIELD() vPortYield()
#define portYIELD_FROM_ISR(x) vPortYield()
#define portDISABLE_INTERRUPTS() vPortDisableInterrupts()
#define portENABLE_INTERRUPTS() vPortEnableInterrupts()
#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL() vPortExitCritical()
#define portSETUP_TCB(pxTCB) vPortInitializeTask((TaskHandle_t)(pxTCB))
#define portCLEAN_UP_TCB(pxTCB) vPortDestructTask((TaskHandle_t)(pxTCB))
#define portTASK_FUNCTION_PROTO(vFunction, pvParameters) void vFunction(void* pvParameters) __attribute__((noreturn)) 
#define portTASK_FUNCTION(vFunction, pvParameters) void vFunction(void* pvParameters)
#define portMEMORY_BARRIER() vPortMemoryBarrier()
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vPortInitializeTimer()
#define portGET_RUN_TIME_COUNTER_VALUE() lPortGetTimerCounter()

struct tskTaskControlBlock;

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef unsigned long TickType_t;
typedef struct tskTaskControlBlock* TaskHandle_t;
typedef void (*TaskFunction_t)(void* parameters);

StackType_t* pxPortInitialiseStack(StackType_t* stack, TaskFunction_t taskFunction, void* taskParameters);
void vPortInitializeTask(TaskHandle_t pxTCB);
void vPortDestructTask(TaskHandle_t pxTCB);
BaseType_t xPortStartScheduler(void);
void vPortEndScheduler(void);
void vPortMemoryBarrier();
void vPortYield(void);
void vPortDisableInterrupts(void);
void vPortEnableInterrupts(void);
UBaseType_t xPortSetInterruptMask(void);
void vPortClearInterruptMask(UBaseType_t xMask);
void vPortEnterCritical(void);
void vPortExitCritical(void);
void vPortInitializeTimer(void);
long lPortGetTimerCounter(void);

#ifdef __cplusplus
}
#endif
