`timescale 1ns/1ps
// Native Vex cache memory buses: one Stream command, unstallable Flow replies.
// One shared synchronous 64 KiB backing RAM; no second cache hierarchy.
// DCache stores are write-through, with no ordinary write response (no exclusive).
module vex_cached_fabric #(
    parameter integer CLOCK_HZ=3200,
    parameter integer BAUD=100,
    parameter integer ADDRESS_BITS=14,
    parameter integer MONITOR_ENABLE=1,
    parameter [31:0] PERFORMANCE_CONFIG=32'h56455801,
    parameter INIT_FILE="firmware.hex"
)(
    input wire clk,rst_n,serial_rx,
    input wire [7:0] gpio_input,
    output wire serial_tx,
    output wire [7:0] gpio_output,
    output wire [31:0] status_code,
    input wire ib_valid,
    output wire ib_ready,
    input wire [31:0] ib_address,
    input wire [2:0] ib_size,
    output wire ib_rsp_valid,ib_rsp_error,
    output wire [31:0] ib_rsp_data,
    input wire db_valid,db_write,db_uncached,db_last,
    output wire db_ready,
    input wire [31:0] db_address,db_data,
    input wire [3:0] db_mask,
    input wire [2:0] db_size,
    output wire db_rsp_valid,db_rsp_last,db_rsp_error,
    output wire [31:0] db_rsp_data,
    input wire [63:0] cycle_count,retired_count,
    input wire i_cache_hit_accepted,d_cache_hit_accepted,i_cache_stall_cycle,d_cache_stall_cycle,
    output wire [63:0] snapshot_cycles,snapshot_retired,
    output wire [31:0] snapshot_sequence,
    output reg bus_fault,
    output wire irq_external,irq_software,irq_timer,
    output wire [63:0] timer_value,
    output reg [31:0] i_refill_count,d_refill_count,store_count
);
    localparam integer MEMORY_WORDS=1<<ADDRESS_BITS;
    localparam [32:0] MEMORY_BYTES=33'd4<<ADDRESS_BITS;
    reg [31:0] memory[0:MEMORY_WORDS-1];
    reg [31:0] ram_read_data;
    reg read_active,read_owner_d,read_error,prefer_d;
    reg [ADDRESS_BITS-1:0] read_word;
    reg [5:0] read_remaining;
    reg response_valid,response_owner_d,response_last,response_error;
    reg mmio_busy,mmio_write;
    reg [31:0] i_hit_count,d_hit_count,i_stall_count,d_stall_count;
    integer init_index;

    wire mmio_req_valid,mmio_req_ready,mmio_rsp_valid,mmio_rsp_error;
    wire [31:0] mmio_rsp_data;
    wire db_mmio=db_address>=32'h10000000 && db_uncached && db_size<=2 && db_last;
    // Preserve D response ordering: an MMIO read may overlap an I refill,
    // but may not bypass an earlier D RAM response.
    wire d_read_pending=(read_active && read_owner_d) || (response_valid && response_owner_d);
    wire d_mmio_available=!mmio_busy && !d_read_pending;
    wire d_ram_candidate=db_valid && !db_mmio && !mmio_busy;
    wire choose_d=d_ram_candidate && (!ib_valid || prefer_d);
    wire ram_available=rst_n && !bus_fault && !read_active;
    wire i_fire=ib_valid && ib_ready;
    wire d_fire=db_valid && db_ready;
    wire d_ram_fire=d_fire && !db_mmio;
    wire d_ram_write=d_ram_fire && db_write;
    wire accepted_read=i_fire || (d_ram_fire && !db_write);
    wire [31:0] command_address=i_fire ? ib_address : db_address;
    wire [2:0] command_size=i_fire ? ib_size : db_size;
    wire [5:0] command_beats=command_size<=2 ? 6'd1 : (6'd1<<(command_size-2));
    wire [32:0] command_end={1'b0,command_address[31:2],2'b00}+({27'b0,command_beats}<<2);
    wire [31:0] alignment_mask=(32'd1<<command_size)-1;
    wire read_command_error=command_size>5 || command_end>MEMORY_BYTES ||
        (i_fire && command_address[1:0]!=0) ||
        (command_size>2 && (command_address & alignment_mask)!=0) ||
        (!i_fire && !db_last);
    wire write_command_error=db_size>2 || !db_last ||
        {1'b0,db_address[31:2],2'b00}>=MEMORY_BYTES;
    wire memory_read=rst_n && read_active && !read_error;
    wire memory_write=d_ram_write && !write_command_error;
    wire [ADDRESS_BITS-1:0] memory_word=memory_write ? db_address[ADDRESS_BITS+1:2] : read_word;
    wire mmio_fire=mmio_req_valid && mmio_req_ready;
    wire mmio_response=mmio_busy && mmio_rsp_valid;

    assign ib_ready=ram_available && !choose_d;
    assign db_ready=rst_n && !bus_fault && (db_mmio ?
        (d_mmio_available && mmio_req_ready) : (ram_available && choose_d));
    assign mmio_req_valid=rst_n && !bus_fault && db_valid && db_mmio && d_mmio_available;
    assign ib_rsp_valid=rst_n && response_valid && !response_owner_d;
    assign ib_rsp_error=response_error;
    assign ib_rsp_data=response_error ? 32'b0 : ram_read_data;
    assign db_rsp_valid=rst_n && ((response_valid && response_owner_d) || (mmio_response && !mmio_write));
    assign db_rsp_last=mmio_response && !mmio_write ? 1'b1 : response_last;
    assign db_rsp_error=mmio_response && !mmio_write ? mmio_rsp_error : response_error;
    assign db_rsp_data=mmio_response && !mmio_write ? mmio_rsp_data : (response_error ? 32'b0 : ram_read_data);

    initial begin
        if(INIT_FILE!="") $readmemh(INIT_FILE,memory);
        else for(init_index=0;init_index<MEMORY_WORDS;init_index=init_index+1) memory[init_index]=0;
    end
    // No reset on storage/read data: infer vendor synchronous DRM and byte WEs.
    // Arbitration prevents simultaneous reads and writes at this single port.
    always @(posedge clk) begin
        if(memory_write) begin
            if(db_mask[0]) memory[memory_word][7:0]<=db_data[7:0];
            if(db_mask[1]) memory[memory_word][15:8]<=db_data[15:8];
            if(db_mask[2]) memory[memory_word][23:16]<=db_data[23:16];
            if(db_mask[3]) memory[memory_word][31:24]<=db_data[31:24];
        end else if(memory_read) ram_read_data<=memory[memory_word];
    end
    always @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            read_active<=0;read_owner_d<=0;read_error<=0;read_word<=0;read_remaining<=0;prefer_d<=0;
            response_valid<=0;response_owner_d<=0;response_last<=0;response_error<=0;
            mmio_busy<=0;mmio_write<=0;bus_fault<=0;
            i_refill_count<=0;d_refill_count<=0;store_count<=0;
            i_hit_count<=0;d_hit_count<=0;i_stall_count<=0;d_stall_count<=0;
        end else begin
            if(MONITOR_ENABLE!=0) begin
                if(i_cache_hit_accepted) i_hit_count<=i_hit_count+1'b1;
                if(d_cache_hit_accepted) d_hit_count<=d_hit_count+1'b1;
                if(i_cache_stall_cycle) i_stall_count<=i_stall_count+1'b1;
                if(d_cache_stall_cycle) d_stall_count<=d_stall_count+1'b1;
            end
            response_valid<=read_active;
            if(read_active) begin
                response_owner_d<=read_owner_d;response_last<=read_remaining==1;response_error<=read_error;
                read_word<=read_word+1'b1;read_remaining<=read_remaining-1'b1;
                if(read_remaining==1) read_active<=0;
            end
            if(accepted_read) begin
                read_active<=1;read_owner_d<=!i_fire;read_error<=read_command_error;
                read_word<=command_address[ADDRESS_BITS+1:2];read_remaining<=command_beats;
                prefer_d<=i_fire;
                if(i_fire && ib_size==4) i_refill_count<=i_refill_count+1'b1;
                if(!i_fire && !db_uncached && db_size==4) d_refill_count<=d_refill_count+1'b1;
            end
            if(d_ram_write) begin
                prefer_d<=0;
                if(write_command_error) bus_fault<=1;
                else store_count<=store_count+1'b1;
            end
            if(mmio_fire) begin mmio_busy<=1;mmio_write<=db_write;end
            if(mmio_response) begin
                mmio_busy<=0;
                if(mmio_write && mmio_rsp_error) bus_fault<=1;
            end
        end
    end

    // Full H3 counter widths: 64-bit cycles/retirement and 32-bit cache events.
    // Native write-through supplies no dirty-line writeback event.
    wire [287:0] performance_values={32'b0,d_refill_count,d_hit_count,i_refill_count,i_hit_count,retired_count,cycle_count};
    wire [63:0] monitor_values={d_stall_count,i_stall_count};
    assign snapshot_cycles=peripherals.performance_snapshot[63:0];
    assign snapshot_retired=peripherals.performance_snapshot[127:64];
    assign snapshot_sequence=peripherals.snapshot_sequence;
    rv32_peripherals #(.CLOCK_HZ(CLOCK_HZ),.BAUD(BAUD)) peripherals(
        .clk(clk),.rst_n(rst_n),.performance_values(performance_values),.monitor_values(monitor_values),.performance_config(PERFORMANCE_CONFIG),
        .req_valid(mmio_req_valid),.req_ready(mmio_req_ready),.req_address({db_address[31:2],2'b00}),
        .req_write(db_write),.req_data(db_data),.req_strobe(db_mask),
        .rsp_valid(mmio_rsp_valid),.rsp_ready(mmio_busy),.rsp_data(mmio_rsp_data),.rsp_error(mmio_rsp_error),
        .serial_rx(serial_rx),.serial_tx(serial_tx),.gpio_input(gpio_input),.gpio_output(gpio_output),
        .irq_external(irq_external),.irq_software(irq_software),.irq_timer(irq_timer),.timer_value(timer_value),.status_code(status_code)
    );
endmodule
