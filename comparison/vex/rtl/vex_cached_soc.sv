`timescale 1ns/1ps
// SoC around the exact generated native cached VexRiscv core.
// Observer interface matches the historical vex_soc; the bus is native cached.
module vex_cached_soc #(
    parameter integer CLOCK_HZ=3200,BAUD=100,
    parameter [31:0] PERFORMANCE_CONFIG=32'h56455801,
    parameter INIT_FILE="firmware.hex"
)(
    input wire clk,rst_n,serial_rx,
    input wire [7:0] gpio_input,
    output wire serial_tx,
    output wire [7:0] gpio_output,
    output wire [31:0] status_code,trace_pc,trace_instruction,trap_cause,
    output wire maintenance_fault,trace_valid,trap_valid,
    output wire [63:0] cycle_count,retired_count,
    output wire [31:0] snapshot_sequence,
    output wire [63:0] snapshot_cycles,snapshot_retired,
    output wire retire_event,wb_write,
    output wire [4:0] wb_rd,
    output wire [31:0] wb_data,
    output wire [63:0] rvfi_order,
    output wire [31:0] rvfi_pc_next,rvfi_rs1_data,rvfi_rs2_data,
    output wire [4:0] rvfi_rs1,rvfi_rs2,
    output wire [31:0] rvfi_mem_addr,rvfi_mem_rdata,rvfi_mem_wdata,
    output wire [3:0] rvfi_mem_rmask,rvfi_mem_wmask,
    output wire owner_correction,owner_taken,owner_control,
    output wire [63:0] owner_execute_cycle,owner_resolve_cycle,
    output wire [1:0] owner_younger_valid,
    output wire [31:0] i_refill_count,d_refill_count,store_count
);
    wire ib_valid,ib_ready,ib_rsp_valid,ib_rsp_error;
    wire i_cache_hit_accepted,d_cache_hit_accepted,i_cache_stall_cycle,d_cache_stall_cycle;
    
    wire [31:0] ib_address,ib_rsp_data;
    wire [2:0] ib_size;
    wire db_valid,db_ready,db_write,db_uncached,db_last,db_rsp_valid,db_rsp_last,db_rsp_error;
    wire [31:0] db_address,db_data,db_rsp_data;
    wire [3:0] db_mask;
    wire [2:0] db_size;
    wire bus_fault,rvfi_trap,irq_external,irq_software,irq_timer;
    wire [63:0] timer_value;
    wire debug_reset_out;
    assign maintenance_fault=bus_fault;
    assign trap_valid=trace_valid && rvfi_trap;
    assign trap_cause=0; // RVFI reports a trap flag; firmware logs mcause.
    assign retire_event=trace_valid && !rvfi_trap;
    assign wb_write=retire_event && (wb_rd!=0);
    
    VexRiscv core(
        .clk(clk),.reset(!rst_n || debug_reset_out),.debugReset(!rst_n),
        .utime(timer_value),
        .machineCycleCount(cycle_count),.machineRetiredCount(retired_count),
        .iCacheHitAccepted(i_cache_hit_accepted),.dCacheHitAccepted(d_cache_hit_accepted),
        .iCacheStallCycle(i_cache_stall_cycle),.dCacheStallCycle(d_cache_stall_cycle),
        .ownerCorrection(owner_correction),.ownerTaken(owner_taken),.ownerControl(owner_control),
        .ownerExecuteCycle(owner_execute_cycle),.ownerResolveCycle(owner_resolve_cycle),
        .ownerYoungerValid(owner_younger_valid),
        .timerInterrupt(irq_timer),.externalInterrupt(irq_external),.softwareInterrupt(irq_software),
        .debug_bus_cmd_valid(1'b0),.debug_bus_cmd_ready(),.debug_bus_cmd_payload_wr(1'b0),
        .debug_bus_cmd_payload_address(8'b0),.debug_bus_cmd_payload_data(32'b0),
        .debug_bus_rsp_data(),.debug_resetOut(debug_reset_out),
        .iBus_cmd_valid(ib_valid),.iBus_cmd_ready(ib_ready),.iBus_cmd_payload_address(ib_address),.iBus_cmd_payload_size(ib_size),
        .iBus_rsp_valid(ib_rsp_valid),.iBus_rsp_payload_error(ib_rsp_error),.iBus_rsp_payload_data(ib_rsp_data),
        .dBus_cmd_valid(db_valid),.dBus_cmd_ready(db_ready),.dBus_cmd_payload_wr(db_write),.dBus_cmd_payload_uncached(db_uncached),
        .dBus_cmd_payload_mask(db_mask),.dBus_cmd_payload_address(db_address),.dBus_cmd_payload_data(db_data),
        .dBus_cmd_payload_size(db_size),.dBus_cmd_payload_last(db_last),
        .dBus_rsp_valid(db_rsp_valid),.dBus_rsp_payload_last(db_rsp_last),.dBus_rsp_payload_error(db_rsp_error),.dBus_rsp_payload_data(db_rsp_data),
        .rvfi_valid(trace_valid),.rvfi_order(rvfi_order),.rvfi_insn(trace_instruction),.rvfi_trap(rvfi_trap),
        .rvfi_halt(),.rvfi_intr(),.rvfi_mode(),.rvfi_ixl(),
        .rvfi_rs1_addr(rvfi_rs1),.rvfi_rs2_addr(rvfi_rs2),.rvfi_rs1_rdata(rvfi_rs1_data),.rvfi_rs2_rdata(rvfi_rs2_data),
        .rvfi_rd_addr(wb_rd),.rvfi_rd_wdata(wb_data),.rvfi_pc_rdata(trace_pc),.rvfi_pc_wdata(rvfi_pc_next),
        .rvfi_mem_addr(rvfi_mem_addr),.rvfi_mem_rmask(rvfi_mem_rmask),.rvfi_mem_wmask(rvfi_mem_wmask),
        .rvfi_mem_rdata(rvfi_mem_rdata),.rvfi_mem_wdata(rvfi_mem_wdata)
    );
    vex_cached_fabric #(.CLOCK_HZ(CLOCK_HZ),.BAUD(BAUD),.PERFORMANCE_CONFIG(PERFORMANCE_CONFIG),.MONITOR_ENABLE(1),.INIT_FILE(INIT_FILE)) fabric(
        .clk(clk),.rst_n(rst_n),.serial_rx(serial_rx),.gpio_input(gpio_input),.serial_tx(serial_tx),
        .gpio_output(gpio_output),.status_code(status_code),
        .ib_valid(ib_valid),.ib_ready(ib_ready),.ib_address(ib_address),.ib_size(ib_size),
        .ib_rsp_valid(ib_rsp_valid),.ib_rsp_error(ib_rsp_error),.ib_rsp_data(ib_rsp_data),
        .db_valid(db_valid),.db_ready(db_ready),.db_write(db_write),.db_uncached(db_uncached),.db_last(db_last),
        .db_address(db_address),.db_data(db_data),.db_mask(db_mask),.db_size(db_size),
        .db_rsp_valid(db_rsp_valid),.db_rsp_last(db_rsp_last),.db_rsp_error(db_rsp_error),.db_rsp_data(db_rsp_data),
        .i_cache_hit_accepted(i_cache_hit_accepted),.d_cache_hit_accepted(d_cache_hit_accepted),
        .i_cache_stall_cycle(i_cache_stall_cycle),.d_cache_stall_cycle(d_cache_stall_cycle),
        .cycle_count(cycle_count),.retired_count(retired_count),.snapshot_cycles(snapshot_cycles),.snapshot_retired(snapshot_retired),
        .snapshot_sequence(snapshot_sequence),.bus_fault(bus_fault),
        .irq_external(irq_external),.irq_software(irq_software),.irq_timer(irq_timer),.timer_value(timer_value),
        .i_refill_count(i_refill_count),.d_refill_count(d_refill_count),.store_count(store_count)
    );
endmodule
