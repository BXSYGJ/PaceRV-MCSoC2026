#include "Vrv32_observed_soc.h"
#include "verilated.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <string>
// Standard SHA-256 compression; test vectors are checked before trace use.
struct TraceSHA256 {
    uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    uint8_t buf[64] = {}; unsigned used=0; uint64_t bytes=0;
    static uint32_t rr(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
    void block(){
        static const uint32_t k[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
        uint32_t w[64];
        for(unsigned i=0;i<16;i++)w[i]=(uint32_t(buf[4*i])<<24)|(uint32_t(buf[4*i+1])<<16)|(uint32_t(buf[4*i+2])<<8)|buf[4*i+3];
        for(unsigned i=16;i<64;i++)w[i]=w[i-16]+(rr(w[i-15],7)^rr(w[i-15],18)^(w[i-15]>>3))+w[i-7]+(rr(w[i-2],17)^rr(w[i-2],19)^(w[i-2]>>10));
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
        for(unsigned i=0;i<64;i++){uint32_t t1=z+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+k[i]+w[i];
            uint32_t t2=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c)); z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
    }
    void put(uint8_t v){buf[used++]=v;bytes++;if(used==64){block();used=0;}}
    void word(uint32_t v){for(unsigned i=0;i<4;i++)put(uint8_t(v>>(8*i)));}
    std::string finish(){uint64_t bits=bytes*8;put(0x80);while(used!=56)put(0);for(int i=7;i>=0;i--)put(uint8_t(bits>>(8*i)));
        const char* hex="0123456789abcdef";std::string out;for(uint32_t v:h)for(int i=7;i>=0;i--)out+=hex[(v>>(4*i))&15];return out;}
};

#include "native_timing_tick_v2.h"
#define P(x) model->study_##x
using Counts=std::map<std::string,uint64_t>;
static uint32_t cycle_read_pc_low=0,cycle_read_pc_high=0;
enum EventIndex {event_created,event_actual_fast_consume,event_invalidated,event_eligible_unconsumed,event_accepted_arithmetic_request,event_valid_response_handshake,event_token_hold_edges,event_retained_wait_edges,event_consume_after_wait,event_memory_blocked_cycles,event_ready_MUL_wait_cycles,event_early_request_accept,event_data_request_accept,event_data_response_accept,event_memory_launch_wait,event_memory_response_wait,event_frontend_missing,event_current_missing,event_following_missing,event_redirects,event_prefetch_operand_edges,event_fast_owner_retirements,event_i_refill_commands,event_i_refill_beats,event_i_refill_completions,event_d_refill_commands,event_d_refill_beats,event_d_refill_completions,event_refill_error_beats,event_count};
static constexpr const char* event_names[]={"created","actual_fast_consume","invalidated","eligible_unconsumed","accepted_arithmetic_request","valid_response_handshake","token_hold_edges","retained_wait_edges","consume_after_wait","memory_blocked_cycles","ready_MUL_wait_cycles","early_request_accept","data_request_accept","data_response_accept","memory_launch_wait","memory_response_wait","frontend_missing","current_missing","following_missing","redirects","prefetch_operand_edges","fast_owner_retirements","i_refill_commands","i_refill_beats","i_refill_completions","d_refill_commands","d_refill_beats","d_refill_completions","refill_error_beats"};
// Passive retirement dataflow: seed only the four start-trigger snapshot counter loads.
#include <unordered_set>
static uint32_t retired_read_pc_low=0,retired_read_pc_high=0;
struct TimerTaint {
    std::array<uint32_t,32> values{};
    std::array<bool,32> tags{};
    std::unordered_set<uint32_t> memory_bytes;
    bool result=false,source_a=false,source_b=false,address_tainted=false,control_tainted=false;
    uint32_t address=0;
    static int32_t signed12(uint32_t x){return int32_t(x<<20)>>20;}
    bool retirement(uint32_t pc,uint32_t word,bool writes,uint32_t rd,uint32_t value) {
        const unsigned opcode=word&127,rs1=(word>>15)&31,rs2=(word>>20)&31,f3=(word>>12)&7;
        source_a=tags[rs1];source_b=tags[rs2];address=0;address_tainted=false;control_tainted=false;result=false;
        if(opcode==0x03){
            address=values[rs1]+signed12(word>>20);address_tainted=source_a;
            const unsigned width=(f3==0||f3==4)?1:(f3==1||f3==5)?2:4;
            for(unsigned i=0;i<width;i++)result=result||memory_bytes.count(address+i);
            result=result||address_tainted;
            if(pc==cycle_read_pc_low||pc==cycle_read_pc_high||pc==retired_read_pc_low||pc==retired_read_pc_high){
                const uint32_t expected=pc==cycle_read_pc_low?0x10000410U:pc==cycle_read_pc_high?0x10000414U:pc==retired_read_pc_low?0x10000418U:0x1000041cU;
                if(address!=expected||f3!=2)throw std::runtime_error("Timer taint seed is not its declared cycle-word load");
                result=true;
            }
        } else if(opcode==0x23){
            const uint32_t immediate=((word>>25)<<5)|((word>>7)&31);
            address=values[rs1]+signed12(immediate);address_tainted=source_a;
            const unsigned width=f3==0?1:f3==1?2:4;
            for(unsigned i=0;i<width;i++){
                if(source_b||address_tainted)memory_bytes.insert(address+i);
                else memory_bytes.erase(address+i);
            }
        } else if(opcode==0x13){
            result=source_a;
            // AND with zero is a constant, independent of its input.
            if(f3==7&&signed12(word>>20)==0)result=false;
        } else if(opcode==0x33){
            result=source_a||source_b;
            // Exact self-subtraction, XOR, or comparison removes the dependency.
            if(rs1==rs2 && (((word>>25)==0 && (f3==4 || f3==2 || f3==3)) || (f3==0 && (word>>25)==0x20)))result=false;
        } else if(opcode==0x63){
            control_tainted=source_a||source_b;
        } else if(opcode==0x67){
            control_tainted=source_a; // Link-register data is the constant PC+length.
        } else if(opcode==0x73){
            // CSR values are not seeded by an instruction-independent blanket mask.
            // CSR writes using a timer-derived value are separately identified.
            control_tainted=(f3!=0&&f3<4)&&source_a;
        } else if(opcode!=0x37&&opcode!=0x17&&opcode!=0x6f&&opcode!=0x0f){
            throw std::runtime_error("Unclassified opcode in passive timer dataflow");
        }
        if(writes&&rd){values[rd]=value;tags[rd]=result;}
        values[0]=0;tags[0]=false;
        return writes&&rd&&result;
    }
};
static bool retirement_timer_tainted=false;

struct Owner {
    uint64_t sequence=0,admission=0; uint32_t pc=0,instruction=0,a=0,b=0,length=0;
    bool valid=false,prepared=false,consumed=false,bank_seen=false;
    std::array<uint64_t,4> bank{};
};
struct Measure {
    std::array<uint64_t,event_count> totals{}; Counts ages,kills,retired_ops,cache_counts,memory_waits,permission_value_checks; std::array<uint64_t,10> buckets{};
    std::set<uint32_t> i_lines,d_lines;
    uint64_t cycles=0,retired=0,live_in=0,live_out=0;
    TraceSHA256 instructions,writeback,functional_writeback,untainted_writeback; uint64_t functional_retired=0,untainted_retired=0,timer_derived_writebacks=0;
    void retirement(uint32_t pc,uint32_t word,bool writes,uint32_t rd,uint32_t value) {
        ++retired; instructions.word(pc);instructions.word(word);
        writeback.word(pc);writeback.word(word);writeback.word(writes ? rd : 0);writeback.word(writes ? value : 0);
        if(!retirement_timer_tainted) {
            ++untainted_retired;untainted_writeback.word(pc);untainted_writeback.word(word);
            untainted_writeback.word(writes?rd:0);untainted_writeback.word(writes?value:0);
        } else ++timer_derived_writebacks;
        if(pc!=cycle_read_pc_low && pc!=cycle_read_pc_high) {
            ++functional_retired; functional_writeback.word(pc);functional_writeback.word(word);
            functional_writeback.word(writes ? rd : 0);functional_writeback.word(writes ? value : 0);
        }
        if((word&0xfe00007fU)==0x02000033U) {
            unsigned f=(word>>12)&7;
            ++retired_ops[f==0 ? "mul" : f<4 ? "other_multiply" : "div_rem"];
        }
    }
};
template<class Rows> void emit(std::ostream& out,const char* name,const Rows& rows) {
    out<<'"'<<name<<"\":{";bool first=true;
    for(auto& row:rows){if(!first)out<<',';first=false;out<<'"'<<row.first<<"\":"<<row.second;}out<<'}';
}
void emit_measure(std::ostream& out,Measure& m) {
    static const char* names[]={"ex_transfer","memory_blocked","load_use","muldiv","control_serialization","occupied_other","empty_recovery","empty_frontend","empty_barrier","empty_other"};
    uint64_t sum=0;Counts bins;for(unsigned i=0;i<10;++i){sum+=m.buckets[i];bins[names[i]]=m.buckets[i];}
    if(sum!=m.cycles)throw std::runtime_error("Exclusive occupancy partition does not close");
    if(m.totals[event_refill_error_beats])throw std::runtime_error("Refill error in verified public workload");
    if(m.live_in+m.totals[event_created]!=m.totals[event_actual_fast_consume]+m.totals[event_invalidated]+m.live_out)
        throw std::runtime_error("Token conservation does not close in declared window");
    uint64_t memory_wait_sum=0;for(const auto& v:m.memory_waits)memory_wait_sum+=v.second;
    if(memory_wait_sum!=m.buckets[1])throw std::runtime_error("Exclusive memory-wait attribution does not close");
    Counts counts;for(unsigned i=0;i<event_count;++i)counts[event_names[i]]=m.totals[i];
    counts["i_distinct_refill_lines"]=m.i_lines.size();counts["d_distinct_refill_lines"]=m.d_lines.size();
    counts["i_refill_bytes"]=4*m.totals[event_i_refill_beats];counts["d_refill_bytes"]=4*m.totals[event_d_refill_beats];
    out<<"{\"cycles\":"<<m.cycles<<",\"retired\":"<<m.retired<<",\"live_in\":"<<m.live_in<<",\"live_out\":"<<m.live_out;
    out<<",\"pc_instruction_sha256\":\""<<m.instructions.finish()<<"\",\"writeback_sha256\":\""<<m.writeback.finish()<<"\",";
    out<<"\"functional_retired\":"<<m.functional_retired<<",\"functional_writeback_sha256\":\""<<m.functional_writeback.finish()<<"\",";
    out<<"\"untainted_retired\":"<<m.untainted_retired<<",\"timer_derived_writebacks\":"<<m.timer_derived_writebacks<<",\"untainted_writeback_sha256\":\""<<m.untainted_writeback.finish()<<"\",";
    emit(out,"counts",counts);out<<',';emit(out,"permission_value_checks",m.permission_value_checks);out<<',';emit(out,"dcache",m.cache_counts);out<<',';emit(out,"memory_blocked_by_cause",m.memory_waits);out<<',';emit(out,"buckets",bins);out<<',';emit(out,"consume_age_histogram",m.ages);out<<',';emit(out,"invalidated_by_reason",m.kills);out<<',';emit(out,"retired_arithmetic",m.retired_ops);out<<'}';
}
int main(int argc,char** argv) {
    try {
        if(argc!=11)throw std::runtime_error("Expected output directory, cycle limit, start PC, stop PC, token and workload");
        std::string out=argv[1],limit=argv[2],workload=argv[6];
        if(limit.empty()||limit.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid cycle limit");
        uint64_t max_cycles=std::stoull(limit);if(!max_cycles||max_cycles>2500000000ULL)throw std::runtime_error("Cycle limit out of range");
        uint32_t start_pc=std::stoul(argv[3],nullptr,16),stop_pc=std::stoul(argv[4],nullptr,16);
        std::string kw=argv[5]; if(kw.empty()||kw.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid window");
        cycle_read_pc_low=std::stoul(argv[7],nullptr,16);cycle_read_pc_high=std::stoul(argv[8],nullptr,16);
        retired_read_pc_low=std::stoul(argv[9],nullptr,16);retired_read_pc_high=std::stoul(argv[10],nullptr,16);
        unsigned window=std::stoul(kw);if(window>255)throw std::runtime_error("Window out of range"); bool retained=window!=1;
        TraceSHA256 empty,abc;abc.put('a');abc.put('b');abc.put('c');
        if(empty.finish()!="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"||abc.finish()!="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")throw std::runtime_error("SHA256 test vectors failed");
        const bool trace_pipeline=std::getenv("UNSEEN_FULL_PIPELINE")!=nullptr;
        std::ofstream dependency(out+"/dependencies.csv"),pipeline(out+"/pipeline.csv");
        std::ofstream uart(out+"/uart.log",std::ios::binary),owners(out+"/owner_samples.csv"),sample(out+"/edge_samples.csv"),complete(out+"/owner_events.csv");
        if(!dependency||!uart||!owners||!sample||!complete)throw std::runtime_error("Cannot open output files");
        owners<<"edge,owner_seq,pc,instruction,length,rd,rs1,rs2,admitted_a,admitted_b,forwarded_a,forwarded_b,token_age,partial_ll,partial_lh,partial_hl,partial_hh,event\n";
        sample<<"edge,owner_seq,id_fire,prepare,token,mem_ready,load_use,barrier,mem_flush,common_exception,started,req_valid,req_ready,req_precomputed,req_operation,rsp_available,rsp_valid,rsp_ready,ex_fire,cancel\n";
        complete<<"edge,event,owner_seq,retirement_ordinal,frontier_retired,pc,instruction,length,admission,age,a,b,id_fire,id_valid,if_valid,mem_ready,ex_fire,fast,token,started,snapshot,architectural,write_rd,write_value\n";
        dependency<<"edge,event,data_id,transaction_id,master,address,length,beat,owner_seq,pc,instruction,admission,age,snapshot,architectural,mem_ready,d_state,d_wait_i,transaction_write\n";
        pipeline<<"edge,event,instruction_id,owner_seq,pc,instruction,data_id,i_id,transaction_id,address,flag\n";
        uint64_t data_seq=0,data_live=0,cache_data=0,transaction_seq=0,transaction_live=0,transaction_data=0,dependency_rows=0;
        uint64_t front_seq=0,pipe_if=0,pipe_id=0,pipe_ex=0,pipe_mem=0,pipe_wb=0,i_seq=0,i_live=0,i_cache=0,pipeline_rows=0;
        uint32_t i_address=0,i_pair_address=0;
        unsigned transaction_master=0,transaction_address=0,transaction_length=0,transaction_beat=0;bool transaction_write=false;
        uint64_t complete_rows=0,last_read_edge=0,backend_commands=0,backend_beats=0,backend_wait_edges=0; bool backend_read_live=false;
        TimerTaint timer_taint;uint64_t tainted_addresses=0,tainted_controls=0;
        std::ofstream diagnostic;const bool diagnose=std::getenv("PROSPECTIVE_WB_DIAGNOSTIC")!=nullptr;
        if(diagnose){diagnostic.open(out+"/unmasked_writebacks.csv");diagnostic<<"ordinal,pc,instruction,writes,rd,value,timer_derived,rs1_timer,rs2_timer,memory_address,address_timer,control_timer\n";}
        VerilatedContext context; context.commandArgs(argc, argv);
        auto model = std::make_unique<Vrv32_observed_soc>(&context);
        model->clk=0;model->rst_n=1;model->serial_rx=1;model->gpio_input=0;model->eval();model->rst_n=0;model->eval();
        auto tick=[&](){
            native_tick(context, *model);
        };
        for(int i=0;i<4;++i)tick();model->rst_n=1;model->eval();
        bool previous_tx=true,receiving=false,recovery=false,marker_live=false,marker_done=false;
        uint64_t deadline=0,owner_seq=0,first_cycles=0,first_retired=0,last_cycles=0,last_retired=0;
        uint64_t marker_first_cycle=0,marker_last_cycle=0,marker_first_ordinal=0,marker_last_ordinal=0;
        uint32_t sequence=0,sp=0,minimum_sp=65536;int bit=-1;unsigned byte=0,owner_rows=0,edge_rows=0;
        Owner owner,mem_owner,wb_owner;Measure snapshot,architectural;
        for(uint64_t cycle=1;cycle<=max_cycles;++cycle) {
            const uint64_t before_cycles=model->cycle_count,before_retired=model->retired_count;
            bool retiring=model->retire_event;uint32_t rpc=model->trace_pc,ri=model->trace_instruction;
            bool writes=model->wb_write;uint32_t rd=model->wb_rd,value=model->wb_data;
            if(retiring&&(!wb_owner.valid||wb_owner.pc!=rpc||wb_owner.instruction!=ri))throw std::runtime_error("Retirement lost pipeline instruction-instance ownership");
            retirement_timer_tainted=retiring?timer_taint.retirement(rpc,ri,writes,rd,value):false;
            bool ex_valid=P(ex_valid),ex_fire=P(ex_fire),id_fire=P(id_fire),prepare=P(prepare),token=P(token),started=P(started);
            bool req=P(req_valid)&&P(req_ready),rsp=P(rsp_valid)&&P(rsp_ready);
            bool fast=req&&P(req_precomputed)&&P(req_operation)==0&&P(rsp_valid)&&P(rsp_ready)&&ex_fire&&!P(cancel);
            std::array<uint64_t,4> bank={P(partial_ll),P(partial_lh),P(partial_hl),P(partial_hh)};
            if(ex_valid) {
                if(!owner.valid||owner.pc!=P(ex_pc)||owner.instruction!=P(ex_instruction)||owner.length!=P(ex_length))throw std::runtime_error("EX instruction instance ownership differs");
            }
            if(token) {
                if(!ex_valid||!owner.prepared||started)throw std::runtime_error("Token is detached from admitted EX owner");
                if(uint32_t(P(product))!=uint32_t(uint64_t(owner.a)*owner.b))throw std::runtime_error("Prepared low product differs from admitted operands");
                if(!owner.bank_seen){owner.bank=bank;owner.bank_seen=true;}
                if(retained&&bank!=owner.bank)throw std::runtime_error("Held token partial bank was overwritten");
                if(owner.a!=P(forwarded_a)||owner.b!=P(forwarded_b))throw std::runtime_error("Prepared owner operands changed");
            }
            if(fast) {
                if(!token||owner.consumed||uint32_t(P(rsp_data))!=uint32_t(uint64_t(owner.a)*owner.b))throw std::runtime_error("Immediate consume has stale or duplicate ownership");
                owner.consumed=true;
            }
            if(P(bank_hold)&&owner.valid&&owner.prepared) {
                if(!owner.bank_seen){owner.bank=bank;owner.bank_seen=true;}
                if(bank!=owner.bank)throw std::runtime_error("Expired permission changed protected bank");
                if(uint32_t(P(product))!=uint32_t(uint64_t(owner.a)*owner.b))throw std::runtime_error("Held value after expiry differs");
            }
            if(fast&&window&&before_cycles-owner.admission>window)throw std::runtime_error("Fast consume exceeded finite window");
            if(rsp&&P(cancel))throw std::runtime_error("Cancelled arithmetic response handshook");
            if(P(backend_command) && !P(backend_command_write)){backend_read_live=true;last_read_edge=0;++backend_commands;}
            if(P(backend_r_valid)&&!P(backend_r_ready))throw std::runtime_error("Configured native cache stalled a valid backing beat");
            if(P(backend_r_valid)&&P(backend_r_ready)){
                if(!backend_read_live)throw std::runtime_error("Read beat has no command owner");
                if(last_read_edge && before_cycles-last_read_edge<1)throw std::runtime_error("Backing read beats violate service interval");
                last_read_edge=before_cycles;++backend_beats;if(P(backend_r_last))backend_read_live=false;
            }
            if(P(backend_read_wait))++backend_wait_edges;
            const bool live_before=token;
            const uint64_t age=owner.valid ? before_cycles-owner.admission : 0;
            bool invalidated=token&&!fast;std::string reason;
            if(invalidated) {
                if(P(mem_flush))reason="mem_flush";
                else if(P(barrier))reason="frontend_barrier";
                else if(!retained&&!prepare)reason="pulse_expired";
                else if(P(common_exception))reason="common_exception";
                else if(req)reason="request_without_immediate_transfer";
                else if(ex_fire)reason="owner_departure";
                else if(!ex_valid)reason="owner_absent";
                else if(!P(ex_muldiv))reason="non_mul_owner";
                else if(started)reason="started";
                else if(window>1&&age>=window&&!prepare)reason="window_expired";
                else invalidated=false;
            }
            Owner next_owner=owner;
            Owner next_wb=P(mem_fire)&&!P(mem_fault) ? mem_owner : Owner{};
            Owner next_mem=mem_owner;
            if(P(mem_fire)||P(mem_flush))next_mem=Owner{};
            if(ex_fire)next_mem=owner;
            if(ex_fire||P(mem_flush)||P(barrier))next_owner=Owner{};
            if(id_fire){
                next_owner=Owner{};next_owner.valid=true;next_owner.sequence=++owner_seq;next_owner.admission=before_cycles;
                next_owner.pc=P(id_pc);next_owner.instruction=P(id_instruction);next_owner.length=P(id_length);
                next_owner.a=P(admit_a);next_owner.b=P(admit_b);next_owner.prepared=prepare;
            }
            if(retiring&&rpc==stop_pc){if(!marker_live||marker_done)throw std::runtime_error("Unexpected stop retirement marker");marker_last_cycle=before_cycles;marker_last_ordinal=before_retired;marker_live=false;marker_done=true;}
            if(retiring&&rpc==start_pc){if(marker_live||marker_done)throw std::runtime_error("Repeated start retirement marker");marker_first_cycle=before_cycles;marker_first_ordinal=before_retired;marker_live=true;architectural.live_in=live_before;}
            unsigned bucket;
            bool lack=P(current_missing)||P(following_missing);
            if(ex_fire)bucket=0;
            else if(ex_valid&&!P(mem_ready))bucket=1;
            else if(ex_valid&&P(load_use))bucket=2;
            else if(ex_valid&&P(ex_muldiv)&&!P(rsp_available))bucket=3;
            else if(ex_valid&&(P(serial_wait)||P(mem_flush)||P(barrier)))bucket=4;
            else if(ex_valid)bucket=5;
            else if(recovery||P(front_flush))bucket=6;
            else if(lack&&!P(id_valid)&&!P(if_valid))bucket=7;
            else if(P(barrier))bucket=8;
            else bucket=9;
            Counts permission_value_events;
            if(owner.valid&&owner.prepared&&!token&&!owner.consumed&&ex_valid&&!started) {
                ++permission_value_events["expired_permission_owner_edges"];
                const bool value_correct=uint32_t(P(product))==uint32_t(uint64_t(owner.a)*owner.b);
                ++permission_value_events[value_correct?"expired_low_product_correct":"expired_low_product_incorrect"];
                ++permission_value_events[bank==owner.bank?"expired_partial_bank_equal_admission":"expired_partial_bank_changed_admission"];
                const bool operands_equal=owner.a==P(forwarded_a)&&owner.b==P(forwarded_b);
                ++permission_value_events[operands_equal?"expired_operands_equal":"expired_operands_changed"];
                if(!operands_equal)throw std::runtime_error("Expired preparation owner operands changed");
            }
            const uint32_t d_hits_before=P(d_hit_counter),d_misses_before=P(d_miss_counter),d_wb_before=P(d_writeback_counter);
            Counts cache_events;
            cache_events["counter_hits"]=P(d_hit_event);cache_events["counter_misses"]=P(d_miss_event);cache_events["counter_writebacks"]=P(d_writeback_event);
            if(P(d_accept))++cache_events[P(d_accept_mmio)?"shadow_mmio_accept":P(d_accept_write)?"store_accept":"load_accept"];
            if(P(d_hit_event))++cache_events[P(d_request_mmio)?"shadow_mmio_hit":P(d_request_write)?"store_hit":"load_hit"];
            if(P(d_miss_event))++cache_events[P(d_request_mmio)?"shadow_mmio_miss":P(d_request_write)?"store_miss":"load_miss"];
            cache_events["writeback_commands"]=P(d_writeback_command);cache_events["writeback_beats"]=P(d_writeback_beat);
            cache_events["data_command_wait_behind_instruction"]=P(d_wait_behind_i);
            cache_events["instruction_command_wait_behind_data"]=P(i_wait_behind_d);
            std::string memory_wait_class;
            if(bucket==1) {
                if(P(memory_launch_wait))memory_wait_class="request_not_accepted";
                else if(P(d_active)&&P(d_active_mmio))memory_wait_class="mmio_response";
                else if(P(d_active)) {
                    const unsigned dstate=P(d_state);
                    if(dstate>=2&&dstate<=4)memory_wait_class="dirty_writeback";
                    else if(dstate==5)memory_wait_class="refill_command";
                    else if(dstate==6)memory_wait_class="refill_data";
                    else if(P(d_request_missed))memory_wait_class="post_refill_lookup_response";
                    else memory_wait_class="hit_lookup_response";
                } else memory_wait_class="other_owner_wait";
                ++cache_events["memory_blocked_"+memory_wait_class];
                if(P(d_wait_behind_i))++cache_events["memory_blocked_data_command_wait_behind_instruction"];
            }
            // Save all edge facts before advancing the actual vendor timing model.
            std::array<uint64_t,event_count> events{};
            events[event_created]=prepare;events[event_actual_fast_consume]=fast;events[event_invalidated]=invalidated;
            events[event_eligible_unconsumed]=invalidated;events[event_accepted_arithmetic_request]=req;events[event_valid_response_handshake]=rsp;
            events[event_token_hold_edges]=retained&&token&&!fast&&!invalidated;
            events[event_retained_wait_edges]=retained&&token&&!P(mem_ready)&&!invalidated;
            events[event_consume_after_wait]=fast&&age>1;events[event_memory_blocked_cycles]=ex_valid&&!P(mem_ready);
            events[event_ready_MUL_wait_cycles]=ex_valid&&P(mem_ready)&&P(ex_muldiv)&&!ex_fire;
            events[event_early_request_accept]=P(early_request)&&P(d_req_ready);
            events[event_data_request_accept]=P(d_req_valid)&&P(d_req_ready);events[event_data_response_accept]=P(d_rsp_valid)&&P(d_rsp_ready);
            events[event_memory_launch_wait]=P(memory_launch_wait);events[event_memory_response_wait]=P(memory_response_wait);
            events[event_frontend_missing]=lack;events[event_current_missing]=P(current_missing);events[event_following_missing]=P(following_missing);
            events[event_redirects]=P(front_flush);events[event_prefetch_operand_edges]=P(prefetch_operands);
            events[event_fast_owner_retirements]=retiring&&wb_owner.consumed;
            events[event_i_refill_commands]=P(i_fill_command);events[event_i_refill_beats]=P(i_fill_beat);events[event_i_refill_completions]=P(i_fill_complete);
            events[event_d_refill_commands]=P(d_fill_command);events[event_d_refill_beats]=P(d_fill_beat);events[event_d_refill_completions]=P(d_fill_complete);
            events[event_refill_error_beats]=P(i_fill_error)||P(d_fill_error);
            uint32_t ifill=P(i_fill_address),dfill=P(d_fill_address);
            std::ostringstream owner_line,edge_line;
            if((token||prepare||req||rsp)&&owner_rows<240){
                owner_line<<before_cycles<<','<<owner.sequence<<','<<std::hex<<owner.pc<<','<<owner.instruction<<std::dec<<','<<owner.length<<','<<((owner.instruction>>7)&31)<<','<<((owner.instruction>>15)&31)<<','<<((owner.instruction>>20)&31)<<','<<owner.a<<','<<owner.b<<','<<P(forwarded_a)<<','<<P(forwarded_b)<<','<<age;
                for(auto v:bank)owner_line<<','<<v;owner_line<<','<<(fast?"consume":invalidated?reason:prepare?"admit":"hold")<<'\n';
            }
            if((token||prepare)&&edge_rows<240)edge_line<<before_cycles<<','<<owner.sequence<<','<<id_fire<<','<<prepare<<','<<token<<','<<P(mem_ready)<<','<<P(load_use)<<','<<P(barrier)<<','<<P(mem_flush)<<','<<P(common_exception)<<','<<started<<','<<P(req_valid)<<','<<P(req_ready)<<','<<P(req_precomputed)<<','<<unsigned(P(req_operation))<<','<<P(rsp_available)<<','<<P(rsp_valid)<<','<<P(rsp_ready)<<','<<ex_fire<<','<<P(cancel)<<'\n';
            if(retiring&&marker_live){
                tainted_addresses+=timer_taint.address_tainted;tainted_controls+=timer_taint.control_tainted;
                if(diagnose)diagnostic<<before_retired<<','<<rpc<<','<<ri<<','<<writes<<','<<rd<<','<<value<<','<<retirement_timer_tainted<<','<<timer_taint.source_a<<','<<timer_taint.source_b<<','<<timer_taint.address<<','<<timer_taint.address_tainted<<','<<timer_taint.control_tainted<<'\n';
            }
            if(writes&&rd==2)sp=value;
            if(marker_live) {
                ++architectural.cycles;++architectural.buckets[bucket];
                for(const auto& v:permission_value_events)architectural.permission_value_checks[v.first]+=v.second;
                for(const auto& v:cache_events)architectural.cache_counts[v.first]+=v.second;
                if(bucket==1)++architectural.memory_waits[memory_wait_class];
                for(unsigned i=0;i<event_count;++i)architectural.totals[i]+=events[i];
                if(fast)++architectural.ages[std::to_string(age)];if(invalidated)++architectural.kills[reason];
                if(P(i_fill_command))architectural.i_lines.insert(ifill);if(P(d_fill_command))architectural.d_lines.insert(dfill);
                if(retiring)architectural.retirement(rpc,ri,writes,rd,value);
            }
            if(retiring&&rpc==stop_pc)architectural.live_out=live_before;
            // Complete pre-edge causal links. Byte-wide Verilator facts are cast to integers.
            auto dep=[&](const char* event,uint64_t did,uint64_t tid,const Owner& o){
                dependency<<before_cycles<<','<<event<<','<<did<<','<<tid<<','<<transaction_master<<','<<transaction_address<<','<<transaction_length<<','<<transaction_beat<<','<<(o.valid?o.sequence:0)<<','<<(o.valid?o.pc:0)<<','<<(o.valid?o.instruction:0)<<','<<(o.valid?o.admission:0)<<','<<(o.valid?before_cycles-o.admission:0)<<','<<(sequence==1)<<','<<marker_live<<','<<unsigned(P(mem_ready))<<','<<unsigned(P(d_state))<<','<<unsigned(P(d_wait_behind_i))<<','<<transaction_write<<'\n';++dependency_rows;
            };
            auto pipe=[&](const char* event,uint64_t iid,uint64_t oid,uint32_t pc,uint32_t word,uint64_t did,uint64_t irid,uint64_t tid,uint32_t address,unsigned flag){
                if(trace_pipeline) pipeline<<before_cycles<<','<<event<<','<<iid<<','<<oid<<','<<pc<<','<<word<<','<<did<<','<<irid<<','<<tid<<','<<address<<','<<flag<<'\n';++pipeline_rows;
            };
            const bool da=P(d_req_valid)&&P(d_req_ready),dr=P(d_rsp_valid)&&P(d_rsp_ready);
            if(dr){if(!data_live)throw std::runtime_error("Data response detached from request");dep("data_complete",data_live,0,Owner{});pipe("data_complete",pipe_mem,0,0,0,data_live,0,0,0,0);data_live=0;}
            if(da){if(data_live)throw std::runtime_error("Two accepted data owners overlap");data_live=++data_seq;dep("data_accept",data_live,0,Owner{});pipe("data_accept",P(data_from_ex)?pipe_ex:pipe_mem,0,0,0,data_live,0,0,P(d_address),unsigned(P(data_from_ex)));}
            if(P(d_accept)){if(!da)throw std::runtime_error("Cache admission lacks accepted data request");cache_data=data_live;}
            if(P(i_rsp_valid)&&P(i_rsp_ready)){if(!i_live)throw std::runtime_error("Instruction response detached");pipe("i_complete",0,0,0,0,0,i_live,0,i_address,unsigned(P(i_pair_valid)));i_live=0;}
            if(P(i_req_valid)&&P(i_req_ready)){if(i_live)throw std::runtime_error("Instruction request overlap");i_live=++i_seq;i_cache=i_live;i_address=P(i_address);i_pair_address=P(i_pair_address);pipe("i_accept",0,0,0,0,0,i_live,0,i_address,i_pair_address);}
            if(P(backend_command)){
                if(transaction_live)throw std::runtime_error("Backing commands overlap");
                transaction_live=++transaction_seq;transaction_master=unsigned(P(backend_owner));transaction_write=P(backend_command_write);
                transaction_address=P(backend_address);transaction_length=P(backend_length);transaction_beat=0;transaction_data=transaction_master?cache_data:0;
                dep(transaction_write?"write_command":"read_command",transaction_data,transaction_live,Owner{});
                pipe(transaction_write?"write_command":"read_command",0,0,0,0,transaction_data,transaction_master?0:i_cache,transaction_live,transaction_address,transaction_length);
            }
            if(prepare)dep("prepare",data_live?data_live:((!P(mem_ready)&&P(memory_launch_wait))?data_seq+1:0),0,next_owner);
            if(owner.valid&&owner.prepared&&!started&&!P(mem_ready)){
                uint64_t did=data_live?data_live:(P(memory_launch_wait)?data_seq+1:0);
                bool linked=transaction_live&&((transaction_master&&transaction_data==did)||(!transaction_master&&P(d_wait_behind_i)));
                dep("owner_block",did,linked?transaction_live:0,owner);
            }
            if(P(backend_r_valid)&&!P(backend_r_ready))throw std::runtime_error("Configured native cache stalled a valid backing beat");
            if(P(backend_r_valid)&&P(backend_r_ready)){
                if(!transaction_live||transaction_write)throw std::runtime_error("Read beat detached from transaction");
                dep("read_beat",transaction_data,transaction_live,Owner{});++transaction_beat;
                pipe("read_beat",0,0,0,0,transaction_data,transaction_master?0:i_cache,transaction_live,transaction_address,transaction_beat-1);
                if(P(backend_r_last)){if(transaction_beat!=transaction_length+1)throw std::runtime_error("Read length mismatch");dep("read_complete",transaction_data,transaction_live,Owner{});transaction_live=0;}
            }
            if(P(backend_b_valid)&&P(backend_b_ready)){if(!transaction_live||!transaction_write)throw std::runtime_error("Write completion detached");dep("write_complete",transaction_data,transaction_live,Owner{});pipe("write_complete",0,0,0,0,transaction_data,0,transaction_live,transaction_address,0);transaction_live=0;}
            if(retiring){if(!pipe_wb)throw std::runtime_error("Retirement lacks pipeline owner");pipe("retire",pipe_wb,0,rpc,ri,0,0,0,0,0);}
            if(P(mem_fire))pipe("mem_wb",pipe_mem,0,0,0,0,0,0,0,unsigned(P(mem_fault)));
            if(ex_fire)pipe("ex_mem",pipe_ex,owner.sequence,P(ex_pc),P(ex_instruction),0,0,0,0,unsigned(P(load_use)));
            if(id_fire)pipe("id_ex",pipe_id,next_owner.sequence,P(id_pc),P(id_instruction),0,0,0,0,unsigned(prepare));
            if(P(if_fire))pipe("if_id",pipe_if,0,P(if_pc),P(if_instruction),0,0,0,0,0);
            if(P(front_flush)||P(barrier))pipe("front_flush",P(mem_flush)?pipe_mem:P(ex_redirect)?pipe_ex:pipe_id,0,0,0,0,0,0,0,P(mem_flush)?1:P(ex_redirect)?2:3);
            uint64_t incoming_front=P(frontend_fire)?++front_seq:0;
            if(incoming_front)pipe("front_if",incoming_front,0,P(frontend_pc),P(frontend_instruction),0,0,0,0,unsigned(P(frontend_length)));
            uint64_t next_pipe_wb=P(mem_fire)&&!P(mem_fault)?pipe_mem:0;
            uint64_t next_pipe_mem=(P(mem_fire)||P(mem_flush))?0:pipe_mem;if(ex_fire)next_pipe_mem=pipe_ex;
            uint64_t next_pipe_ex=(ex_fire||P(mem_flush)||P(barrier))?0:pipe_ex;if(id_fire)next_pipe_ex=pipe_id;
            uint64_t next_pipe_id=id_fire?0:pipe_id;if(P(if_fire))next_pipe_id=pipe_if;
            uint64_t next_pipe_if=P(if_fire)?0:pipe_if;if(incoming_front)next_pipe_if=incoming_front;
            if(P(front_flush)||P(barrier)){next_pipe_if=0;next_pipe_id=0;if(P(mem_flush)||P(barrier)||P(ex_redirect))next_pipe_ex=0;}
            pipe_if=next_pipe_if;pipe_id=next_pipe_id;pipe_ex=next_pipe_ex;pipe_mem=next_pipe_mem;pipe_wb=next_pipe_wb;
            const bool architectural_edge=marker_live;
            const bool pre_id_valid=P(id_valid),pre_if_valid=P(if_valid),pre_mem_ready=P(mem_ready);
            const bool cancel_ex_event=(P(mem_flush)||P(barrier))&&!ex_fire;
            const bool cancel_mem_event=P(mem_flush)&&mem_owner.valid&&!(P(mem_fire)&&!P(mem_fault));
            tick();
            if(uint32_t(P(d_hit_counter)-d_hits_before)!=cache_events["counter_hits"] || uint32_t(P(d_miss_counter)-d_misses_before)!=cache_events["counter_misses"] || uint32_t(P(d_writeback_counter)-d_wb_before)!=cache_events["counter_writebacks"])throw std::runtime_error("Passive data-cache events differ from native counters");
            if(model->cycle_count!=before_cycles+1||model->retired_count!=before_retired+unsigned(retiring))throw std::runtime_error("Independent pre-edge counter disagreement");
            const bool live_after=P(token);
            if(retained&&token&&!fast&&!invalidated&&!live_after)throw std::runtime_error("Retained token lost during legal owner wait");
            if(!retained&&live_after!=prepare)throw std::runtime_error("Baseline one-cycle preparation pulse changed");
            if(model->status_code==0xeb000000U){minimum_sp=std::min(minimum_sp,sp);if(sp<49152||sp>65536)throw std::runtime_error("Reserved stack exceeded");}
            if(model->snapshot_sequence!=sequence){
                if(model->snapshot_sequence!=sequence+1||sequence>=2)throw std::runtime_error("Unexpected snapshot sequence");
                if(model->snapshot_cycles!=before_cycles||model->snapshot_retired!=before_retired)throw std::runtime_error("Native snapshot differs from pre-edge counters");
                sequence=model->snapshot_sequence;
                if(sequence==1){first_cycles=before_cycles;first_retired=before_retired;snapshot.live_in=live_before;}
                else{last_cycles=before_cycles;last_retired=before_retired;snapshot.live_out=live_before;}
            }
            auto owner_event=[&](const char* kind,const Owner& o,bool retirement){
                if(!o.valid||!o.prepared)return;
                complete<<before_cycles<<','<<kind<<','<<o.sequence<<',';
                if(retirement)complete<<before_retired;
                complete<<','<<before_retired<<','<<o.pc<<','<<o.instruction<<','<<o.length<<','<<o.admission<<','<<(before_cycles-o.admission)<<','<<o.a<<','<<o.b<<','<<id_fire<<','<<pre_id_valid<<','<<pre_if_valid<<','<<pre_mem_ready<<','<<ex_fire<<','<<fast<<','<<token<<','<<started<<','<<(sequence==1)<<','<<architectural_edge<<','<<(retirement&&writes?rd:0)<<','<<(retirement&&writes?value:0)<<'\n';
                ++complete_rows;
            };
            if(prepare)owner_event("admit",next_owner,false);
            if(owner.valid&&owner.prepared&&!started)owner_event("prepared_edge",owner,false);
            if(req)owner_event(fast?"fast_consume":"slow_request",owner,false);
            if(invalidated)owner_event(reason.c_str(),owner,false);
            if(cancel_ex_event)owner_event("cancel_ex",owner,false);
            if(cancel_mem_event)owner_event("cancel_mem",mem_owner,false);
            if(retiring)owner_event("retire",wb_owner,true);
            if(sequence==1){
                for(const auto& v:permission_value_events)snapshot.permission_value_checks[v.first]+=v.second;
                for(const auto& v:cache_events)snapshot.cache_counts[v.first]+=v.second;
                if(bucket==1)++snapshot.memory_waits[memory_wait_class];
                ++snapshot.cycles;++snapshot.buckets[bucket];for(unsigned i=0;i<event_count;++i)snapshot.totals[i]+=events[i];
                if(fast)++snapshot.ages[std::to_string(age)];if(invalidated)++snapshot.kills[reason];
                if(events[event_i_refill_commands])snapshot.i_lines.insert(ifill);if(events[event_d_refill_commands])snapshot.d_lines.insert(dfill);
                if(retiring)snapshot.retirement(rpc,ri,writes,rd,value);
                if(!owner_line.str().empty()){owners<<owner_line.str();++owner_rows;}
                if(!edge_line.str().empty()){sample<<edge_line.str();++edge_rows;}
            }
            owner=next_owner;mem_owner=next_mem;wb_owner=next_wb;if(ex_fire)recovery=false;if(events[event_redirects])recovery=true;
            bool tx=model->serial_tx;
            if(!receiving&&previous_tx&&!tx){receiving=true;deadline=cycle+16;bit=-1;byte=0;}
            if(receiving&&cycle==deadline){if(bit==-1){if(tx)throw std::runtime_error("Invalid UART start");}else if(bit<8)byte|=unsigned(tx)<<bit;else{if(!tx)throw std::runtime_error("Invalid UART stop");uart.put(char(byte));std::cout.put(char(byte));if(byte==10){uart.flush();std::cout.flush();}receiving=false;}++bit;deadline+=32;}previous_tx=tx;
            if(model->trap_valid||model->maintenance_fault)throw std::runtime_error("Unexpected trap or maintenance fault");
            if(model->status_code==0xeb000001U){
                if(sequence!=2||!marker_done||!snapshot.cycles||!snapshot.retired||receiving)throw std::runtime_error("Incomplete benchmark/window");
                if(snapshot.cycles!=last_cycles-first_cycles||snapshot.retired!=last_retired-first_retired)throw std::runtime_error("Snapshot interval accounting differs");
                if(architectural.cycles!=marker_last_cycle-marker_first_cycle||architectural.retired!=marker_last_ordinal-marker_first_ordinal)throw std::runtime_error("Architectural marker interval differs");
                std::ofstream result(out+"/observation.json");if(!result)throw std::runtime_error("Cannot write observation");
                result<<"{\"schema\":\"h3o-mechanism-observation/v1\",\"workload\":\""<<workload<<"\",\"retention_enabled\":"<<retained<<",\"window_length\":"<<window<<",\"minimum_sp\":"<<minimum_sp<<",\"marker_start_pc\":"<<start_pc<<",\"marker_stop_pc\":"<<stop_pc;
                result<<",\"architectural_timer_tainted_addresses\":"<<tainted_addresses<<",\"architectural_timer_tainted_controls\":"<<tainted_controls;
                result<<",\"dependency_trace_rows\":"<<dependency_rows<<",\"pipeline_trace_rows\":"<<pipeline_rows;
                result<<",\"complete_owner_trace_rows\":"<<complete_rows<<",\"backend_read_commands\":"<<backend_commands<<",\"backend_read_beats\":"<<backend_beats<<",\"backend_wait_edges\":"<<backend_wait_edges;
                result<<",\"snapshot_start_retired\":"<<first_retired<<",\"snapshot_stop_retired\":"<<last_retired<<",\"marker_start_retired\":"<<marker_first_ordinal<<",\"marker_stop_retired\":"<<marker_last_ordinal;
                result<<",\"start_snapshot_retirement_lag\":"<<int64_t(first_retired)-int64_t(marker_first_ordinal)<<",\"stop_snapshot_retirement_lag\":"<<int64_t(last_retired)-int64_t(marker_last_ordinal)<<",\"snapshot\":";emit_measure(result,snapshot);result<<",\"architectural\":";emit_measure(result,architectural);result<<"}\n";
                pipeline.flush();dependency.flush();result.flush();uart.flush();complete.flush();if(diagnose)diagnostic.flush();if(!dependency||!result||!uart||!complete)throw std::runtime_error("Output write failure");model->final();std::cout<<"PASS source-bound native mechanism cycles="<<snapshot.cycles<<" retired="<<snapshot.retired<<'\n';return 0;
            }
            if((model->status_code&0xffffff00U)==0xeb000000U&&(model->status_code&255)>=0xf0)throw std::runtime_error("Workload verification failed");
            if(Verilated::gotFinish())throw std::runtime_error("Unexpected RTL finish");
        }
        throw std::runtime_error("Cycle limit exceeded");
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
