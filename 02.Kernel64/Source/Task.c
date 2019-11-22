#include "Task.h"
#include "Descriptor.h"

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;
#define A 16807L
#define M 2147483647L
static long ln[16] = {0L,1973272912L,747177549L,20464843L,640830765L,1098742207L,78126602L,84743774L,831312807L, 124667236L,1172177002L,1124933064L,1223960546L,1878892440L,1449793615L,553303732L};
static int strm = 1;
long ranf(){
	short *p,*q,k;
	long Hi,Lo;
	p= (short *)&ln[strm];
	Hi = *(p+1)*A;
	*(p+1) = 0;
	Lo = ln[strm]*A;
	p = (short *)&Lo;
    Hi += *(p+1);
    q = (short *)&Hi;   
	*(p+1) = *q&0x7FFF;
	k = *(q+1)<<1;
	if(*q&0x7FFF){ k++;}
	Lo -= M;
	Lo +=k;
	if(Lo<0) {Lo+=M;}
	ln[strm] = Lo;
	return (long)Lo*4-10 ;
}
void kInitializeTCBPool( void )
{
    int i;
    
    kMemSet( &( gs_stTCBPoolManager ), 0, sizeof( gs_stTCBPoolManager ) );
    
    gs_stTCBPoolManager.pstStartAddress = ( TCB* ) TASK_TCBPOOLADDRESS;
    kMemSet( TASK_TCBPOOLADDRESS, 0, sizeof( TCB ) * TASK_MAXCOUNT );

    for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
    {
        gs_stTCBPoolManager.pstStartAddress[ i ].stLink.qwID = i;
    }
    
    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

TCB* kAllocateTCB( void )
{
    TCB* pstEmptyTCB;
    int i;
    
    if( gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount )
    {
        return NULL;
    }

    for( i = 0 ; i < gs_stTCBPoolManager.iMaxCount ; i++ )
    {
        // ID의 상위 32비트가 0이면 할당되지 않은 TCB
        if( ( gs_stTCBPoolManager.pstStartAddress[ i ].stLink.qwID >> 32 ) == 0 )
        {
            pstEmptyTCB = &( gs_stTCBPoolManager.pstStartAddress[ i ] );
            break;
        }
    }
    pstEmptyTCB->stLink.qwID = ( ( QWORD ) gs_stTCBPoolManager.iAllocatedCount << 32 ) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if( gs_stTCBPoolManager.iAllocatedCount == 0 )
    {
        gs_stTCBPoolManager.iAllocatedCount = 1;
    }
    
    return pstEmptyTCB;
}

void kFreeTCB( QWORD qwID )
{
    int i;
    
    i = qwID & 0xFFFFFFFF;
    
    // TCB를 초기화하고 ID 설정
    kMemSet( &( gs_stTCBPoolManager.pstStartAddress[ i ].stContext ), 0, sizeof( CONTEXT ) );
    gs_stTCBPoolManager.pstStartAddress[ i ].stLink.qwID = i;
    
    gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask( QWORD qwFlags, QWORD qwEntryPointAddress )
{
    TCB* pstTask;
    void* pvStackAddress;
    
    pstTask = kAllocateTCB();
    if( pstTask == NULL )
    {
        return NULL;
    }
    
    pvStackAddress = ( void* ) ( TASK_STACKPOOLADDRESS + ( TASK_STACKSIZE * 
            ( pstTask->stLink.qwID & 0xFFFFFFFF ) ) );
    
    kSetUpTask( pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, 
            TASK_STACKSIZE );
    kAddTaskToReadyList( pstTask );
    
    return pstTask;
}

void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
                 void* pvStackAddress, QWORD qwStackSize )
{
    // 콘텍스트 초기화
    kMemSet( pstTCB->stContext.vqRegister, 0, sizeof( pstTCB->stContext.vqRegister ) );
    // 스택에 관련된 RSP, RBP 레지스터 설정
    pstTCB->stContext.vqRegister[ TASK_RSPOFFSET ] = ( QWORD ) pvStackAddress + 
            qwStackSize;
    pstTCB->stContext.vqRegister[ TASK_RBPOFFSET ] = ( QWORD ) pvStackAddress + 
            qwStackSize;

    // 세그먼트 셀렉터 설정
    pstTCB->stContext.vqRegister[ TASK_CSOFFSET ] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[ TASK_DSOFFSET ] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[ TASK_ESOFFSET ] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[ TASK_FSOFFSET ] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[ TASK_GSOFFSET ] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[ TASK_SSOFFSET ] = GDT_KERNELDATASEGMENT;
    
    // RIP 레지스터와 인터럽트 플래그 설정
    pstTCB->stContext.vqRegister[ TASK_RIPOFFSET ] = qwEntryPointAddress;

    // RFLAGS 레지스터의 IF 비트(비트 9)를 1로 설정하여 인터럽트 활성화
    pstTCB->stContext.vqRegister[ TASK_RFLAGSOFFSET ] |= 0x0200;
    // ticket 
    pstTCB->tickets = qwFlags*10;
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}

void kInitializeScheduler( void )
{
    kInitializeTCBPool();

    kInitializeList( &( gs_stScheduler.stReadyList ) );

    gs_stScheduler.pstRunningTask = kAllocateTCB();
}

void kSetRunningTask( TCB* pstTask )
{
    gs_stScheduler.pstRunningTask = pstTask;
}

TCB* kGetRunningTask( void )
{
        return gs_stScheduler.pstRunningTask;
}

TCB* kGetNextTaskToRun( void )
{
    if( kGetListCount( &( gs_stScheduler.stReadyList ) ) == 0 )
    {
        return NULL;
    }
       return ( TCB* ) kRemoveListFromHeader( &( gs_stScheduler.stReadyList ) );
}

void kAddTaskToReadyList( TCB* pstTask )
{
    kAddListToTail( &( gs_stScheduler.stReadyList ), pstTask );
}

void kSchedule( void )
{
    TCB* pstRunningTask, * pstNextTask;
    BOOL bPreviousFlag;
    
    if( kGetListCount( &( gs_stScheduler.stReadyList ) ) == 0 )
	{   
        return ;
}
	 LISTLINK *pstLink;
    pstLink = (LISTLINK *)(&gs_stScheduler.stReadyList)->pvHeader;
    pstNextTask = (TCB *)pstLink;
    //
    //instead of "kGetNextTaskToRun() , due to we won't remove the header list.
    //this implementation always guarantees to point 'Task 1' List. 
    // 
    bPreviousFlag = kSetInterruptFlag( FALSE );
    if( pstNextTask == NULL )
    {
		kPrintf("interrupt \n");
        kSetInterruptFlag( bPreviousFlag );
        return ;
    }
    int ticket=0;
    long ran = ranf() ;     // random number created.
   	ran = ran % (long)550;  //just example. Task 1 - 10 allocated range in 1~550
    pstRunningTask = gs_stScheduler.pstRunningTask; 
	while(1){
		ticket += pstNextTask->tickets;
	if(ticket >= ran){    
	    gs_stScheduler.pstRunningTask = pstNextTask;
	    break;
	}
	else{       pstLink = pstLink->pvNext;
				pstNextTask = (TCB *)pstLink;  // otherwise, move on .
	}
	}
    //kAddTaskToReadyList( pstRunningTask );
	// we don't need to add the running task list back anymore cuz we haven't removed it before
	kSwitchContext( &( pstRunningTask->stContext ), &( pstNextTask->stContext ) );

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    kSetInterruptFlag( bPreviousFlag );
}

BOOL kScheduleInInterrupt( void )
{
    TCB* pstRunningTask, * pstNextTask;
    char* pcContextAddress;
	// kGetHeaderFromList() instead of 'kGetNextTaskToRun()'.
    pstNextTask = kGetHeaderFromList(&gs_stScheduler.stReadyList);
  	if( pstNextTask == NULL )
    {
        return FALSE;
    }
    
    pcContextAddress = ( char* ) IST_STARTADDRESS + IST_SIZE - sizeof( CONTEXT );
    
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kMemCpy( &( pstRunningTask->stContext ), pcContextAddress, sizeof( CONTEXT ) );
//    kAddTaskToReadyList( pstRunningTask );

    gs_stScheduler.pstRunningTask = pstNextTask;
    kMemCpy( pcContextAddress, &( pstNextTask->stContext ), sizeof( CONTEXT ) );
    
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    return TRUE;
}

void kDecreaseProcessorTime( void )
{
    if( gs_stScheduler.iProcessorTime > 0 )
    {
        gs_stScheduler.iProcessorTime--;
    }
}

BOOL kIsProcessorTimeExpired( void )
{
    if( gs_stScheduler.iProcessorTime <= 0 )
    {
        return TRUE;
    }
    return FALSE;
}
int kGetTaskCount( void )
{
    int iTotalCount;
    
    iTotalCount = kGetReadyTaskCount();
    iTotalCount += kGetListCount( &( gs_stScheduler.stReadyList ) ) + 1;

    return iTotalCount;
}

TCB* kGetTCBInTCBPool( int iOffset )
{
    if( ( iOffset < -1 ) && ( iOffset > TASK_MAXCOUNT ) )
    {
        return NULL;
    }
    
    return &( gs_stTCBPoolManager.pstStartAddress[ iOffset ] );
}

int kGetReadyTaskCount( void )
{
    int iTotalCount = 0;
    int i;
    
    for( i = 0 ; i < 10 ; i++ )
    {
        iTotalCount += kGetListCount( &( gs_stScheduler.stReadyList ) );
    }
    
    return iTotalCount ;
}
