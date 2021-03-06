// Copyright (C) 1991-2016 Altera Corporation
//
// Your use of Altera Corporation's design tools, logic functions 
// and other software and tools, and its AMPP partner logic 
// functions, and any output files from any of the foregoing 
// (including device programming or simulation files), and any 
// associated documentation or information are expressly subject 
// to the terms and conditions of the Altera Program License 
// Subscription Agreement, Altera MegaCore Function License 
// Agreement, or other applicable license agreement, including, 
// without limitation, that your use is for the sole purpose of 
// programming logic devices manufactured by Altera and sold by 
// Altera or its authorized distributors.  Please refer to the 
// applicable agreement for further details.

// Generated by Quartus.
// With edits from Matthew Naylor, June 2016.

// Dual-port block RAM
// ===================

// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on
module AlteraBlockRam (
  CLK,     // Clock
  DI,      // Data in
  RD_ADDR, // Read address
  WR_ADDR, // Write address
  WE,      // Write enable
  RE,      // Read enable
  BE,      // Byte enable
  DO       // Data out
  );

  parameter ADDR_WIDTH   = 1;
  parameter DATA_WIDTH   = 1;
  parameter NUM_ELEMS    = 1;
  parameter BE_WIDTH     = 1;
  parameter RD_DURING_WR = "OLD_DATA";     // Or: "DONT_CARE"
  parameter DO_REG       = "UNREGISTERED"; // Or: "CLOCK0"
  parameter INIT_FILE    = "UNUSED";
  parameter DEV_FAMILY   = "Stratix V";
  parameter STYLE        = "AUTO";

  input  CLK;
  input  [DATA_WIDTH-1:0] DI;
  input  [ADDR_WIDTH-1:0] RD_ADDR;
  input  [ADDR_WIDTH-1:0] WR_ADDR;
  input  [BE_WIDTH-1:0] BE;
  input  WE;
  input  RE;
  output [DATA_WIDTH-1:0] DO;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
  //tri1 BE[BE_WIDTH-1:0];
  tri1 CLK;
  tri0 WE;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

  altsyncram altsyncram_component (
        .address_a (WR_ADDR),
        .byteena_a (BE),
        .clock0 (CLK),
        .data_a (DI),
        .wren_a (WE),
        .address_b (RD_ADDR),
        .q_b (DO),
        .aclr0 (1'b0),
        .aclr1 (1'b0),
        .addressstall_a (1'b0),
        .addressstall_b (1'b0),
        .byteena_b (-1),
        .clock1 (1'b1),
        .clocken0 (1'b1),
        .clocken1 (1'b1),
        .clocken2 (1'b1),
        .clocken3 (1'b1),
        .data_b (-1),
        .eccstatus (),
        .q_a (),
        .rden_a (1'b1),
        .rden_b (1'b1),
        .wren_b (1'b0));
  defparam
    altsyncram_component.address_aclr_b = "NONE",
    altsyncram_component.address_reg_b = "CLOCK0",
    altsyncram_component.clock_enable_input_a = "BYPASS",
    altsyncram_component.clock_enable_input_b = "BYPASS",
    altsyncram_component.clock_enable_output_b = "BYPASS",
    altsyncram_component.init_file = INIT_FILE,
    altsyncram_component.intended_device_family = DEV_FAMILY,
    altsyncram_component.lpm_type = "altsyncram",
    altsyncram_component.numwords_a = NUM_ELEMS,
    altsyncram_component.numwords_b = NUM_ELEMS,
    altsyncram_component.operation_mode = "DUAL_PORT",
    altsyncram_component.outdata_aclr_b = "NONE",
    altsyncram_component.outdata_reg_b = DO_REG,
    altsyncram_component.power_up_uninitialized = "FALSE",
    altsyncram_component.read_during_write_mode_mixed_ports = RD_DURING_WR,
    altsyncram_component.widthad_a = ADDR_WIDTH,
    altsyncram_component.widthad_b = ADDR_WIDTH,
    altsyncram_component.width_a = DATA_WIDTH,
    altsyncram_component.width_b = DATA_WIDTH,
    altsyncram_component.width_byteena_a = BE_WIDTH,
    altsyncram_component.ram_block_type = STYLE;

endmodule

// Mixed-width true dual port block RAM
// ====================================

// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on
module AlteraBlockRamTrueMixed (
  CLK,    // Clock
  DI_A,   // Port A data in
  DI_B,   // Port B data in
  ADDR_A, // Port A address
  ADDR_B, // Port B address
  WE_A,   // Port A write enable
  WE_B,   // Port B write enable
  EN_A,   // Port A enable
  EN_B,   // Port B enable
  DO_A,   // Port A data out
  DO_B    // Port B data out
  );

  parameter ADDR_WIDTH_A = 1;
  parameter ADDR_WIDTH_B = 1;
  parameter DATA_WIDTH_A = 1;
  parameter DATA_WIDTH_B = 1;
  parameter NUM_ELEMS_A  = 1;
  parameter NUM_ELEMS_B  = 1;
  parameter RD_DURING_WR = "OLD_DATA";     // Or: "DONT_CARE"
  parameter DO_REG_A     = "UNREGISTERED"; // Or: "CLOCK0"
  parameter DO_REG_B     = "UNREGISTERED"; // Or: "CLOCK0"
  parameter DEV_FAMILY   = "Stratix V";
  parameter INIT_FILE    = "UNUSED";
  parameter STYLE        = "AUTO";

  input   [ADDR_WIDTH_A-1:0]  ADDR_A;
  input   [ADDR_WIDTH_B-1:0]  ADDR_B;
  input   CLK;
  input   [DATA_WIDTH_A-1:0]  DI_A;
  input   [DATA_WIDTH_B-1:0]  DI_B;
  input   WE_A;
  input  WE_B;
  input  EN_A;
  input  EN_B;
  output [DATA_WIDTH_A-1:0]  DO_A;
  output [DATA_WIDTH_B-1:0]  DO_B;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
  tri1    CLK;
  tri0    WE_A;
  tri0    WE_B;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

  altsyncram altsyncram_component (
        .address_a (ADDR_A),
        .address_b (ADDR_B),
        .byteena_b (1'b1),
        .clock0 (CLK),
        .data_a (DI_A),
        .data_b (DI_B),
        .wren_a (EN_A & WE_A),
        .wren_b (EN_B & WE_B),
        .q_a (DO_A),
        .q_b (DO_B),
        .aclr0 (1'b0),
        .aclr1 (1'b0),
        .addressstall_a (1'b0),
        .addressstall_b (1'b0),
        .byteena_a (1'b1),
        .clock1 (1'b1),
        .clocken0 (1'b1),
        .clocken1 (1'b1),
        .clocken2 (1'b1),
        .clocken3 (1'b1),
        .eccstatus (),
        .rden_a (1'b1),
        .rden_b (1'b1));
  defparam
    altsyncram_component.address_reg_b = "CLOCK0",
    altsyncram_component.clock_enable_input_a = "BYPASS",
    altsyncram_component.clock_enable_input_b = "BYPASS",
    altsyncram_component.clock_enable_output_a = "BYPASS",
    altsyncram_component.clock_enable_output_b = "BYPASS",
    altsyncram_component.init_file = INIT_FILE,
    altsyncram_component.init_file_layout = "PORT_A",
    altsyncram_component.indata_reg_b = "CLOCK0",
    altsyncram_component.intended_device_family = DEV_FAMILY,
    altsyncram_component.lpm_type = "altsyncram",
    altsyncram_component.numwords_a = NUM_ELEMS_A,
    altsyncram_component.numwords_b = NUM_ELEMS_B,
    altsyncram_component.operation_mode = "BIDIR_DUAL_PORT",
    altsyncram_component.outdata_aclr_a = "NONE",
    altsyncram_component.outdata_aclr_b = "NONE",
    altsyncram_component.outdata_reg_a = DO_REG_A,
    altsyncram_component.outdata_reg_b = DO_REG_B,
    altsyncram_component.power_up_uninitialized = "FALSE",
    altsyncram_component.read_during_write_mode_mixed_ports = RD_DURING_WR,
    altsyncram_component.read_during_write_mode_port_a = "NEW_DATA_NO_NBE_READ",
    altsyncram_component.read_during_write_mode_port_b = "NEW_DATA_NO_NBE_READ",
    altsyncram_component.widthad_a = ADDR_WIDTH_A,
    altsyncram_component.widthad_b = ADDR_WIDTH_B,
    altsyncram_component.width_a = DATA_WIDTH_A,
    altsyncram_component.width_b = DATA_WIDTH_B,
    altsyncram_component.width_byteena_a = 1,
    altsyncram_component.width_byteena_b = 1,
    altsyncram_component.wrcontrol_wraddress_reg_b = "CLOCK0",
    altsyncram_component.ram_block_type = STYLE;

endmodule

// Mixed-width true dual port block RAM with byte enables
// ======================================================

// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on
module AlteraBlockRamTrueMixedBE (
  CLK,    // Clock
  DI_A,   // Port A data in
  DI_B,   // Port B data in
  ADDR_A, // Port A address
  ADDR_B, // Port B address
  BE_B,   // Port B byte enable
  WE_A,   // Port A write enable
  WE_B,   // Port B write enable
  EN_A,   // Port A enable
  EN_B,   // Port B enable
  DO_A,   // Port A data out
  DO_B    // Port B data out
  );

  parameter ADDR_WIDTH_A = 1;
  parameter ADDR_WIDTH_B = 1;
  parameter DATA_WIDTH_A = 1;
  parameter DATA_WIDTH_B = 1;
  parameter NUM_ELEMS_A  = 1;
  parameter NUM_ELEMS_B  = 1;
  parameter BE_WIDTH     = 1;
  parameter RD_DURING_WR = "OLD_DATA";     // Or: "DONT_CARE"
  parameter DO_REG_A     = "UNREGISTERED"; // Or: "CLOCK0"
  parameter DO_REG_B     = "UNREGISTERED"; // Or: "CLOCK0"
  parameter DEV_FAMILY   = "Stratix V";
  parameter INIT_FILE    = "UNUSED";
  parameter STYLE        = "AUTO";

  input   [ADDR_WIDTH_A-1:0]  ADDR_A;
  input   [ADDR_WIDTH_B-1:0]  ADDR_B;
  input   [BE_WIDTH-1:0]      BE_B;
  input   CLK;
  input   [DATA_WIDTH_A-1:0]  DI_A;
  input   [DATA_WIDTH_B-1:0]  DI_B;
  input   WE_A;
  input  WE_B;
  input  EN_A;
  input  EN_B;
  output [DATA_WIDTH_A-1:0]  DO_A;
  output [DATA_WIDTH_B-1:0]  DO_B;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
  //tri1  [BE_WIDTH-1:0] BE_B;
  tri1    CLK;
  tri0    WE_A;
  tri0    WE_B;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

  altsyncram altsyncram_component (
        .address_a (ADDR_A),
        .address_b (ADDR_B),
        .byteena_b (BE_B),
        .clock0 (CLK),
        .data_a (DI_A),
        .data_b (DI_B),
        .wren_a (EN_A & WE_A),
        .wren_b (EN_B & WE_B),
        .q_a (DO_A),
        .q_b (DO_B),
        .aclr0 (1'b0),
        .aclr1 (1'b0),
        .addressstall_a (1'b0),
        .addressstall_b (1'b0),
        .byteena_a (1'b1),
        .clock1 (1'b1),
        .clocken0 (1'b1),
        .clocken1 (1'b1),
        .clocken2 (1'b1),
        .clocken3 (1'b1),
        .eccstatus (),
        .rden_a (1'b1),
        .rden_b (1'b1));
  defparam
    altsyncram_component.address_reg_b = "CLOCK0",
    altsyncram_component.byteena_reg_b = "CLOCK0",
    altsyncram_component.byte_size = 8,
    altsyncram_component.clock_enable_input_a = "BYPASS",
    altsyncram_component.clock_enable_input_b = "BYPASS",
    altsyncram_component.clock_enable_output_a = "BYPASS",
    altsyncram_component.clock_enable_output_b = "BYPASS",
    altsyncram_component.init_file = INIT_FILE,
    altsyncram_component.init_file_layout = "PORT_A",
    altsyncram_component.indata_reg_b = "CLOCK0",
    altsyncram_component.intended_device_family = DEV_FAMILY,
    altsyncram_component.lpm_type = "altsyncram",
    altsyncram_component.numwords_a = NUM_ELEMS_A,
    altsyncram_component.numwords_b = NUM_ELEMS_B,
    altsyncram_component.operation_mode = "BIDIR_DUAL_PORT",
    altsyncram_component.outdata_aclr_a = "NONE",
    altsyncram_component.outdata_aclr_b = "NONE",
    altsyncram_component.outdata_reg_a = DO_REG_A,
    altsyncram_component.outdata_reg_b = DO_REG_B,
    altsyncram_component.power_up_uninitialized = "FALSE",
    altsyncram_component.read_during_write_mode_mixed_ports = RD_DURING_WR,
    altsyncram_component.read_during_write_mode_port_a = "NEW_DATA_NO_NBE_READ",
    altsyncram_component.read_during_write_mode_port_b = "NEW_DATA_NO_NBE_READ",
    altsyncram_component.widthad_a = ADDR_WIDTH_A,
    altsyncram_component.widthad_b = ADDR_WIDTH_B,
    altsyncram_component.width_a = DATA_WIDTH_A,
    altsyncram_component.width_b = DATA_WIDTH_B,
    altsyncram_component.width_byteena_a = 1,
    altsyncram_component.width_byteena_b = BE_WIDTH,
    altsyncram_component.wrcontrol_wraddress_reg_b = "CLOCK0",
    altsyncram_component.ram_block_type = STYLE;

endmodule
