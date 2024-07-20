/*Please Note that:
 * 1- We ran the program 2 times to change the Queue size instead of macros to
 * 	  be easy to get the results extracted.
 * 2- We commented all the check-if-allocated or running properly trace_print,
 *    only the results and the Timer is XYZ is left uncommented in the program.
 *
 *							Thank You
 * */

// -----------------------------Includes----------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

// ------------------------------Definitions------------------------------------

#define CCM_RAM __attribute__((section(".ccmram")))

// -----------------------------Functions Declarations--------------------------


void ReceiverTask (void *parameters);
void ReceiverTimerCallback(TimerHandle_t xTimer3);

int RandomNumberGen(int lower, int upper);
void initSemaphores();
void reset();

void SenderTask0 (void *parameters);
void SenderTask1 (void *parameters);
void SenderTask2 (void *parameters);

void SenderTimerCallback0(TimerHandle_t xTimer0);
void SenderTimerCallback1(TimerHandle_t xTimer1);
void SenderTimerCallback2(TimerHandle_t xTimer2);


//-------------------------------Global Variables-------------------------------


SemaphoreHandle_t xSemaphore0;
SemaphoreHandle_t xSemaphore1;
SemaphoreHandle_t xSemaphore2;
SemaphoreHandle_t xSemaphore3;

QueueHandle_t QueueChannel;

TimerHandle_t xTimer0;
TimerHandle_t xTimer1;
TimerHandle_t xTimer2;
TimerHandle_t xTimer3;

TaskHandle_t xTaskHandle0;
TaskHandle_t xTaskHandle1;
TaskHandle_t xTaskHandle2;
TaskHandle_t xTaskHandle3;

int32_t LowerBoundArray[] = {50, 80, 110, 140, 170, 200};
int32_t UpperBoundArray[] = {150, 200, 250, 300, 350, 400};
int32_t ArrayIndex = -1; 			 //To be incremented in the rest function.

int32_t CounterBlock0 = 0, CounterSent0 = 0;
int32_t CounterBlock1 = 0, CounterSent1 = 0;
int32_t CounterBlock2 = 0, CounterSent2 = 0;
int32_t CounterReceived = 0;

int32_t CounterOfAvgTime0 = 0;
int32_t CounterOfAvgTime1 = 0;
int32_t CounterOfAvgTime2 = 0;
int32_t CounterOfAvgTimeTotal = 0;

BaseType_t xTimer0Started, xTimer1Started, xTimer2Started, xTimer3Started;


//----------------------------Main----------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

int main(void)
{
	BaseType_t Sender0Status;
	BaseType_t Sender1Status;
	BaseType_t Sender2Status;
	BaseType_t ReceiverStatus;
	QueueChannel = xQueueCreate(3, sizeof(char[50])); // Then we change to 10.

	initSemaphores();
	reset();

	CounterOfAvgTime0 += pdMS_TO_TICKS(RandomNumberGen(LowerBoundArray[ArrayIndex],UpperBoundArray[ArrayIndex]));
	CounterOfAvgTime1 += pdMS_TO_TICKS(RandomNumberGen(LowerBoundArray[ArrayIndex],UpperBoundArray[ArrayIndex]));
	CounterOfAvgTime2 += pdMS_TO_TICKS(RandomNumberGen(LowerBoundArray[ArrayIndex],UpperBoundArray[ArrayIndex]));

	xTimer0 = xTimerCreate( "Sender1", CounterOfAvgTime0, pdTRUE, ( void * ) 0, SenderTimerCallback0);
	xTimer1 = xTimerCreate( "Sender2", CounterOfAvgTime1, pdTRUE, ( void * ) 1, SenderTimerCallback1);
	xTimer2 = xTimerCreate( "Sender3", CounterOfAvgTime2, pdTRUE, ( void * ) 2, SenderTimerCallback2);
	xTimer3 = xTimerCreate( "Receiver",( pdMS_TO_TICKS(100) ), pdTRUE, ( void * ) 3, ReceiverTimerCallback);

	if( ( xTimer0 != NULL ) && ( xTimer1 != NULL ) && ( xTimer2 != NULL ) && ( xTimer3 != NULL ) )
	{
		xTimer0Started = xTimerStart( xTimer0, 0 );
		xTimer1Started = xTimerStart( xTimer1, 0 );
		xTimer2Started = xTimerStart( xTimer2, 0 );
		xTimer3Started = xTimerStart( xTimer3, 0 );
	}

	if( xTimer0Started == pdPASS && xTimer1Started == pdPASS && xTimer2Started == pdPASS && xTimer3Started == pdPASS)
	{
		//trace_printf("Timers created successfully. \r\n");
	}
	else
	{
		//trace_printf("Error: Could not create timers, exiting. \r\n");
		exit(0);
	}


	if (QueueChannel != NULL)
	{	//Creation of Tasks
		Sender0Status = xTaskCreate (SenderTask0, "Sender0", 1000 /*Stack*/,
						(void*)100 /*param for sender*/, 1/*priority*/, xTaskHandle0);
		Sender1Status = xTaskCreate (SenderTask1, "Sender1", 1000, (void*)200, 1, xTaskHandle1);
		Sender2Status = xTaskCreate (SenderTask2, "Sender2", 1000, (void*)200, 2, xTaskHandle2);

		/*Reader task created with priority 3, higher than sender tasks.*/
		ReceiverStatus = xTaskCreate (ReceiverTask, "Receiver", 1000, NULL, 3, xTaskHandle3);

		if( Sender0Status == pdPASS && Sender1Status == pdPASS && Sender2Status == pdPASS && ReceiverStatus == pdPASS)
		{
				/*Start the scheduler so the created tasks start executing.*/
				//trace_printf("Tasks created, starting the program. \r\n");
				vTaskStartScheduler();
		}
		else
		{
				//trace_printf("Error: Could not create tasks, exiting. \r\n");
				exit(0);
		}
	}
	else
	{
		/*Queue Could not be created. ==> Print error & exit peacefully*/
		//trace_printf("Error: Queue Could not be created. \r\n");
		exit(0);
	}

}

#pragma GCC diagnostic pop

// -------------------------Function Definitions-------------------------------

void ReceiverTask(void *parameters)
{
    // parameters is not used, so explicitly ignore it
    (void)parameters;

    char ReceivedMessage[50];

    int32_t ReceivedValue;
    BaseType_t status;

    while (1)
    {
        // Wait for the semaphore to be given by the timer callback
        if (xSemaphoreTake(xSemaphore3, portMAX_DELAY) == pdTRUE)
        {
            // Semaphore obtained, proceed to read from the queue
            status = xQueueReceive(QueueChannel, &ReceivedMessage, 0);

            if (status == pdPASS)
            {
            	trace_printf("ReceivedMessage = %s \r\n", ReceivedMessage);
                CounterReceived++;
                if (CounterReceived == 1000)
                {
                	//trace_printf("Completed 1000 messages, calling reset.\r\n");
                	reset (); // reset function
                }
            }
            else
            {
            	//trace_printf("Could not receive from the queue.\r\n");
            }
        }
    }
}
void ReceiverTimerCallback(TimerHandle_t xTimer3)
{
	//trace_printf("Hello From Receiver CB. %lu \r\n", xTimer3);

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// Release the semaphore to unblock the receiver task
    xSemaphoreGiveFromISR(xSemaphore3, &xHigherPriorityTaskWoken);
    // Perform context switch if required
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void reset()
{
	trace_printf("***********************Hello from Reset, Array Index =%i ********************** \r\n",ArrayIndex);
	if(ArrayIndex == -1)
	{
		CounterBlock0 = CounterBlock1 = CounterBlock2 = 0;
		CounterSent0  = CounterSent1  = CounterSent2  = 0;
		CounterReceived = 0;

		xQueueReset(QueueChannel);

		ArrayIndex++;
		trace_printf("Initialized Reset is done. \r\n");
	}

	else
	{

		int32_t TotalBlocked = CounterBlock0 + CounterBlock1 + CounterBlock2;
		int32_t TotalSent	 = CounterSent0  + CounterSent1  + CounterSent2 ;

		//Calculating the Total Average time:
		CounterOfAvgTimeTotal= ((CounterOfAvgTime0/(CounterBlock0+CounterSent0))+(CounterOfAvgTime1/(CounterBlock1+CounterSent1))+(CounterOfAvgTime2/(CounterBlock2+CounterSent2)))/3;

		//Printing the Counters:
		trace_printf("Total Sent = %i, Total Blocked = %i, AvgTimeTotal = %i.\r\n",TotalSent,TotalBlocked, CounterOfAvgTimeTotal);
		trace_printf("Sender task 0:\r\n  CounterBlock0 = %i, CounterSent0 = %i.\r\n",CounterBlock0,CounterSent0 );
		trace_printf("Sender task 1:\r\n  CounterBlock1 = %i, CounterSent1 = %i.\r\n",CounterBlock1,CounterSent1 );
		trace_printf("Sender task 2 [Highest priority]:\r\n  CounterBlock2 = %i, CounterSent2 = %i.\r\n",CounterBlock2,CounterSent2 );

		//Average Time calculations & resetting the Counter for the next period:
		trace_printf("Sender0: AvgTime0 = %i, Sent = %i \r\n", CounterOfAvgTime0/(CounterBlock0+CounterSent0),CounterSent0);
		trace_printf("Sender0: AvgTime0 = %i, Blocked = %i \r\n", CounterOfAvgTime0/(CounterBlock0+CounterSent0),CounterBlock0);

		trace_printf("Sender1: AvgTime1 = %i, Sent = %i \r\n", CounterOfAvgTime1/(CounterBlock1+CounterSent1),CounterSent1);
		trace_printf("Sender1: AvgTime1 = %i, Blocked = %i \r\n", CounterOfAvgTime1/(CounterBlock1+CounterSent1),CounterBlock1);

		trace_printf("Sender2: AvgTime2 = %i, Sent = %i \r\n", CounterOfAvgTime2/(CounterBlock2+CounterSent2),CounterSent2);
		trace_printf("Sender2: AvgTime2 = %i, Blocked = %i \r\n", CounterOfAvgTime2/(CounterBlock2+CounterSent2),CounterBlock2);

		CounterOfAvgTime0 = 0;
		CounterOfAvgTime1 = 0;
		CounterOfAvgTime2 = 0;
		CounterOfAvgTimeTotal = 0;

		//Reseting the Counters:
		CounterBlock0 = CounterBlock1 = CounterBlock2 = 0;
		CounterSent0  = CounterSent1  = CounterSent2  = 0;
		CounterReceived = 0;

    	// Clear the queue:
    	xQueueReset(QueueChannel);

    	//Adjusting the lower and upper bound:
    	ArrayIndex++;

		if (ArrayIndex > 5 )
		{
			//Terminating the program:
			trace_printf("Game Over");

			xTimerStop(xTimer0, 0);
			xTimerDelete(xTimer0, 0);
			xTimerStop(xTimer1, 0);
			xTimerDelete(xTimer1, 0);
			xTimerStop(xTimer2, 0);
			xTimerDelete(xTimer2, 0);
			xTimerStop(xTimer3, 0);
			xTimerDelete(xTimer3, 0);

	        vTaskDelete(xTaskHandle0);
	        vTaskDelete(xTaskHandle1);
	        vTaskDelete(xTaskHandle2);
	        vTaskDelete(xTaskHandle3);

			exit(0);
		}
	}

}

void initSemaphores()
{
	xSemaphore0 = xSemaphoreCreateBinary();
	xSemaphore1 = xSemaphoreCreateBinary();
    xSemaphore2 = xSemaphoreCreateBinary();
    xSemaphore3 = xSemaphoreCreateBinary();
    if (xSemaphore0 == NULL || xSemaphore1 == NULL || xSemaphore2 == NULL || xSemaphore3 == NULL)
    {
    	//trace_printf("Semaphore creation failed. \r\n");
        exit(0);
    }
    else
    {
    	//trace_printf("Semaphore created successfully. \r\n");
    }
}
int RandomNumberGen(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}

void SenderTask0 (void *parameters)
{
	BaseType_t xStatus;
	char Message[50];			 // to hold the message
	TickType_t CurrentTicks;

	 while (1)
	 {
	        // Wait for the semaphore to be given by the timer callback
	        if (xSemaphoreTake(xSemaphore0, portMAX_DELAY) == pdTRUE)
	        {
	            // Get the current time in ticks
	            CurrentTicks = xTaskGetTickCount();
	            snprintf(Message, sizeof(Message), "Time is %lu", CurrentTicks); //print the ticks

	            // Try to send the message to the queue
	            xStatus = xQueueSend(QueueChannel, &Message, 0);
	            if (xStatus == pdPASS)
	            {
	            	(CounterSent0)++;
	            	//trace_printf("Message sent from sender0. \r\n");
	            }
	            else
	            {
	                (CounterBlock0)++;
	                //trace_printf("Message blocked from sender0. \r\n");
	            }
	        }

	 }
}
void SenderTask1 (void *parameters)
{
	BaseType_t xStatus;
	char Message[50];			 // to hold the message
	TickType_t CurrentTicks;

	 while (1)
	 {
	        // Wait for the semaphore to be given by the timer callback
	        if (xSemaphoreTake(xSemaphore1, portMAX_DELAY) == pdTRUE)
	        {
	            // Get the current time in ticks
	            CurrentTicks = xTaskGetTickCount();
	            snprintf(Message, sizeof(Message), "Time is %lu", CurrentTicks); //print the ticks

	            // Try to send the message to the queue
	            xStatus = xQueueSend(QueueChannel, &Message, 0);
	            if (xStatus == pdPASS)
	            {
	                (CounterSent1)++;
	                //trace_printf("Message sent from sender1. \r\n");
	            }
	            else
	            {
	                (CounterBlock1)++;
	                //trace_printf("Message blocked from sender1. \r\n");
	            }
	        }

	 }
}
void SenderTask2 (void *parameters)
{
	BaseType_t xStatus;
	char Message[50];			 // to hold the message
	TickType_t CurrentTicks;

	 while (1)
	 {
	        // Wait for the semaphore to be given by the timer callback
	        if (xSemaphoreTake(xSemaphore2, portMAX_DELAY) == pdTRUE)
	        {
	            // Get the current time in ticks
	            CurrentTicks = xTaskGetTickCount();
	            snprintf(Message, sizeof(Message), "Time is %lu", CurrentTicks); //print the ticks

	            // Try to send the message to the queue
	            xStatus = xQueueSend(QueueChannel, &Message, 0);
	            if (xStatus == pdPASS)
	            {
	                (CounterSent2)++;
	                //trace_printf("Message sent from sender2. \r\n");
	            }
	            else
	            {
	                (CounterBlock2)++;
	                //trace_printf("Message blocked from sender2. \r\n");
	            }
	        }

	 }
}

void SenderTimerCallback0(TimerHandle_t xTimer0)
{
    // Generate new random timer period
    uint32_t newPeriod = pdMS_TO_TICKS(RandomNumberGen(LowerBoundArray[ArrayIndex], UpperBoundArray[ArrayIndex]));

    //Timer delay to wait for change, is indefinitely so it return with a changed period.
    const TickType_t xTicksToWait = portMAX_DELAY;

    // Check if the timer changed:
    if (xTimerChangePeriod(xTimer0, newPeriod, xTicksToWait) == pdPASS)
    {
    	//trace_printf("Period of Timer0 Changed successfully. \r\n");
    	CounterOfAvgTime0 += newPeriod;
    }
    else
    {
    	//trace_printf("Could not change the period of Timer0.");
	}

	// Releasing the Semaphore:
	//trace_printf("Hello From CB0. %lu \r\n", xTimer0);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore0, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}
void SenderTimerCallback1(TimerHandle_t xTimer1)
{
    // Generate new random timer period
    uint32_t newPeriod = pdMS_TO_TICKS(RandomNumberGen(LowerBoundArray[ArrayIndex], UpperBoundArray[ArrayIndex]));

    //Timer delay to wait for change, is indefinitely so it return with a changed period.
    const TickType_t xTicksToWait = portMAX_DELAY;

    // Check if the timer changed:
    if (xTimerChangePeriod(xTimer1, newPeriod, xTicksToWait) == pdPASS)
    {
    	//trace_printf("Period of Timer1 Changed successfully. \r\n");
    	CounterOfAvgTime1 += newPeriod;
    }
    else
    {
    	//trace_printf("Could not change the period of Timer1.");
	}

	// Releasing the Semaphore:
	//trace_printf("Hello From CB1. %lu \r\n", xTimer1);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore1, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void SenderTimerCallback2(TimerHandle_t xTimer2)
{
    // Generate new random timer period
    uint32_t newPeriod = pdMS_TO_TICKS(RandomNumberGen(LowerBoundArray[ArrayIndex], UpperBoundArray[ArrayIndex]));

    //Timer delay to wait for change, is indefinitely so it return with a changed period.
    const TickType_t xTicksToWait = portMAX_DELAY;

    // Check if the timer changed:
    if (xTimerChangePeriod(xTimer2, newPeriod, xTicksToWait) == pdPASS)
    {
    	//trace_printf("Period of Timer2 Changed successfully. \r\n");
    	CounterOfAvgTime2 += newPeriod;
    }
    else
    {
    	//trace_printf("Could not change the period of Timer2.");
	}

	// Releasing the Semaphore:
	//trace_printf("Hello From CB2. %lu \r\n", xTimer2);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore2, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
