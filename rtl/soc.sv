// graphite.sv
// Copyright (c) 2022 Daniel Cliche
// SPDX-License-Identifier: MIT

/*
    0x00000000 - 0x00000FFF: block ram (4kB)
    0x10000000 - 0x1001FFFF: single port ram (128kB)
    0x20001000 - 0x00001FFF: display
    0x20002000 - 0x00002FFF: UART (BAUDS-N-8-1)
        0x20002000: Data Register (8 bits)
        0x20002004: Status Register (Read-only)
            bit 0: busy
            bit 1: valid
*/

module soc #(
    parameter FREQ_HZ = 12 * 1000000,
    parameter BAUDS    = 115200,
    parameter RAM_SIZE = 128*1024
    ) (
    input  wire logic       clk,
`ifdef VGA    
    input  wire logic       clk_pix,
`endif
    input  wire logic       reset_i,
    output      logic [7:0] display_o,

    input  wire logic       rx_i,
    output      logic       tx_o,

`ifdef VGA
    output      logic       vga_hsync_o,
    output      logic       vga_vsync_o,
    output      logic [3:0] vga_r_o,
    output      logic [3:0] vga_g_o,
    output      logic [3:0] vga_b_o,
    output      logic       vga_de_o,
    output      logic       audio_l_o,
    output      logic       audio_r_o,
`endif
    );

    // bus
    logic [31:0] addr;
    logic        mem_we, cpu_we;
    logic [31:0] mem_data_in, cpu_data_in;
    logic [31:0] rom_data_out, ram_data_out, mem_data_out, cpu_data_out;
    logic [3:0]  wr_mask;

    // display
    logic [7:0] display;
    logic display_we;

    // UART
    logic uart_tx_strobe;
    logic uart_rx_strobe;

    logic [7:0] uart_tx_data = 0;
    logic [7:0] uart_rx_data;
    logic uart_busy, uart_valid;
    logic uart_wr = 0;
    logic uart_rd = 0;

`ifdef VGA
    // Graphite
    logic           vga_axis_tvalid;
    logic           vga_axis_tready;
    logic [31:0]    vga_axis_tdata;

    // Xosera
    logic         xosera_bus_cs_n, xosera_bus_cs_n_r;           // register select strobe (active low)
    logic         xosera_bus_rd_nwr, xosera_bus_rd_nwr_r;         // 0 = write, 1 = read
    logic [3:0]   xosera_bus_reg_num, xosera_bus_reg_num_r;        // register number
    logic         xosera_bus_bytesel, xosera_bus_bytesel_r;        // 0 = even byte, 1 = odd byte
    logic [7:0]   xosera_bus_data_in, xosera_bus_data_in_r;        // 8-bit data bus input
    logic [7:0]   xosera_bus_data_out;       // 8-bit data bus output    
    logic pulse_xosera_bus;

    always_ff @(posedge clk) begin
        if (reset_i) begin
            pulse_xosera_bus <= 1'b0;
            xosera_bus_cs_n_r <= 1'b1;
        end else begin
            if (pulse_xosera_bus) begin
                xosera_bus_cs_n_r <= 1'b0;
                pulse_xosera_bus <= 1'b0;
            end else begin
                xosera_bus_cs_n_r <= 1'b1;
            end
            if (!xosera_bus_cs_n) begin
                xosera_bus_rd_nwr_r <= xosera_bus_rd_nwr;
                xosera_bus_reg_num_r <= xosera_bus_reg_num;
                xosera_bus_bytesel_r <= xosera_bus_bytesel;
                xosera_bus_data_in_r <= xosera_bus_data_in;
                pulse_xosera_bus <= 1'b1;
            end
        end
    end

`endif

    bram #(
        .SIZE(1024),
        .INIT_FILE("firmware.hex")
    ) rom(
        .clk(clk),
        .sel_i(addr[31:28] == 4'h0),
        .wr_en_i(1'b0),
        .wr_mask_i(wr_mask),
        .address_in_i(32'(addr[27:0] >> 2)),
        .data_in_i(mem_data_in), 
        .data_out_o(rom_data_out)
    );

`ifdef SPRAM
    spram ram(
        .address_in_i(15'(addr[27:0] >> 2)),
`else
    bram #(.SIZE(RAM_SIZE/4)
`ifndef SYNTHESIS
        ,.INIT_FILE("program.hex")
`endif
        ) ram(
        .address_in_i(32'(addr[27:0] >> 2)),
`endif
        .clk(clk),
        .sel_i(addr[31:28] == 4'h1),
        .wr_en_i(mem_we),
        .wr_mask_i(wr_mask),
        .data_in_i(mem_data_in), 
        .data_out_o(ram_data_out)
    );

    processor cpu(
        .clk(clk),
        .reset_i(reset_i),
        .addr_o(addr),
        .we_o(cpu_we),
        .data_in_i(cpu_data_in),
        .data_out_o(cpu_data_out),
        .wr_mask_o(wr_mask)
    );

    uart #(
        .FREQ_HZ(FREQ_HZ),
        .BAUDS(BAUDS)
    ) uart(
        .clk(clk),
        .reset_i(reset_i),
        .tx_o(tx_o),
        .rx_i(rx_i),
        .wr_i(uart_wr),
        .rd_i(uart_rd),
        .tx_data_i(uart_tx_data),
        .rx_data_o(uart_rx_data),
        .busy_o(uart_busy),
        .valid_o(uart_valid)
    );

    always_comb begin
        mem_data_out = (addr[31:28] == 4'h1) ? ram_data_out : rom_data_out;
    end

`ifdef VGA    
    vga vga(
        .clk(clk_pix),
        .reset_i(reset_i),

`ifdef GRAPHITE           
        .cmd_axis_tvalid_i(vga_axis_tvalid),
        .cmd_axis_tready_o(vga_axis_tready),
        .cmd_axis_tdata_i(vga_axis_tdata),
`endif

`ifdef XOSERA
        .xosera_bus_cs_n_i(xosera_bus_cs_n_r),
        .xosera_bus_rd_nwr_i(xosera_bus_rd_nwr_r),
        .xosera_bus_reg_num_i(xosera_bus_reg_num_r),
        .xosera_bus_bytesel_i(xosera_bus_bytesel_r),
        .xosera_bus_data_i(xosera_bus_data_in_r),
        .xosera_bus_data_o(xosera_bus_data_out),
        .xosera_audio_l_o(audio_l_o),
        .xosera_audio_r_o(audio_r_o),
`endif

        .vga_hsync_o(vga_hsync_o),
        .vga_vsync_o(vga_vsync_o),
        .vga_r_o(vga_r_o),
        .vga_g_o(vga_g_o),
        .vga_b_o(vga_b_o),
        .vga_de_o(vga_de_o)
    );
`endif

    // address decoding
    always_comb begin
        mem_we = 1'b0;
        display_we = 1'b0;
        mem_data_in = cpu_data_out;
        display = 8'd0;
        cpu_data_in = mem_data_out;
        uart_tx_strobe = 1'b0;
        uart_rx_strobe = 1'b0;
`ifdef VGA        
        vga_axis_tvalid = 1'b0;
        vga_axis_tdata = cpu_data_out;
        xosera_bus_cs_n = 1'b1;
        xosera_bus_rd_nwr = 1'b1;
        xosera_bus_data_in = cpu_data_out[7:0];
        xosera_bus_bytesel = addr[8];
        xosera_bus_reg_num = addr[7:4];
`endif
        if (cpu_we) begin
            // write
            if (addr[31:28] == 4'h2) begin
                // peripheral
                case (addr[13:12])
                    2'b01: begin
                        // display
                        display_we = 1'b1;
                    end
                    2'b10: begin
                        // UART
                        if (addr[11:0] == 12'd0) begin
                            // data
                            uart_tx_strobe = 1'b1;
                        end
                    end
`ifdef VGA                    
                    2'b11: begin
                        // VGA
                        if (addr[11] == 1'b0) begin
                            // xosera
                            xosera_bus_rd_nwr = 1'b0;
                            xosera_bus_cs_n = 1'b0;
                        end else if (addr[11] == 1'b1) begin
                            // graphite
                            vga_axis_tvalid = 1'b1;
                        end
                    end
`endif
                endcase
            end else begin
                // memory
                mem_we = 1'b1;
            end
        end else begin
            // read
            if (addr[31:28] == 4'h2) begin
                // peripheral
                case (addr[13:12])
                    2'b10: begin
                        // UART
                        if (addr[11:0] == 12'd0) begin
                            // data
                            uart_rx_strobe = 1'b1;
                            cpu_data_in = {24'd0, uart_rx_data};
                        end else if (addr[11:0] == 12'd4) begin
                            // status
                            cpu_data_in = {30'd0, uart_valid, uart_busy};
                        end
                    end
`ifdef VGA                    
                    2'b11: begin
                        // VGA
                        if (addr[11] == 1'b0) begin
                            // xosera
                            xosera_bus_cs_n = 1'b0;
                            cpu_data_in = {24'd0, xosera_bus_data_out};
                        end else if (addr[11] == 1'b1) begin
                            // graphite
                            cpu_data_in = {31'd0, vga_axis_tready};
                        end
                    end
`endif                    
                endcase
            end
        end
    end

    always @(posedge clk) begin
        if (display_we)
            display_o <= cpu_data_out[7:0];
    end

    always @(posedge clk) begin
        if (uart_tx_strobe) begin
            uart_tx_data <= cpu_data_out[7:0];
            uart_wr <= 1'b1;
        end else begin
            uart_wr <= 1'b0;
        end

        if (uart_rx_strobe) begin
            uart_rd <= 1'b1;
        end else begin
            uart_rd <= 1'b0;
        end
    end
    

endmodule