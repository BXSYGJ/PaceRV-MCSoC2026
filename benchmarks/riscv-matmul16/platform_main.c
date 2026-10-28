#include <stddef.h>
#include "dataset.h"
#include "boardsupport.h"
void initialise_board(void); void start_trigger(void); void stop_trigger(void);
void matmul(size_t,size_t,size_t,const data_t *,const data_t *,data_t *);
static data_t results_data[ARRAY_SIZE];
int main(void) { initialise_board(); start_trigger();
matmul(0,1,DIM_SIZE,input1_data,input2_data,results_data); stop_trigger();
int valid=1; for(unsigned i=0;i<ARRAY_SIZE;i++) if(results_data[i]!=verify_data[i]) valid=0;
finish_benchmark(valid); return !valid; }
