#ifdef OPROFILER
#define BEGIN_ROI GOTO_SIM(); TIMER_READ(startTime); system("sudo opcontrol --start"); asm("movl $1025, %ecx\n\t" 	"xchg %rcx, %rcx");
 
#define END_ROI system("sudo opcontrol --stop"); TIMER_READ(stopTime); GOTO_REAL(); asm("movl $1026, %ecx\n\t"  "xchg %rcx, %rcx"); 

#else
#define BEGIN_ROI GOTO_SIM(); TIMER_READ(startTime); /*asm("movl $1025, %ecx\n\t"  "xchg %rcx, %rcx");*/
 
#define END_ROI TIMER_READ(stopTime); GOTO_REAL(); /*asm("movl $1026, %ecx\n\t"  "xchg %rcx, %rcx");*/

#endif

#define INSERT_MALLOC asm("mov $666, %rcx\n\t" " movl $1027, %ecx\n\t"  "xchg %rcx, %rcx");
