`timescale 1ns/1ps
module rv32_observed_soc #(
    parameter integer ENABLE_M=1, ENABLE_C=1, ENABLE_PREDICTOR=1, LOAD_USE_BUBBLE=1,
    parameter integer FAST_ICACHE_HIT=1, FAST_DCACHE_READ=1, FAST_DCACHE_WRITE=1,
    parameter integer FETCH_OVERLAP=1, DATA_OVERLAP=1, EARLY_D_REQUEST=1,
    parameter integer BTB_INDEX_BITS=8, ICACHE_SET_BITS=9, CACHE_SET_BITS=6,
    parameter integer ENABLE_CACHE=1, MEMORY_ADDRESS_BITS=14,
    parameter integer CLOCK_HZ=3200, BAUD=100,
    parameter integer NATIVE_SNAPSHOT=1,
    parameter INIT_FILE="firmware.hex"
) (
    input wire clk, rst_n, serial_rx,
    input wire [7:0] gpio_input,
    output wire serial_tx,
    output wire [7:0] gpio_output,
    output wire [31:0] status_code, trace_pc, trace_instruction, trap_cause,
    output wire maintenance_fault, trace_valid, trap_valid,
    output wire [63:0] cycle_count, retired_count,
    output wire [31:0] snapshot_sequence,
    output wire [63:0] snapshot_cycles, snapshot_retired,
    output wire retire_event, wb_write,
    output wire [4:0] wb_rd,
    output wire [31:0] wb_data,
    output wire redirect_event, redirect_eligible, fetch_request_stall, fetch_response_wait, frontend_barrier,
    output wire [2:0] redirect_kind,
    output wire study_ex_valid,
    output wire study_ex_fire,
    output wire study_id_fire,
    output wire [31:0] study_ex_pc,
    output wire [31:0] study_ex_instruction,
    output wire [2:0] study_ex_length,
    output wire [31:0] study_id_pc,
    output wire [31:0] study_id_instruction,
    output wire [2:0] study_id_length,
    output wire [31:0] study_admit_a,
    output wire [31:0] study_admit_b,
    output wire [31:0] study_forwarded_a,
    output wire [31:0] study_forwarded_b,
    output wire study_prepare,
    output wire study_token,
    output wire study_started,
    output wire study_prefetch_operands,
    output wire study_mem_ready,
    output wire study_mem_fire,
    output wire study_mem_fault,
    output wire study_load_use,
    output wire study_barrier,
    output wire study_mem_flush,
    output wire study_common_exception,
    output wire study_ex_muldiv,
    output wire study_req_valid,
    output wire study_req_ready,
    output wire study_req_precomputed,
    output wire [2:0] study_req_operation,
    output wire study_rsp_available,
    output wire study_rsp_valid,
    output wire study_rsp_ready,
    output wire [31:0] study_rsp_data,
    output wire study_cancel,
    output wire [31:0] study_partial_ll,
    output wire [33:0] study_partial_lh,
    output wire [33:0] study_partial_hl,
    output wire [31:0] study_partial_hh,
    output wire [63:0] study_product,
    output wire study_front_flush,
    output wire study_serial_wait,
    output wire study_id_valid,
    output wire study_if_valid,
    output wire study_current_missing,
    output wire study_following_missing,
    output wire study_memory_launch_wait,
    output wire study_memory_response_wait,
    output wire study_early_request,
    output wire study_d_req_valid,
    output wire study_d_req_ready,
    output wire study_d_rsp_valid,
    output wire study_d_rsp_ready,
    output wire study_i_fill_command,
    output wire study_i_fill_beat,
    output wire study_i_fill_complete,
    output wire study_i_fill_error,
    output wire [31:0] study_i_fill_address,
    output wire study_d_fill_command,
    output wire study_d_fill_beat,
    output wire study_d_fill_complete,
    output wire study_d_fill_error,
    output wire [31:0] study_d_fill_address,
    output wire [3:0] study_d_state,
    output wire study_d_request_missed,
    output wire study_d_request_mmio,
    output wire study_d_request_write,
    output wire study_d_accept,
    output wire study_d_accept_mmio,
    output wire study_d_accept_write,
    output wire study_d_hit_event,
    output wire study_d_miss_event,
    output wire study_d_writeback_event,
    output wire [31:0] study_d_hit_counter,
    output wire [31:0] study_d_miss_counter,
    output wire [31:0] study_d_writeback_counter,
    output wire study_d_writeback_command,
    output wire study_d_writeback_beat,
    output wire study_d_active,
    output wire study_d_active_mmio,
    output wire study_d_wait_behind_i,
    output wire study_i_wait_behind_d,
    output wire study_bank_hold,
    output wire study_backend_command,
    output wire study_backend_command_write,
    output wire [31:0] study_backend_address,
    output wire [7:0] study_backend_length,
    output wire study_backend_owner,
    output wire study_backend_r_valid,
    output wire study_backend_r_ready,
    output wire study_backend_r_last,
    output wire study_backend_read_wait,
    output wire study_backend_b_valid,
    output wire study_backend_b_ready,
    output wire study_data_from_ex,
    output wire [31:0] study_d_address,
    output wire study_i_req_valid,
    output wire study_i_req_ready,
    output wire [31:0] study_i_address,
    output wire [31:0] study_i_pair_address,
    output wire study_i_rsp_valid,
    output wire study_i_rsp_ready,
    output wire study_i_pair_valid,
    output wire study_if_fire,
    output wire [31:0] study_if_pc,
    output wire [31:0] study_if_instruction,
    output wire study_frontend_fire,
    output wire [31:0] study_frontend_pc,
    output wire [31:0] study_frontend_instruction,
    output wire [2:0] study_frontend_length,
    output wire study_ex_redirect
);
    rv32_soc #(
        .ENABLE_M(ENABLE_M),.ENABLE_C(ENABLE_C),.ENABLE_PREDICTOR(ENABLE_PREDICTOR),.LOAD_USE_BUBBLE(LOAD_USE_BUBBLE),
        .FAST_ICACHE_HIT(FAST_ICACHE_HIT),.FAST_DCACHE_READ(FAST_DCACHE_READ),.FAST_DCACHE_WRITE(FAST_DCACHE_WRITE),
        .FETCH_OVERLAP(FETCH_OVERLAP),.DATA_OVERLAP(DATA_OVERLAP),.EARLY_D_REQUEST(EARLY_D_REQUEST),
        .BTB_INDEX_BITS(BTB_INDEX_BITS),.ICACHE_SET_BITS(ICACHE_SET_BITS),.CACHE_SET_BITS(CACHE_SET_BITS),
        .ENABLE_CACHE(ENABLE_CACHE),.MEMORY_ADDRESS_BITS(MEMORY_ADDRESS_BITS),
        .CLOCK_HZ(CLOCK_HZ),.BAUD(BAUD),.INIT_FILE(INIT_FILE)
    ) soc(
        .clk(clk),.rst_n(rst_n),.serial_rx(serial_rx),.gpio_input(gpio_input),.serial_tx(serial_tx),.gpio_output(gpio_output),
        .status_code(status_code),.maintenance_fault(maintenance_fault),.trace_valid(trace_valid),
        .trace_pc(trace_pc),.trace_instruction(trace_instruction),.trap_valid(trap_valid),.trap_cause(trap_cause),
        .cycle_count(cycle_count),.retired_count(retired_count)
    );
    always @(posedge clk) if(rst_n) begin
        if(soc.core.redirect_pair_pc!==({soc.core.redirect_pc[31:2],2'b0}+32'd4))
            $fatal(1,"Preselected redirect companion address mismatch");
    end
    // Independent architectural predicate for the shared branch relation.
    // Check the cycle in which the result enters MEM, including load bypass.
    reg reference_branch_taken;
    always @* begin
        reference_branch_taken=soc.core.ex_decode.is_jal || soc.core.ex_decode.is_jalr;
        if(soc.core.ex_decode.is_branch) begin
            case(soc.core.ex_instruction[14:12])
                0: reference_branch_taken=soc.core.branch_operand_a==soc.core.branch_operand_b;
                1: reference_branch_taken=soc.core.branch_operand_a!=soc.core.branch_operand_b;
                4: reference_branch_taken=$signed(soc.core.forwarded_a)<$signed(soc.core.forwarded_b);
                5: reference_branch_taken=$signed(soc.core.forwarded_a)>=$signed(soc.core.forwarded_b);
                6: reference_branch_taken=soc.core.forwarded_a<soc.core.forwarded_b;
                7: reference_branch_taken=soc.core.forwarded_a>=soc.core.forwarded_b;
                default: reference_branch_taken=0;
            endcase
        end
    end
    always @(posedge clk) if(rst_n && soc.core.ex_valid && soc.core.ex_decode.is_branch && !soc.core.early_load_dependency)
        if(soc.core.early_taken!==reference_branch_taken)
            $fatal(1,"Single comparison bank changed an eligible early branch decision");
    always @(posedge clk) if(rst_n && soc.core.ex_fire && !soc.core.interrupt_pending && soc.core.ex_decode.is_branch)
        if(soc.core.branch_taken!==reference_branch_taken)
            $fatal(1,"Shared branch predicate differs from architectural forwarded operands");
    always @(posedge clk) if(rst_n && soc.i_req_valid && soc.i_req_ready)
        if(soc.i_req_pair_address!==(soc.i_req_address+32'd4))
            $fatal(1,"Precomputed companion request address mismatch");
    always @(posedge clk) if(rst_n && soc.core.frontend_buffered_valid && soc.core.frontend_out_valid) begin
        if(soc.core.frontend_buffered_instruction!==soc.core.frontend_instruction ||
           soc.core.frontend_buffered_length!==soc.core.frontend_length)
            $fatal(1,"Buffered-only static decode differs from delivered instruction");
    end
    always @(posedge clk) if(rst_n)
        if(soc.core.frontend_redirect_address_select!==soc.core.frontend_flush)
            $fatal(1,"Factored redirect address selector differs from architectural flush");
    // Reference remains the architectural generic immediate decoder. Compare
    // target arithmetic only for eligible ID branches; inactive payloads differ.
    wire [31:0] generic_id_target=soc.core.id_pc+soc.core.id_decode.immediate;
    wire generic_id_aligned=!(soc.core.id_zero_taken &&
        (generic_id_target[0] || (ENABLE_C==0 && generic_id_target[1])));
    wire generic_id_miss=soc.core.id_zero_taken ? soc.core.id_prediction!=generic_id_target :
        soc.core.id_prediction!=soc.core.id_zero_fallthrough;
    wire generic_id_resolve=soc.core.id_zero_pair && generic_id_aligned;
    wire generic_id_redirect=generic_id_resolve && generic_id_miss;
    wire generic_id_address_redirect=soc.core.id_zero_address_pair && generic_id_aligned && generic_id_miss;
    wire [31:0] generic_id_next=soc.core.id_zero_taken ? generic_id_target : soc.core.id_zero_fallthrough;
    wire [31:0] generic_active_redirect_pc=soc.core.mem_flush ? soc.core.memory_redirect_pc :
        soc.core.ex_redirect_candidate ? soc.core.early_next : generic_id_next;
    always @(posedge clk) if(rst_n) begin
        if((soc.core.id_zero_pair || soc.core.id_zero_address_pair) && soc.core.id_zero_target!==generic_id_target)
            $fatal(1,"Raw ID branch target differs from generic decoder for eligible branch");
        if(soc.core.id_zero_resolve!==generic_id_resolve || soc.core.id_zero_redirect!==generic_id_redirect ||
           soc.core.id_zero_address_redirect!==generic_id_address_redirect)
            $fatal(1,"Raw ID branch target changed resolution or redirect control");
        if(soc.core.frontend_flush && (soc.core.redirect_pc!==generic_active_redirect_pc ||
           soc.core.redirect_pair_pc!==({generic_active_redirect_pc[31:2],2'b0}+32'd4) ||
           soc.core.redirect_fetch_valid!==((generic_active_redirect_pc>>(MEMORY_ADDRESS_BITS+2))==0)))
            $fatal(1,"Raw ID branch target changed active frontend redirect payload");
    end
    GTP_GRS GRS_INST(.GRS_N(1'b1));
    assign snapshot_sequence=soc.peripherals.snapshot_sequence;
    // Check reset state, invalid/held WB cycles, and every instruction class.
    always @(posedge clk)
        if(soc.core.wb_data_zero!==(soc.core.wb_data==0))
            $fatal(1,"Registered WB zero flag differs from WB data");
    always @(posedge clk) if(rst_n && soc.core.id_zero_pair && soc.core.ex_valid)
        if(soc.core.execute_exception || soc.core.ex_decode.is_mret ||
           soc.core.ex_decode.is_fence_i || soc.core.ex_decode.is_jalr)
            $fatal(1,"Conservative ID fault guard admitted an older precise exception or serial redirect");
    always @(posedge clk) if(rst_n && soc.core.id_valid) begin
        if(soc.core.id_value_a_zero!==(soc.core.id_operand_a==0) ||
           soc.core.id_value_b_zero!==(soc.core.id_operand_b==0))
            $fatal(1,"ID zero flag differs from the live masked/WB-forwarded register operand");
    end
    wire [31:0] legacy_redirect_pc=soc.core.mem_flush ? soc.core.memory_redirect_pc :
        soc.core.ex_redirect ? soc.core.early_next : soc.core.id_zero_next;
    wire [31:0] legacy_redirect_pair=soc.core.mem_flush ? soc.core.memory_redirect_pair :
        soc.core.ex_redirect ? soc.core.ex_redirect_pair : soc.core.id_redirect_pair;
    wire legacy_redirect_range=soc.core.mem_flush ? (soc.core.memory_redirect_pc >> (MEMORY_ADDRESS_BITS+2))==0 :
        soc.core.ex_redirect ? (soc.core.early_taken ? (soc.core.early_target >> (MEMORY_ADDRESS_BITS+2))==0 :
        (soc.core.early_fallthrough >> (MEMORY_ADDRESS_BITS+2))==0) :
        soc.core.id_zero_taken ? (soc.core.id_zero_target >> (MEMORY_ADDRESS_BITS+2))==0 :
        (soc.core.id_zero_fallthrough >> (MEMORY_ADDRESS_BITS+2))==0;
    integer preselect_idle_count,preselect_load_wait_count,preselect_irq_count;
    always @(posedge clk) begin
        if(!rst_n) begin preselect_idle_count<=0; preselect_load_wait_count<=0; preselect_irq_count<=0; end
        else begin
            if(soc.core.frontend_flush && (soc.core.redirect_pc!==legacy_redirect_pc ||
               soc.core.redirect_pair_pc!==legacy_redirect_pair || soc.core.redirect_fetch_valid!==legacy_redirect_range))
                $fatal(1,"EX address preselection changed an active redirect payload");
            if(soc.core.id_zero_redirect && soc.core.ex_redirect_candidate)
                $fatal(1,"EX address candidate conflicted with an ID redirect");
            if(soc.core.ex_redirect_candidate && !soc.core.frontend_flush) preselect_idle_count<=preselect_idle_count+1;
            if(soc.core.ex_redirect_candidate && soc.core.mem_valid && soc.core.mem_load && !soc.core.mem_ready)
                preselect_load_wait_count<=preselect_load_wait_count+1;
            if(soc.core.interrupt_pending) preselect_irq_count<=preselect_irq_count+1;
        end
    end
    final $display("EX_PRESELECT_PROFILE idle_candidates=%0d older_load_wait=%0d irq_cycles=%0d",
        preselect_idle_count,preselect_load_wait_count,preselect_irq_count);
    reg id_zero_expected_valid;
    reg [31:0] id_zero_expected_pc,id_zero_expected_next;
    integer id_zero_resolve_count,id_zero_redirect_count;
    always @(posedge clk) begin
        if(!rst_n) begin
            id_zero_expected_valid<=0; id_zero_resolve_count<=0; id_zero_redirect_count<=0;
        end else begin
            if(id_zero_expected_valid && (!soc.core.ex_valid || soc.core.ex_pc!==id_zero_expected_pc ||
               soc.core.ex_prediction!==id_zero_expected_next || soc.core.actual_next!==id_zero_expected_next))
                $fatal(1,"ID zero branch lost or resolved differently in EX");
            if(soc.core.id_zero_redirect && (!soc.core.id_fire || soc.core.id_zero_hazard ||
               soc.core.mem_flush || soc.core.ex_redirect || soc.core.interrupt_pending))
                $fatal(1,"ID zero redirect violated older-stage priority or operand availability");
            id_zero_expected_valid<=soc.core.id_zero_resolve;
            if(soc.core.id_zero_resolve) begin
                id_zero_expected_pc<=soc.core.id_pc; id_zero_expected_next<=soc.core.id_zero_next;
                id_zero_resolve_count<=id_zero_resolve_count+1;
            end
            if(soc.core.id_zero_redirect) id_zero_redirect_count<=id_zero_redirect_count+1;
        end
    end
    final $display("ID_ZERO_PROFILE resolved=%0d redirects=%0d",id_zero_resolve_count,id_zero_redirect_count);
    always @(posedge clk) if(rst_n && soc.core.frontend_flush)
        if(soc.core.redirect_fetch_valid!==((soc.core.redirect_pc >> (MEMORY_ADDRESS_BITS+2))==0))
            $fatal(1,"Redirect range factoring mismatch");
    always @(posedge clk) if(rst_n && soc.core.ex_fire && !soc.core.ex_decode.is_mret)
        if(soc.core.execute_prediction_miss!==(soc.core.actual_next!=soc.core.ex_prediction))
            $fatal(1,"Prediction mismatch factoring changed the next-PC comparison");
    reg expected_mem_prediction_miss;
    reg [99:0] expected_if_payload;
    reg [31:0] expected_ex_operand_a,expected_ex_operand_b;
    reg [31:0] expected_fetch_request;
    wire reference_ex_capacity=soc.core.ex_valid && soc.core.mem_ready && !soc.core.load_use_wait &&
        !((soc.core.ex_decode.is_csr || soc.core.ex_decode.is_mret) && (soc.core.mem_valid || soc.core.wb_valid)) &&
        (!soc.core.ex_muldiv || soc.core.execute_local_exception || soc.core.muldiv_valid);
    always @(posedge clk) if(rst_n) begin
        if(soc.core.ex_fire!==(soc.core.interrupt_pending ? soc.core.ex_fire_with_irq : reference_ex_capacity && !soc.core.mem_flush))
            $fatal(1,"Multiply availability factoring changed execution transfer");
        if(soc.core.ex_fire && soc.core.ex_muldiv && !soc.core.execute_common_exception && !soc.core.muldiv_valid)
            $fatal(1,"Multiply result consumed without a valid response");
    end
    always @(posedge clk) begin
        if(rst_n) begin
            if(soc.core.wb_valid && soc.core.wb_write_candidate!==(soc.core.wb_writes && soc.core.wb_rd!=0))
                $fatal(1,"Registered writeback eligibility mismatch");
            if(soc.core.mem_valid && soc.core.mem_forward_candidate!==
                (soc.core.mem_writes && soc.core.mem_rd!=0 && !soc.core.mem_exception))
                $fatal(1,"Registered memory forwarding eligibility mismatch");
            if(soc.core.frontend.request_pending && soc.core.frontend.request_address!==expected_fetch_request)
                $fatal(1,"Fetch request payload enable changed an outstanding address");
            if(soc.i_req_valid && soc.i_req_ready) expected_fetch_request<=soc.i_req_address;
            if(soc.core.predictor.prediction_hit &&
               soc.core.predictor.direction_storage[soc.core.predictor.fetch_index]!==
               (!soc.core.predictor.conditional[soc.core.predictor.fetch_index] ||
                soc.core.predictor.pattern_prediction[soc.core.predictor.history[soc.core.predictor.fetch_index]]))
                $fatal(1,"Precomputed branch direction differs from the local-history predictor");
            if(soc.core.ex_valid && (soc.core.ex_operand_a!==expected_ex_operand_a ||
                                    soc.core.ex_operand_b!==expected_ex_operand_b))
                $fatal(1,"Execution operand enable factoring changed a valid operand");
            if(soc.core.ex_valid) begin
                expected_ex_operand_a<=soc.core.forwarded_a;
                expected_ex_operand_b<=soc.core.forwarded_b;
            end
            if(soc.core.id_fire) begin
                expected_ex_operand_a<=soc.core.id_operand_a;
                expected_ex_operand_b<=soc.core.id_operand_b;
            end
            if(soc.core.if_valid && {soc.core.if_pc,soc.core.if_instruction,soc.core.if_prediction,
                soc.core.if_length,soc.core.if_error}!==expected_if_payload)
                $fatal(1,"Fetch payload enable factoring changed a valid instruction");
            if(soc.core.frontend_fire)
                expected_if_payload<={soc.core.frontend_pc,soc.core.frontend_instruction,soc.core.frontend_prediction,
                    soc.core.frontend_length,soc.core.frontend_out_error};
            if(soc.i_rsp_valid && soc.i_rsp_is_c!=={soc.i_rsp_data[17:16]!=2'b11,soc.i_rsp_data[1:0]!=2'b11})
                $fatal(1,"Cache instruction-length metadata mismatch");
            if(soc.i_rsp_valid && soc.i_rsp_pair_valid && soc.i_rsp_pair_is_c!==
                {soc.i_rsp_pair_data[17:16]!=2'b11,soc.i_rsp_pair_data[1:0]!=2'b11})
                $fatal(1,"Cache companion-length metadata mismatch");
            if(soc.core.frontend.out_valid && !soc.core.frontend.out_error && soc.core.ENABLE_C!=0 &&
               soc.core.frontend.compressed!==(soc.core.frontend.halfword[1:0]!=2'b11))
                $fatal(1,"Buffered instruction-length metadata mismatch");
            if(soc.core.frontend_fire!==(soc.core.frontend_out_valid &&
               (!soc.core.if_valid || !soc.core.id_valid || !soc.core.ex_valid || soc.core.ex_fire) &&
               !soc.core.frontend_flush && !soc.core.frontend_barrier))
                $fatal(1,"Fetch acceptance factoring changed a handshake");
            if(soc.peripherals.rx_fifo_valid!==(soc.peripherals.rx_count!=0) ||
               soc.peripherals.uart_tx_valid!==(soc.peripherals.tx_count!=0))
                $fatal(1,"Registered FIFO occupancy differs from its count");
            if(soc.core.ex_valid && (soc.core.early_zero_a!==(soc.core.ex_rs1==0 || !soc.core.ex_decode.uses_rs1) ||
                                    soc.core.early_zero_b!==(soc.core.ex_rs2==0 || !soc.core.ex_decode.uses_rs2)))
                $fatal(1,"Registered source-operand mask mismatch");
            if(soc.core.wb_valid && (soc.core.wb_match_a!==(soc.core.ex_rs1==soc.core.wb_rd) ||
                                    soc.core.wb_match_b!==(soc.core.ex_rs2==soc.core.wb_rd)))
                $fatal(1,"Registered writeback dependency mismatch");
        end
        if(rst_n && soc.core.ex_fire)
            expected_mem_prediction_miss<=soc.core.actual_next!=soc.core.ex_prediction;
        if(rst_n && soc.core.mem_valid && !soc.core.mem_mret)
            if(soc.core.mem_prediction_miss!==expected_mem_prediction_miss)
                $fatal(1,"Registered prediction mismatch changed the next-PC comparison");
    end
    generate begin: native_snapshot_observation
        // Read the actual vendor behavioral storage, never an unchecked counter mirror.
        wire [127:0] captured_native_words;
        for(genvar word_index=0;word_index<4;word_index=word_index+1) begin: observed_word
            for(genvar bank=0;bank<4;bank=bank+1) begin: observed_bank
                assign captured_native_words[word_index*32+bank*8+:8]=soc.peripherals.snapshot_bank[bank].native_ram.mem[word_index][7:0];
            end
        end
        assign snapshot_cycles=soc.peripherals.snapshot_valid ? captured_native_words[63:0] : 64'b0;
        assign snapshot_retired=soc.peripherals.snapshot_valid ? captured_native_words[127:64] : 64'b0;
    end endgenerate
    assign retire_event=soc.core.wb_valid && !soc.core.wb_counter_write;
    assign wb_write=soc.core.trace_write;
    assign wb_rd=soc.core.trace_rd;
    assign wb_data=soc.core.trace_data;
    wire [6:0] redirect_opcode=soc.core.mem_flush ? soc.core.mem_instruction[6:0] :
        soc.core.ex_redirect ? soc.core.ex_instruction[6:0] : soc.core.id_instruction[6:0];
    assign redirect_event=soc.core.frontend_flush;
    assign frontend_barrier=soc.core.frontend_barrier;
    assign redirect_eligible=redirect_event && !frontend_barrier;
    assign redirect_kind=(soc.core.mem_flush && soc.core.mem_fault) ? 3'd3 :
        (soc.core.mem_flush && soc.core.mem_mret) ? 3'd4 :
        (soc.core.mem_flush && soc.core.mem_fence_i) ? 3'd5 :
        redirect_opcode==7'h63 ? 3'd0 : redirect_opcode==7'h6f ? 3'd1 : redirect_opcode==7'h67 ? 3'd2 : 3'd6;
    assign fetch_request_stall=soc.i_req_valid && !soc.i_req_ready;
    assign fetch_response_wait=soc.core.frontend.request_pending && !soc.core.frontend.i_rsp_valid;
    // PASSIVE_STUDY_BEGIN
    assign study_ex_valid = soc.core.ex_valid;
    assign study_ex_fire = soc.core.ex_fire;
    assign study_id_fire = soc.core.id_fire;
    assign study_ex_pc = soc.core.ex_pc;
    assign study_ex_instruction = soc.core.ex_instruction;
    assign study_ex_length = soc.core.ex_length;
    assign study_id_pc = soc.core.id_pc;
    assign study_id_instruction = soc.core.id_instruction;
    assign study_id_length = soc.core.id_length;
    assign study_admit_a = soc.core.mul_id_operand_a;
    assign study_admit_b = soc.core.mul_id_operand_b;
    assign study_forwarded_a = soc.core.forwarded_a;
    assign study_forwarded_b = soc.core.forwarded_b;
    assign study_prepare = soc.core.mul_precompute;
    assign study_token = soc.core.mul_precomputed;
    assign study_started = soc.core.muldiv_started;
    assign study_prefetch_operands = soc.core.mul_prefetch_operands;
    assign study_mem_ready = soc.core.mem_ready;
    assign study_mem_fire = soc.core.mem_fire;
    assign study_mem_fault = soc.core.mem_fault;
    assign study_load_use = soc.core.load_use_wait;
    assign study_barrier = soc.core.frontend_barrier;
    assign study_mem_flush = soc.core.mem_flush;
    assign study_common_exception = soc.core.execute_common_exception;
    assign study_ex_muldiv = soc.core.ex_muldiv;
    assign study_req_valid = soc.core.multiply_divide.unit.req_valid;
    assign study_req_ready = soc.core.multiply_divide.unit.req_ready;
    assign study_req_precomputed = soc.core.multiply_divide.unit.req_precomputed;
    assign study_req_operation = soc.core.multiply_divide.unit.req_operation;
    assign study_rsp_available = soc.core.multiply_divide.unit.rsp_available;
    assign study_rsp_valid = soc.core.multiply_divide.unit.rsp_valid;
    assign study_rsp_ready = soc.core.multiply_divide.unit.rsp_ready;
    assign study_rsp_data = soc.core.multiply_divide.unit.rsp_data;
    assign study_cancel = soc.core.multiply_divide.unit.cancel;
    assign study_partial_ll = soc.core.multiply_divide.unit.product_low_low;
    assign study_partial_lh = soc.core.multiply_divide.unit.product_low_high;
    assign study_partial_hl = soc.core.multiply_divide.unit.product_high_low;
    assign study_partial_hh = soc.core.multiply_divide.unit.product_high_high;
    assign study_product = soc.core.multiply_divide.unit.product;
    assign study_front_flush = soc.core.frontend_flush;
    assign study_serial_wait = soc.core.ex_serial && (soc.core.mem_valid || soc.core.wb_valid);
    assign study_id_valid = soc.core.id_valid;
    assign study_if_valid = soc.core.if_valid;
    assign study_current_missing = soc.core.frontend.out_ready && !soc.core.frontend.redirect_valid && !soc.core.frontend.invalidate && !soc.core.frontend.current_valid;
    assign study_following_missing = soc.core.frontend.out_ready && !soc.core.frontend.redirect_valid && !soc.core.frontend.invalidate && soc.core.frontend.current_valid && !soc.core.frontend.current_error && soc.core.frontend.straddles && !soc.core.frontend.following_valid;
    assign study_memory_launch_wait = soc.core.mem_valid && soc.core.mem_access && !soc.core.mem_sent;
    assign study_memory_response_wait = soc.core.mem_valid && soc.core.mem_access && soc.core.mem_sent && !soc.d_rsp_valid;
    assign study_early_request = soc.core.early_data_request;
    assign study_d_req_valid = soc.d_req_valid;
    assign study_d_req_ready = soc.d_req_ready;
    assign study_d_rsp_valid = soc.d_rsp_valid;
    assign study_d_rsp_ready = soc.d_rsp_ready;
    assign study_i_fill_command = soc.memory_system.caches[0].enabled.cache.cmd_valid && soc.memory_system.caches[0].enabled.cache.cmd_ready && !soc.memory_system.caches[0].enabled.cache.cmd_write;
    assign study_i_fill_beat = soc.memory_system.caches[0].enabled.cache.r_valid && soc.memory_system.caches[0].enabled.cache.r_ready;
    assign study_i_fill_complete = soc.memory_system.caches[0].enabled.cache.r_valid && soc.memory_system.caches[0].enabled.cache.r_ready && soc.memory_system.caches[0].enabled.cache.r_last;
    assign study_i_fill_error = soc.memory_system.caches[0].enabled.cache.r_valid && soc.memory_system.caches[0].enabled.cache.r_ready && soc.memory_system.caches[0].enabled.cache.r_error;
    assign study_i_fill_address = soc.memory_system.caches[0].enabled.cache.cmd_address;
    assign study_d_fill_command = soc.memory_system.caches[1].enabled.cache.cmd_valid && soc.memory_system.caches[1].enabled.cache.cmd_ready && !soc.memory_system.caches[1].enabled.cache.cmd_write;
    assign study_d_fill_beat = soc.memory_system.caches[1].enabled.cache.r_valid && soc.memory_system.caches[1].enabled.cache.r_ready;
    assign study_d_fill_complete = soc.memory_system.caches[1].enabled.cache.r_valid && soc.memory_system.caches[1].enabled.cache.r_ready && soc.memory_system.caches[1].enabled.cache.r_last;
    assign study_d_fill_error = soc.memory_system.caches[1].enabled.cache.r_valid && soc.memory_system.caches[1].enabled.cache.r_ready && soc.memory_system.caches[1].enabled.cache.r_error;
    assign study_d_fill_address = soc.memory_system.caches[1].enabled.cache.cmd_address;
    assign study_d_state = soc.memory_system.caches[1].enabled.cache.state;
    assign study_d_request_missed = soc.memory_system.caches[1].enabled.cache.request_missed;
    assign study_d_request_mmio = |soc.memory_system.caches[1].enabled.cache.request_address[31:28];
    assign study_d_request_write = soc.memory_system.caches[1].enabled.cache.request_write;
    assign study_d_accept = soc.memory_system.caches[1].enabled.cache.req_valid && soc.memory_system.caches[1].enabled.cache.req_ready;
    assign study_d_accept_mmio = |soc.memory_system.caches[1].enabled.cache.req_address[31:28];
    assign study_d_accept_write = soc.memory_system.caches[1].enabled.cache.req_write;
    assign study_d_hit_event = soc.memory_system.caches[1].enabled.cache.state==4'd1 && (soc.memory_system.caches[1].enabled.cache.hit0 || soc.memory_system.caches[1].enabled.cache.hit1) && !soc.memory_system.caches[1].enabled.cache.request_missed;
    assign study_d_miss_event = soc.memory_system.caches[1].enabled.cache.state==4'd1 && !(soc.memory_system.caches[1].enabled.cache.hit0 || soc.memory_system.caches[1].enabled.cache.hit1);
    assign study_d_writeback_event = soc.memory_system.caches[1].enabled.cache.state==4'd4 && soc.memory_system.caches[1].enabled.cache.b_valid && !soc.memory_system.caches[1].enabled.cache.b_error;
    assign study_d_hit_counter = soc.memory_system.d_hit_count;
    assign study_d_miss_counter = soc.memory_system.d_miss_count;
    assign study_d_writeback_counter = soc.memory_system.d_writeback_count;
    assign study_d_writeback_command = soc.memory_system.caches[1].enabled.cache.cmd_valid && soc.memory_system.caches[1].enabled.cache.cmd_ready && soc.memory_system.caches[1].enabled.cache.cmd_write;
    assign study_d_writeback_beat = soc.memory_system.caches[1].enabled.cache.w_valid && soc.memory_system.caches[1].enabled.cache.w_ready;
    assign study_d_active = soc.memory_system.data_active;
    assign study_d_active_mmio = soc.memory_system.data_mmio;
    assign study_d_wait_behind_i = soc.memory_system.caches[1].enabled.cache.cmd_valid && !soc.memory_system.caches[1].enabled.cache.cmd_ready && soc.memory_system.arbiter.state!=2'd0 && !soc.memory_system.arbiter.owner;
    assign study_i_wait_behind_d = soc.memory_system.master_cmd_valid[0] && !soc.memory_system.master_cmd_ready[0] && soc.memory_system.arbiter.state!=2'd0 && soc.memory_system.arbiter.owner;
    assign study_bank_hold = soc.core.mul_precomputed;
    assign study_backend_command = soc.memory_system.ram.cmd_valid && soc.memory_system.ram.cmd_ready;
    assign study_backend_command_write = soc.memory_system.ram.cmd_write;
    assign study_backend_address = soc.memory_system.ram.cmd_address;
    assign study_backend_length = soc.memory_system.ram.cmd_length;
    assign study_backend_owner = soc.memory_system.arbiter.owner;
    assign study_backend_r_valid = soc.memory_system.ram.r_valid;
    assign study_backend_r_ready = soc.memory_system.ram.r_ready;
    assign study_backend_r_last = soc.memory_system.ram.r_last;
    assign study_backend_read_wait = soc.memory_system.ram.state==3'd1 && soc.memory_system.ram.read_service_wait;
    assign study_backend_b_valid = soc.memory_system.ram.b_valid;
    assign study_backend_b_ready = soc.memory_system.ram.b_ready;
    assign study_data_from_ex = soc.core.early_data_request && soc.core.select_ex_data;
    assign study_d_address = soc.d_req_address;
    assign study_i_req_valid = soc.i_req_valid;
    assign study_i_req_ready = soc.i_req_ready;
    assign study_i_address = soc.i_req_address;
    assign study_i_pair_address = soc.i_req_pair_address;
    assign study_i_rsp_valid = soc.i_rsp_valid;
    assign study_i_rsp_ready = soc.i_rsp_ready;
    assign study_i_pair_valid = soc.i_rsp_pair_valid;
    assign study_if_fire = soc.core.if_fire;
    assign study_if_pc = soc.core.if_pc;
    assign study_if_instruction = soc.core.if_normalized_instruction;
    assign study_frontend_fire = soc.core.frontend_fire;
    assign study_frontend_pc = soc.core.frontend_pc;
    assign study_frontend_instruction = soc.core.frontend_instruction;
    assign study_frontend_length = soc.core.frontend_length;
    assign study_ex_redirect = soc.core.ex_redirect;
    // PASSIVE_STUDY_END
endmodule
