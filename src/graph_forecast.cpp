#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <zlib.h>

// Original-graph forecast only; the orchestration pins original input hashes.
using U = uint64_t;
constexpr U missing = std::numeric_limits<U>::max();
enum Kind { DATA_ACCEPT, DATA_COMPLETE, EX_MEM, FRONT_FLUSH, FRONT_IF,
            I_ACCEPT, I_COMPLETE, ID_EX, IF_ID, MEM_WB, READ_BEAT,
            READ_COMMAND, RETIRE, WRITE_COMMAND, WRITE_COMPLETE, KIND_COUNT };
constexpr const char* names[] = {"data_accept", "data_complete", "ex_mem",
    "front_flush", "front_if", "i_accept", "i_complete", "id_ex", "if_id",
    "mem_wb", "read_beat", "read_command", "retire", "write_command", "write_complete"};
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
U number(std::string_view text) {
    U value = 0;
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    require(parsed.ec == std::errc() && parsed.ptr == text.data() + text.size(),
            "Invalid integer");
    return value;
}
U little(const unsigned char* bytes) {
    U value = 0;
    for (unsigned i = 0; i < 8; ++i) value |= U(bytes[i]) << (8*i);
    return value;
}
struct Times { U original, absolute, relative, legacy; };
struct Owner { Times admission{}, consume{}, cancel{}; U expected_admission=missing,expected_consume=missing,expected_cancel=missing; bool admitted=false, consumed=false, canceled=false; };
struct Command { U address, length, accepted=0, beats=0; bool write; };

int main(int argc, char** argv) {
    try {
        require(argc == 13,
          "Usage: original_graph.gz original_commands.csv original_cancels.csv owners.csv summary.json node_count transported_event_mask none|old|new original_prepared_endpoints.csv original_pipeline_inventory.csv");
        const U expected_nodes = number(argv[6]);
        const U mask = number(argv[7]);
        const std::string schedule=argv[8];
        const bool repair=number(argv[11])!=0,transport=number(argv[12])!=0;
        require(schedule=="none" || schedule=="old" || schedule=="new" || schedule=="bit89","Unknown service intervention");
        require((mask >> KIND_COUNT) == 0, "Unknown transported event kind");
        std::unordered_map<U,Command> commands;
        std::unordered_map<U,std::vector<U>> canceled_by_edge;
        std::unordered_map<U,Owner> owners;
        std::unordered_map<U,U> data_addresses;
        std::unordered_set<U> serialized_instructions;
        std::ifstream inventory_in(argv[10]);
        require(bool(inventory_in),"Original sparse pipeline inventory absent");
        std::string line;
        require(bool(std::getline(inventory_in,line)) && line=="kind,id,value","Sparse pipeline inventory schema differs");
        while(std::getline(inventory_in,line)) {
            if(!line.empty() && line.back()=='\r') line.pop_back();
            auto first=line.find(','),second=line.find(',',first+1);
            require(first!=std::string::npos && second!=std::string::npos,"Sparse pipeline inventory truncated");
            const std::string_view row(line);
            const auto kind=row.substr(0,first);
            const U id=number(row.substr(first+1,second-first-1)),value=number(row.substr(second+1));
            if(kind=="data") require(data_addresses.emplace(id,value).second,"Original accepted data ID repeats");
            else if(kind=="serial") require(serialized_instructions.emplace(id).second,"Original serialized instruction ID repeats");
            else require(kind=="muldiv","Unknown original inventory kind");
        }
        std::ifstream owner_in(argv[9]);
        require(bool(owner_in),"Original preparation inventory absent");
        require(bool(std::getline(owner_in,line)),"Original preparation inventory header absent");
        if(!line.empty() && line.back()=='\r') line.pop_back();
        require(line=="owner_seq,admission_original,admission_predicted,consume_original,consume_predicted,cancel_original,cancel_predicted","Original prepared-endpoint schema differs");
        while(std::getline(owner_in,line)) {
            if(!line.empty() && line.back()=='\r') line.pop_back();
            std::array<U,7> fields{}; size_t start=0;
            for(unsigned i=0;i<7;++i) {
                const auto end=i==6?line.size():line.find(',',start);
                require(end!=std::string::npos,"Original preparation inventory truncated");
                fields[i]=number(std::string_view(line).substr(start,end-start)); start=end+1;
            }
            Owner owner; owner.expected_admission=fields[1]; owner.expected_consume=fields[3]; owner.expected_cancel=fields[5];
            require(owners.emplace(fields[0],owner).second,"Original prepared owner repeats");
        }
        std::ifstream commands_in(argv[2]);
        require(bool(commands_in), "Original command manifest absent");
        require(bool(std::getline(commands_in,line)), "Command manifest header absent");
        require(line == "transaction_id,address,length,write", "Command manifest schema differs");
        while (std::getline(commands_in,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            std::array<U,4> fields{}; size_t start=0;
            for (unsigned i=0;i<4;++i) {
                const auto end=i==3?line.size():line.find(',',start);
                require(end!=std::string::npos,"Command manifest truncated");
                fields[i]=number(std::string_view(line).substr(start,end-start)); start=end+1;
            }
            require(fields[3]<=1,"Invalid command direction");
            require(commands.emplace(fields[0],Command{fields[1],fields[2],0,0,bool(fields[3])}).second,
                    "Duplicate original transaction");
        }
        std::ifstream cancel_in(argv[3]);
        require(bool(cancel_in),"Original cancellation manifest absent");
        require(bool(std::getline(cancel_in,line)),"Cancellation header absent");
        while (std::getline(cancel_in,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            auto comma=line.find(','); require(comma!=std::string::npos,"Cancellation row truncated");
            canceled_by_edge[number(std::string_view(line).substr(comma+1))].push_back(
                number(std::string_view(line).substr(0,comma)));
        }
        gzFile input=gzopen(argv[1],"rb");
        require(input!=nullptr,"Original graph absent");
        require(gzbuffer(input,1<<20)==0,"Graph decompression buffer failed");
        std::vector<Times> times; times.reserve(expected_nodes+1); times.push_back({0,0,0,0});
        std::vector<unsigned char> kinds; kinds.reserve(expected_nodes+1); kinds.push_back(255);
        std::array<U,KIND_COUNT> counts{},residual_roots{},transported_roots{},residual_total{};
        U edges=0,selected_first=0,selected_later=0,read_transactions=0,write_transactions=0,redirect_exclusion_edges=0,mmio_response_edges=0,serial_drain_edges=0,uart_absolute_roots=0;
        Times last_retire{}; bool have_retire=false;
        std::array<unsigned char,66> header{};
        std::array<unsigned char,24*255> dependencies{};
        while (true) {
            int got=gzread(input,header.data(),header.size());
            require(got>=0,"Original graph decompression failed");
            if (got==0) break;
            require(got==int(header.size()),"Truncated compact graph header");
            const U id=little(header.data()),original=little(header.data()+8),legacy=little(header.data()+16);
            const unsigned kind=header[24],degree=header[25];
            const U release=little(header.data()+26),instruction=little(header.data()+34),owner=little(header.data()+42),transaction=little(header.data()+50),request=little(header.data()+58);
            require(id==times.size() && id<=expected_nodes && kind<KIND_COUNT,"Compact graph node identity differs");
            const auto bytes=24*degree;
            require(bytes==0 || gzread(input,dependencies.data(),bytes)==int(bytes),"Truncated compact graph dependencies");
            U delta=0;
            if (kind==READ_COMMAND || kind==WRITE_COMMAND) {
                auto it=commands.find(transaction); require(it!=commands.end(),"Graph command lacks original manifest");
                auto& cmd=it->second; require(cmd.accepted==0,"Graph transaction reused");
                require(cmd.write==(kind==WRITE_COMMAND),"Graph direction differs from original manifest");
                cmd.accepted=id;
                if (cmd.write) ++write_transactions; else ++read_transactions;
            } else if (kind==READ_BEAT) {
                auto it=commands.find(transaction); require(it!=commands.end(),"Graph read beat lacks command");
                auto& cmd=it->second; require(cmd.accepted && !cmd.write && cmd.beats<=cmd.length,"Original read beat invalid");
                require(degree==1,"Read-beat dependency schema differs");
                const U lag=little(dependencies.data()+8);
                if (cmd.beats==0) {
                    require(little(dependencies.data())==cmd.accepted && lag==2,"Original first-beat dependency differs");
                    delta=schedule=="bit89"?((cmd.address&256)?2:0):schedule=="new"?((cmd.address&64)?2:0):schedule=="old"?bool(cmd.address&32):0;
                    selected_first+=bool(delta);
                } else {
                    require(lag==1,"Original later-beat dependency differs");
                    delta=schedule=="bit89"?(bool(cmd.address&512)^bool(cmd.address&256)):schedule=="new"?bool(cmd.address&128):0; selected_later+=bool(delta);
                }
                ++cmd.beats;
            }
            U causal=0,refined_causal=0,absolute=0,relative=0,legacy_replayed=0;
            for (unsigned i=0;i<degree;++i) {
                const auto* d=dependencies.data()+24*i;
                const U parent=little(d),lag=little(d+8),old_delta=little(d+16);
                require(parent>0 && parent<id,"Compact graph is not topological");
                const auto& p=times[parent];
                require(p.original+lag<=original,"Original minimum dependency exceeds event time");
                causal=std::max(causal,p.original+lag);
                U refined_lag=lag;
                // RTL out_valid excludes redirect_valid on the same edge.
                if(repair && kind==FRONT_IF && kinds[parent]==FRONT_FLUSH && lag==0) {
                    refined_lag=1; ++redirect_exclusion_edges;
                }
                if(repair && kind==DATA_COMPLETE && kinds[parent]==DATA_ACCEPT && lag==1) {
                    auto address=data_addresses.find(request); require(address!=data_addresses.end(),"Data response lacks original accepted address");
                    if(address->second>>28) { refined_lag=2; ++mmio_response_edges; }
                }
                require(p.original+refined_lag<=original,"Source redirect exclusion exceeds original event time");
                refined_causal=std::max(refined_causal,p.original+refined_lag);
                absolute=std::max(absolute,p.absolute+lag+delta);
                relative=std::max(relative,p.relative+refined_lag+delta);
                legacy_replayed=std::max(legacy_replayed,p.legacy+lag+old_delta);
            }
            if(repair && kind==EX_MEM && serialized_instructions.contains(instruction) && have_retire) {
                require(last_retire.original+1<=original,"Source serialized retirement drain exceeds original transfer");
                refined_causal=std::max(refined_causal,last_retire.original+1);
                relative=std::max(relative,last_retire.relative+1); ++serial_drain_edges;
            }
            const U gap=original-causal;
            const U refined_gap=original-refined_causal;
            require(release==(gap?original:missing),"Original release encoding differs");
            if (release!=missing) {
                absolute=std::max(absolute,release);
                legacy_replayed=std::max(legacy_replayed,release);
                ++residual_roots[kind]; residual_total[kind]+=gap;
            }
            require(legacy_replayed==legacy,"Independent replay differs from frozen legacy graph");
            bool uart_deadline=false;
            if(kind==DATA_COMPLETE) {
                auto address=data_addresses.find(request); require(address!=data_addresses.end(),"Completed data request lacks sparse inventory");
                uart_deadline=address->second==0x10000100U;
            }
            if (transport && degree && !uart_deadline && (mask&(U(1)<<kind))) {
                relative+=refined_gap;
                if (refined_gap) ++transported_roots[kind];
            } else if (refined_gap) {
                relative=std::max(relative,original);
                if(uart_deadline && refined_gap) ++uart_absolute_roots;
            }
            // Zero-intervention relative reconstruction is exactly causal+gap.
            require(causal+gap==original,"Original residual reconstruction differs");
            if(schedule=="none") require(absolute==original && relative==original,"Zero-intervention event reconstruction differs");
            Times current{original,absolute,relative,legacy}; times.push_back(current); kinds.push_back(kind); ++counts[kind]; edges+=degree;
            if(kind==RETIRE) { last_retire=current; have_retire=true; }
            if (kind==ID_EX && owner) {
                auto it=owners.find(owner);
                if(it!=owners.end()) {
                    auto& o=it->second; require(!o.admitted && original==o.expected_admission,"Prepared owner graph admission differs from original flag inventory");
                    o.admission=current; o.admitted=true;
                }
            } else if (kind==EX_MEM && owner) {
                auto it=owners.find(owner);
                if (it!=owners.end()) { require(!it->second.consumed && original==it->second.expected_consume,"Prepared owner graph consume differs from original endpoint"); it->second.consume=current; it->second.consumed=true; }
            } else if (kind==FRONT_FLUSH) {
                auto it=canceled_by_edge.find(original);
                if (it!=canceled_by_edge.end()) for (U sequence:it->second) {
                    auto o=owners.find(sequence); require(o!=owners.end() && !o->second.canceled && original==o->second.expected_cancel,"Canceled graph owner absent, repeated or differently timed");
                    o->second.cancel=current; o->second.canceled=true;
                }
            }
            if (id%5000000==0) std::cout<<"GRAPH_FORECAST_PROGRESS nodes="<<id<<'\n'<<std::flush;
        }
        int error=0; gzerror(input,&error); require(error==Z_OK || error==Z_STREAM_END,"Original graph decompression error");
        require(gzclose(input)==Z_OK,"Original graph close failed");
        require(times.size()==expected_nodes+1,"Original graph node count differs");
        U terminal=0;
        for (auto& [id,c]:commands) {
            require(c.accepted>0,"Original command omitted by graph");
            if (!c.write && c.beats!=c.length+1) ++terminal;
        }
        std::ofstream output(argv[4]); require(bool(output),"Owner forecast output open failed");
        output<<"owner_seq,admission_original,admission_absolute,admission_relative,consume_original,consume_absolute,consume_relative,cancel_original,cancel_absolute,cancel_relative\n";
        std::vector<U> sequences; for(auto& [sequence,o]:owners) sequences.push_back(sequence); std::sort(sequences.begin(),sequences.end());
        for (U sequence:sequences) {
            const auto& o=owners.at(sequence);
            require(o.admitted && (o.consumed || o.canceled),"Original owner lacks graph endpoint");
            output<<sequence<<','<<o.admission.original<<','<<o.admission.absolute<<','<<o.admission.relative<<','
                  <<(o.consumed?o.consume.original:missing)<<','<<(o.consumed?o.consume.absolute:missing)<<','<<(o.consumed?o.consume.relative:missing)<<','
                  <<(o.canceled?o.cancel.original:missing)<<','<<(o.canceled?o.cancel.absolute:missing)<<','<<(o.canceled?o.cancel.relative:missing)<<'\n';
        }
        output.flush(); require(bool(output),"Owner forecast output write failed");
        std::ofstream summary(argv[5]); require(bool(summary),"Forecast summary open failed");
        summary<<"{\"schema\":\"original-graph-forecast-extension/v1\",\"nodes\":"<<expected_nodes
          <<",\"edges\":"<<edges<<",\"owners\":"<<owners.size()<<",\"transported_event_mask\":"<<mask
          <<",\"schedule\":\""<<schedule<<'\"'
          <<",\"legacy_graph_replay_exact\":true,\"original_residual_reconstruction_exact\":true"
          <<",\"selected_first_beats\":"<<selected_first<<",\"selected_later_beats\":"<<selected_later
          <<",\"read_transactions\":"<<read_transactions<<",\"write_transactions\":"<<write_transactions
          <<",\"source_redirect_exclusion_edges\":"<<redirect_exclusion_edges
          <<",\"source_registered_mmio_response_edges\":"<<mmio_response_edges
          <<",\"source_serialized_retirement_drain_edges\":"<<serial_drain_edges
          <<",\"uart_absolute_residual_roots\":"<<uart_absolute_roots
          <<",\"incomplete_terminal_read_transactions\":"<<terminal<<",\"event_kinds\":{";
        bool first=true;
        for(unsigned i=0;i<KIND_COUNT;++i) if(counts[i]) {
            if(!first) summary<<',';
            first=false;
            summary<<'\"'<<names[i]<<"\":{\"nodes\":"<<counts[i]<<",\"residual_roots\":"<<residual_roots[i]
              <<",\"residual_edge_total\":"<<residual_total[i]<<",\"transported_residual_roots\":"<<transported_roots[i]<<'}';
        }
        summary<<"}}\n"; summary.flush(); require(bool(summary),"Forecast summary write failed");
        std::cout<<"GRAPH_FORECAST_COMPLETE nodes="<<expected_nodes<<" owners="<<owners.size()<<'\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<"ERROR "<<error.what()<<'\n'; return 1;
    }
}
