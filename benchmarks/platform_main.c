#include <stdint.h>
#include "boardsupport.h"
void initialise_board(void); void start_trigger(void); void stop_trigger(void);
int benchmark_entry(int argc, char **argv);
void setStats(int enable) { if(enable) start_trigger(); else stop_trigger(); }
int main(void) { initialise_board(); int result=benchmark_entry(0, (char **)0); finish_benchmark(result==0); return result; }
