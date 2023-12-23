`timescale 1ns / 1ps
`define PERIOD 20
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/09/2023 10:34:11 PM
// Design Name: 
// Module Name: peripheral_tb
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module peripheral_tb;
    reg axi_clock;
    reg axi_aresetn;
    reg [6:0] axi_awaddr;
    reg [2:0] axi_awprot;
    reg axi_awvalid;
    wire axi_awready;
    reg [31:0] axi_wdata;
    reg [3:0] axi_wstrb;
    reg axi_wvalid;
    wire axi_wready;
    wire [1:0] axi_bresp;
    wire axi_bvalid;
    reg axi_bready; 
    reg [6:0] axi_araddr;
    reg [2:0] axi_arprot;
    reg axi_arvalid;
    wire axi_arready;
    wire [31:0] axi_rdata;
    wire [1:0] axi_rresp;
    wire axi_rvalid;
    reg axi_rready;

    reg [31:0] read_value;

    integer i;

    KetchupPeripheral_v1_0_S00_AXI  #(.C_SHA3_SIZE(224))
        keccak_instance (
        // Global Clock Signal
        .S_AXI_ACLK(axi_clock),
        // Global Reset Signal. This Signal is Active LOW
        .S_AXI_ARESETN(axi_aresetn),
        // Write address (issued by master, acceped by Slave)
        .S_AXI_AWADDR(axi_awaddr),
        // Write channel Protection type. This signal indicates the
            // privilege and security level of the transaction, and whether
            // the transaction is a data access or an instruction access.
        .S_AXI_AWPROT(axi_awprot),
        // Write address valid. This signal indicates that the master signaling
            // valid write address and control information.
        .S_AXI_AWVALID(axi_awvalid),
        // Write address ready. This signal indicates that the slave is ready
            // to accept an address and associated control signals.
        .S_AXI_AWREADY(axi_awready),
        // Write data (issued by master, acceped by Slave) 
        .S_AXI_WDATA(axi_wdata),
        // Write strobes. This signal indicates which byte lanes hold
            // valid data. There is one write strobe bit for each eight
            // bits of the write data bus.    
        .S_AXI_WSTRB(axi_wstrb),
        // Write valid. This signal indicates that valid write
            // data and strobes are available.
        .S_AXI_WVALID(axi_wvalid),
        // Write ready. This signal indicates that the slave
            // can accept the write data.
        .S_AXI_WREADY(axi_wready),
        // Write response. This signal indicates the status
            // of the write transaction.
        .S_AXI_BRESP(axi_bresp),
        // Write response valid. This signal indicates that the channel
            // is signaling a valid write response.
        .S_AXI_BVALID(axi_bvalid),
        // Response ready. This signal indicates that the master
            // can accept a write response.
        .S_AXI_BREADY(axi_bready),
        // Read address (issued by master, acceped by Slave)
        .S_AXI_ARADDR(axi_araddr),
        // Protection type. This signal indicates the privilege
            // and security level of the transaction, and whether the
            // transaction is a data access or an instruction access.
        .S_AXI_ARPROT(axi_arprot),
        // Read address valid. This signal indicates that the channel
            // is signaling valid read address and control information.
        .S_AXI_ARVALID(axi_arvalid),
        // Read address ready. This signal indicates that the slave is
            // ready to accept an address and associated control signals.
        .S_AXI_ARREADY(axi_arready),
        // Read data (issued by slave)
        .S_AXI_RDATA(axi_rdata),
        // Read response. This signal indicates the status of the
            // read transfer.
        .S_AXI_RRESP(axi_rresp),
        // Read valid. This signal indicates that the channel is
            // signaling the required read data.
        .S_AXI_RVALID(axi_rvalid),
        // Read ready. This signal indicates that the master can
            // accept the read data and response information.
        .S_AXI_RREADY(axi_rready)
    );

    reg [2:0] core_idx;

    initial begin
        $dumpfile("signals.vcd");
        $dumpvars(0, peripheral_tb);

        axi_clock = 0;

        axi_awvalid = 0;
        axi_wvalid = 0;
        axi_bready = 0;

        axi_arvalid = 0;
        axi_rready = 0;

        axi_araddr = 0;
        axi_awaddr = 0;

        // Reset Procedure
        axi_aresetn = 0;
        #(`PERIOD);
        axi_aresetn = 1;
        #(`PERIOD);
        

        `define REG_CONTROL 7'h00
        `define REG_STATUS  7'h04
        `define REG_INPUT   7'h08
        `define REG_COMMAND 7'h0C
        `define REG_OUTPUT  7'h10

        // $display("Value read from register 3 is 0x%08h", read_value);

        // Control Register:
        // Bit 1:0 - how many bytes to send
        // Bit 2   - is this the last bit

        // ""
        write_procedure(`REG_COMMAND, 32'h1);

        write_procedure(`REG_CONTROL, 32'h4);
        write_procedure(`REG_INPUT, "   ");

        read_procedure(`REG_STATUS);
        while ((read_value & 2'b1) == 0) begin
            read_procedure(`REG_STATUS);
        end

        read_procedure(`REG_OUTPUT);

        $display("Sha3(\"\") = %08h...", read_value);

        
        // "Hello World"
        write_procedure(`REG_COMMAND, 32'h1);

        write_procedure(`REG_CONTROL, 32'h0);
        write_procedure(`REG_INPUT, "Hell");
        write_procedure(`REG_INPUT, "o Wo");

        write_procedure(`REG_CONTROL, 32'h7);
        write_procedure(`REG_INPUT, "rld ");

        read_procedure(`REG_STATUS);
        while ((read_value & 2'b1) == 0) begin
            read_procedure(`REG_STATUS);
        end

        read_procedure(`REG_OUTPUT);

        // Hash finished! Retrieve value
        $display("Sha3(\"Hello World\") = %08h...", read_value);

        // "Hello World!"
        write_procedure(`REG_COMMAND, 32'h1);

        write_procedure(`REG_CONTROL, 32'h0);
        write_procedure(`REG_INPUT, "Hell");
        write_procedure(`REG_INPUT, "o Wo");
        write_procedure(`REG_INPUT, "rld!");

        write_procedure(`REG_CONTROL, 32'h4);
        write_procedure(`REG_INPUT, "    ");

        read_procedure(`REG_STATUS);
        while ((read_value & 2'b1) == 0) begin
            read_procedure(`REG_STATUS);
        end

        read_procedure(`REG_OUTPUT);

        $display("Sha3(\"Hello World!\") = %08h...", read_value);

        // "Hello"
        write_procedure(`REG_COMMAND, 32'h1);

        write_procedure(`REG_CONTROL, 32'h0);
        write_procedure(`REG_INPUT, "Hell");

        write_procedure(`REG_CONTROL, 32'h5);
        write_procedure(`REG_INPUT, "o   ");

        read_procedure(`REG_STATUS);
        while ((read_value & 2'b1) == 0) begin
            read_procedure(`REG_STATUS);
        end

        read_procedure(`REG_OUTPUT);

        $display("Sha3(\"Hello\") = %08h...", read_value);

        // "hi mom"
        write_procedure(`REG_COMMAND, 32'h1);

        write_procedure(`REG_CONTROL, 32'h0);
        write_procedure(`REG_INPUT, "hi m");

        write_procedure(`REG_CONTROL, 32'h6);
        write_procedure(`REG_INPUT, "om  ");

        read_procedure(`REG_STATUS);
        while ((read_value & 2'b1) == 0) begin
            read_procedure(`REG_STATUS);
        end

        read_procedure(`REG_OUTPUT);

        $display("Sha3(\"hi mom\") = %08h...", read_value);

        // "This is a very very very very very very long test test test test test!";
        write_procedure(`REG_COMMAND, 32'h1);

        write_procedure(`REG_CONTROL, 32'h0);
        write_procedure(`REG_INPUT, "This");
        write_procedure(`REG_INPUT, " is ");
        write_procedure(`REG_INPUT, "a ve");
        write_procedure(`REG_INPUT, "ry v");
        write_procedure(`REG_INPUT, "ery ");
        write_procedure(`REG_INPUT, "very");
        write_procedure(`REG_INPUT, " ver");
        write_procedure(`REG_INPUT, "y ve");
        write_procedure(`REG_INPUT, "ry v");
        write_procedure(`REG_INPUT, "ery ");
        write_procedure(`REG_INPUT, "long");
        write_procedure(`REG_INPUT, " tes");
        write_procedure(`REG_INPUT, "t te");
        write_procedure(`REG_INPUT, "st t");
        write_procedure(`REG_INPUT, "est ");
        write_procedure(`REG_INPUT, "test");
        write_procedure(`REG_INPUT, " tes");

        write_procedure(`REG_CONTROL, 32'h6);
        write_procedure(`REG_INPUT, "t!  ");

        read_procedure(`REG_STATUS);
        while ((read_value & 2'b1) == 0) begin
            read_procedure(`REG_STATUS);
        end

        read_procedure(`REG_OUTPUT);

        $display("Sha3(\"This is a very very very very very very long test test test test test!\") = %08h...", read_value);

        #200;
        $finish;
    end

    task write_procedure;
        input [8:0] write_address;
        input [31:0] write_data;
        begin
            // Put address and write data on the bus
            axi_awaddr = write_address;
            axi_wdata = write_data;

            // All byte writes are valid, so write all ones
            axi_wstrb = 4'b1111;


            while (axi_clock !== 0) #10;

            // The address is valid
            axi_awvalid = 1;

            // The written value is also valid
            axi_wvalid = 1;

            // Wait for the slave to ACK us about that
            while (!(axi_awready && axi_wready)) #(`PERIOD);

            // Signal the slave that I'm ready to receive a response
            axi_bready = 1;

            // Wait for the slave to send me the repsonse
            while (!axi_bvalid) #(`PERIOD);

            // Check if the slave said that everything is ok
            if (axi_rresp != 2'b00) begin
                $display("ERROR IN READING: %02b", axi_rresp);
            end

            // We're done here, we can reset all signals now
            axi_awvalid <= 0;
            axi_wvalid <= 0;
            // axi_bready <= 1;
            // while (axi_bvalid) #(`PERIOD);
            axi_bready <= 0;

            #(`PERIOD * 2);
        end
    endtask

    task read_procedure;
        input [8:0] read_address;
        begin
            axi_araddr = read_address; //7'h8;

            // Wait for clock down
            while (axi_clock !== 0) #10;

            // The address on the line is valid
            axi_arvalid = 1;

            // I'm ready to receive data
            axi_rready = 1;

            // Wait for the slave to ACK my ready
            while (!axi_arready) #(`PERIOD);

            // Wait for the slave to send valid data
            while (!axi_rvalid) #(`PERIOD);

            // Check for read success. Response hould be 00
            if (axi_rresp != 2'b00) begin
                $display("ERROR IN READING: %02b", axi_rresp);
            end

            // Now read your value
            read_value = axi_rdata;

            // We're done here, we remove rready and arvalid
            axi_rready <= 0;
            axi_arvalid <= 0;

            #(`PERIOD * 2);
        end
    endtask


    always #(`PERIOD/2) axi_clock = ~axi_clock;

endmodule
